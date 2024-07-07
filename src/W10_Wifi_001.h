
#pragma once

#include <WiFi.h>
#include <WiFiMulti.h>

typedef struct {
		String		SSID;
		String		PWD;
} B10_T2_WIFI_LOGIN_t;

	
const uint8_t			g_W10_T2_Wifi_Login_Arr_Size 	= 5;

B10_T2_WIFI_LOGIN_t		g_W10_T2_Wifi_Login_Arr[g_W10_T2_Wifi_Login_Arr_Size] = { 
													  { .SSID = "GalNote10-2540"	, .PWD = "91932542"		}
													, { .SSID = "2G_1"				, .PWD = "65641077"		}
													, { .SSID = "LSITC_WIFI_AP"		, .PWD = "1234554321"	}
													, { .SSID = "U+Net83BF"			, .PWD = "010C020032"	}
													, { .SSID = "LGU+_M200_732675"	, .PWD = "55070016"		}
												};




WiFiMulti   			g_W10_T2_WIfiMulti;

String 					g_W10_Wifi_IP_Addr ;


void W10_T2_Wifi_init(){
	
	for (uint8_t i=0; i < g_W10_T2_Wifi_Login_Arr_Size; i++){
		g_W10_T2_WIfiMulti.addAP(g_W10_T2_Wifi_Login_Arr[i].SSID.c_str(), g_W10_T2_Wifi_Login_Arr[i].PWD.c_str());
	}

	
	while(g_W10_T2_WIfiMulti.run() != WL_CONNECTED) {
		Serial.print(".");
		delay(500);
	}

	if(WiFi.isConnected()) {

		#ifdef DBG_PROGRESS1
			Serial.println("WiFI_info:   Connected WiFi...");

			Serial.print(" SSID : "); Serial.println(WiFi.SSID() );
			Serial.print(" IP Addr : "); Serial.println(WiFi.localIP() );
		#endif 

		g_W10_Wifi_IP_Addr = WiFi.localIP().toString(); 		

		WiFi.setSleep(false);
	} else {
		#ifdef DBG_ERROR
			Serial.println("WiFI_info: WiFi credentials are not correct"); //  " ANSI_ESC_RED "WiFi credentials are not correct");
		#endif
	}
}