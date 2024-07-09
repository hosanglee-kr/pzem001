#pragma once


#define C20_T2_AWEB_ENABLED

#ifdef C20_T2_AWEB_ENABLED

	#include <ESPAsyncWebServer.h>

	#define					G_C20_T2_ASYNCWEBSRV_PORT			90
	#define					G_C20_T2_ASYNCWEBSOCKETSRV_PORT		91

	AsyncWebServer			g_C20_T2_AsyncWebSrv(G_C20_T2_ASYNCWEBSRV_PORT);

	AsyncWebSocket 			g_C20_T2_AsyncWebSocket_FFT2("/ws"); 				// access at ws://[esp ip]/ws
	//AsyncWebSocket 			g_C20_T2_AsyncWebSocket_Mic("/mic_ws"); 				// access at ws://[esp ip]/ws

	AsyncEventSource 		g_C20_T2_AsyncWebSocket_ServerSideSendEvents("/events"); 								// event source (Server-Sent events)


	//#define G_C20_AWEB_FS_SPIFFS
	#define G_C20_AWEB_FS_LittleFS


	#ifdef G_C20_AWEB_FS_SPIFFS
		#ifndef LittleFS
			#define	LittleFS			SPIFFS
		#endif

		#include "SPIFFS.h"
	#endif

	#ifdef G_C20_AWEB_FS_LittleFS
		#include "FS.h"
		#include <LittleFS.h>

		#define SPIFFS 					LittleFS

	#endif

	void C20_FS_init() {

		#ifdef G_C20_AWEB_FS_SPIFFS
			if (!SPIFFS.begin(true)) {
				Serial.println("An error has occurred while mounting SPIFFS");
			}
			Serial.println("SPIFFS mounted successfully");
		#endif
		#ifdef G_C20_AWEB_FS_LittleFS
			if(!LittleFS.begin(false)){
				Serial.println("LittleFS Mount Failed");
				return;
			}
		#endif
	}


	void C20_T2_AsyncWebSocket_OnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {

		// client connected

		if (type == WS_EVT_CONNECT) {

			#ifdef DBG_PROGRESS2
				Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
			#endif

			client->printf("Hello Client %u :)", client->id());
			client->ping();

			#ifdef DBG_PROGRESS2
				IPAddress v_ip = client->remoteIP();
				Serial.printf("WS Connected from %d.%d.%d.%d\n", v_ip[0], v_ip[1], v_ip[2], v_ip[3]);
			#endif

			// send message to client
			//g_C10_T2_WebSocketSrv.sendTXT(num, "Connected");

		} else if (type == WS_EVT_DISCONNECT) {
			// client disconnected
			#ifdef DBG_PROGRESS2
				Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
			#endif

		} else if (type == WS_EVT_ERROR) {
			// error was received from the other end
			#ifdef DBG_PROGRESS2
				Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
			#endif

		} else if (type == WS_EVT_PONG) {
			// pong message was received (in response to a ping request maybe)
			#ifdef DBG_PROGRESS2
				Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
			#endif
		} else if (type == WS_EVT_DATA) {
			// data packet
			AwsFrameInfo *info = (AwsFrameInfo *)arg;

			if (info->final && info->index == 0 && info->len == len) {
				// the whole message is in a single frame and we got all of it's data

				#ifdef DBG_PROGRESS2
					Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
				#endif

				if (info->opcode == WS_TEXT) {
					data[len] = 0;
					Serial.printf("%s\n", (char *)data);
				} else {
					for (size_t i = 0; i < info->len; i++) {
						Serial.printf("%02x ", data[i]);
					}
					Serial.printf("\n");
				}
				if (info->opcode == WS_TEXT)
					client->text("I got your text message");
				else
					client->binary("I got your binary message");
			} else {
				// message is comprised of multiple frames or the frame is split into multiple packets
				if (info->index == 0) {
					if (info->num == 0)
						Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
					Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
				}

				Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);
				if (info->message_opcode == WS_TEXT) {
					data[len] = 0;
					Serial.printf("%s\n", (char *)data);
				} else {
					for (size_t i = 0; i < len; i++) {
						Serial.printf("%02x ", data[i]);
					}
					Serial.printf("\n");
				}

				if ((info->index + len) == info->len) {
					Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
					if (info->final) {
						Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
						if (info->message_opcode == WS_TEXT)
							client->text("I got your text message");
						else
							client->binary("I got your binary message");
					}
				}
			}
		}
	}


#define		X25_URL_TimeSerial_FILE		"/C10_T2_WEB_TimeChart_002.html"
#define		X25_URL_TimeSerial_URI		"/time"
#define		X25_URL_TimeSerial_NAME		" Time Series"


#define		X25_URL_FFT2_FILE			"/C10_T2_WEB_FFT2_001.html"
#define		X25_URL_FFT2_URI			"/fft2"
#define		X25_URL_FFT2_NAME			"Spectrum Analyzer"


	void C20_T2_AWeb_handle_init(){

		#ifdef G_C20_AWEB_FS_SPIFFS
			// Route for root / web page
			g_C20_T2_AsyncWebSrv.on(X25_URL_FFT2_URI, HTTP_GET, [](AsyncWebServerRequest *request){
				request->send(SPIFFS, X25_URL_FFT2_FILE, "text/html",false);
			});

			// g_C20_T2_AsyncWebSrv.on("/fft2", HTTP_GET, [](AsyncWebServerRequest *request){
			// 	request->send(SPIFFS, "/C10_T2_WEB_FFT2_001.html", "text/html",false);
			// });

			g_C20_T2_AsyncWebSrv.serveStatic("/fft2", SPIFFS, "/fft2");
		#endif

		#ifdef G_C20_AWEB_FS_LittleFS

			g_C20_T2_AsyncWebSrv.on(X25_URL_TimeSerial_URI, HTTP_GET, [](AsyncWebServerRequest *request){
				request->send(LittleFS, X25_URL_TimeSerial_FILE, "text/html",false);
			});

			g_C20_T2_AsyncWebSrv.on(X25_URL_FFT2_URI, HTTP_GET, [](AsyncWebServerRequest *request){
				request->send(LittleFS, X25_URL_FFT2_FILE, "text/html",false);
			});

		#endif
	}

	void C20_T2_AWeb_init(){

		g_C20_T2_AsyncWebSrv.begin();

		g_C20_T2_AsyncWebSocket_FFT2.onEvent(C20_T2_AsyncWebSocket_OnEvent);

		g_C20_T2_AsyncWebSrv.addHandler(&g_C20_T2_AsyncWebSocket_FFT2);
		//g_C20_T2_AsyncWebSrv.addHandler(&g_C20_T2_AsyncWebSocket_Mic);

		// attach AsyncEventSource
		g_C20_T2_AsyncWebSrv.addHandler(&g_C20_T2_AsyncWebSocket_ServerSideSendEvents);

		g_C20_T2_AsyncWebSrv.begin();


		//g_C20_T2_AsyncWebSocket_ServerSideSendEvents.send(temp, "time"); //send event "time"


	}


	// {
	//     "timeserial": {
	//         [
	// 			{
	//                 "datetime": 1111,
	//                 "value": 1112
	//             }, {
	//                 "datetime": 1112,
	//                 "value": 1111
	//             }

	//         ]
	//     }
	// }



	// void C20_T2_AWeb_Send_Data_1(){


	// 	String v_json 	 = 	"{ \"fft2\":";
	// 	v_json 			+=	"[";

	// 	uint8_t v_count = 0;



	// 	#if defined(G_B51_PZEM_MODEL_PZEM004V3)
	// 		Serial.println("ts,\t\tV,\tA,\tW,\tWh,\tdHz,\tpf");
	// 		//Serial.println("TimeStamp\t\tdV\tmA\tW\tWh\tdHz\tpf");
	// 	#elif defined(G_B51_PZEM_MODEL_PZEM003)
	// 		Serial.println("ts,\t\tV,\tA,\tW,\tWh");
	// 		//Serial.println("TimeStamp\t\tdV\tmA\tW\tWh");
	// 	#endif



	// 	for (int i = 0; i < g_B10_oct_bands_num; i++) {

	// 		if (gp_B10_Oct_Bands[i].freq_center >= g_C20_FreqDisp_Low && gp_B10_Oct_Bands[i].freq_center <= g_C20_FreqDisp_High) {
	// 		//if (gp_B10_Oct_Bands[i].freq_center >= 80.0 && gp_B10_Oct_Bands[i].freq_center <= 8000.0) {
	// 			if (v_count > 0) {
	// 				v_json 	+= ", ";
	// 			}

	// 			// if (i > 0) {
	// 			// 	v_json 	+= ", ";
	// 			// }
	// 			v_json += "{\"bin\":";
	// 			v_json += "\"" + String(gp_B10_Oct_Bands[i].freq_center,1) + "\"";

	// 			v_json += ", \"value\":";
	// 			v_json += String(gp_B10_Oct_Bands[i].magnitude, 0);

	// 			v_json += ", \"dB\":";
	// 			v_json += String(gp_B10_Oct_Bands[i].decibel, 0);

	// 			v_json += "}";

	// 			v_count++;
	// 		}

	// 	}
	// 	v_json += "]";
	// 	v_json += "}";


	// 	g_C20_T2_AsyncWebSocket_FFT2.textAll(v_json.c_str(), v_json.length());

	// 	#ifdef DBG_PROGRESS2
	// 		Serial.println("------------------Send Json");
	// 		Serial.println(v_json);
	// 		Serial.println();
	// 	#endif
	// }



	// // uint16_t g_C20_FreqDisp_Low 	= 80;
	// // uint16_t g_C20_FreqDisp_High 	= 8000;

	// void C20_T2_AWeb_Send_FFT2_Data_old(){
	// 	String v_json 	 = 	"{ \"fft2\":";
	// 	v_json 			+=	"[";

	// 	uint8_t v_count = 0;

	// 	for (int i = 0; i < g_B10_oct_bands_num; i++) {

	// 		if (gp_B10_Oct_Bands[i].freq_center >= g_C20_FreqDisp_Low && gp_B10_Oct_Bands[i].freq_center <= g_C20_FreqDisp_High) {
	// 		//if (gp_B10_Oct_Bands[i].freq_center >= 80.0 && gp_B10_Oct_Bands[i].freq_center <= 8000.0) {
	// 			if (v_count > 0) {
	// 				v_json 	+= ", ";
	// 			}

	// 			// if (i > 0) {
	// 			// 	v_json 	+= ", ";
	// 			// }
	// 			v_json += "{\"bin\":";
	// 			v_json += "\"" + String(gp_B10_Oct_Bands[i].freq_center,1) + "\"";

	// 			v_json += ", \"value\":";
	// 			v_json += String(gp_B10_Oct_Bands[i].magnitude, 0);

	// 			v_json += ", \"dB\":";
	// 			v_json += String(gp_B10_Oct_Bands[i].decibel, 0);

	// 			v_json += "}";

	// 			v_count++;
	// 		}

	// 	}
	// 	v_json += "]";
	// 	v_json += "}";


	// 	g_C20_T2_AsyncWebSocket_FFT2.textAll(v_json.c_str(), v_json.length());

	// 	#ifdef DBG_PROGRESS2
	// 		Serial.println("------------------Send Json");
	// 		Serial.println(v_json);
	// 		Serial.println();
	// 	#endif
	// }



	void C20_T2_AWeb_run(){
		g_C20_T2_AsyncWebSocket_FFT2.cleanupClients();
	}

#endif
