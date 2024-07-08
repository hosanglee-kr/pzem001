#pragma once


#include <Arduino.h>
#include <WiFi.h>

#include <ctime>

using namespace pzmbus;

#include "esp_sntp.h"
#include "pzem_edl.hpp"
#include "timeseries.hpp"

// This example shows how to collect TimeSeries data for PZEM metrics
// Pls, check previous examples for basic operations
// For this example we need an internet connection to get time from NTP server

const char* g_B51_ssid		 				= "2G_1";
const char* g_B51_password	 				= "65641077";

const char* g_B51_NTP_ServerURL 			= "pool.ntp.org";
const char* g_B51_NTP_TIMEZONE	 			= "KST-9";


//const pzmodel_t model;					//  enum class pzmodel_t:uint8_t { none, pzem004v3, pzem003 };

pzmbus::pzmodel_t g_B51_PZEM_model			= pzmbus::pzmodel_t::pzem003;

// #define		G_B51_PZEM_MODEL				"PZ003"		//	DC: PZ003, AC : PZ004
// #define		G_B51_PZEM_DUMMY				Y



// #if G_B51_PZEM_MODEL	== "PZ003"

// #elif G_B51_PZEM_MODEL	== "PZ004"

// #endif

#define		G_B51_PZEM_REFRESH_PERIOD_MS	1000		// default : 1000ms, min : 200ms


//using namespace pz004;												// we will need this namespace for PZEM004v3.0 device

#define G_B51_PZEM_UART_PORT 				UART_NUM_1	 			// port attached to pzem (UART_NUM_1 is 2-nd uart port on ESP32, 1-st one is usually busy with USB-to-serial chip)

#define G_B51_RX_PIN		 				22	 					// custom RX pin number
#define G_B51_TX_PIN		 				19	 					// custom TX pin number

#define G_B51_PZEM_ID_PZ003		 			40
#define G_B51_PZEM_ID_PZ003_DUMMY			41
#define G_B51_PZEM_ID_PZ004		 			42	 					// this is a unique PZEM ID for reference, it is NOT slave MODBUS address, just some random id (we all know why '42' is THE number, right? :) )



// first, we need a placeholder for UartQ object to handle RX/TX message queues
UartQ*							g_B51_qport;

// Also we need a placeholder for PZEM004 object

//PZ003*						g_B51_pz003;
DummyPZ003*						g_B51_pz003;		//g_B51_pz003_dummy;

//PZ004*						g_B51_pz004;


// Container object for TimeSeries data
//TSContainer<pz003::metrics> g_B51_TsC_pz003;
TSContainer<pz003::metrics> 	g_B51_TsC_pz003;		//g_B51_TsC_pz003_dummy;

//TSContainer<pz004::metrics> g_B51_TsC_pz004;

// IDs for out time series buffers
uint8_t						g_B51_TsC_ID_001sec;
uint8_t						g_B51_TsC_ID_030sec;
uint8_t						g_B51_TsC_ID_300sec;

// I need a timer to do console printing
TimerHandle_t				g_B51_TimerHandle_5sec;

void B51_WIFI_Connect();
void B51_NTP_init();


// forward declarations
void B51_print_wait4data(void*);

// a call-back function that will configure TimeSeries buffers
void B51_NTP_sync_cb_timeseries(struct timeval* t);

void B51_PZEM_init();
void B51_ESP32_MEMORY_info_print();


void B51_PZEM_init(){
	// Create new serial Q object attached to specified gpios
	g_B51_qport 		= new UartQ(G_B51_PZEM_UART_PORT, G_B51_RX_PIN, G_B51_TX_PIN);



	// Now let's create a PZEM004 object
	g_B51_pz003 		= new DummyPZ003(G_B51_PZEM_ID_PZ003_DUMMY);
	//g_B51_pz003 		= new PZ003(G_B51_PZEM_ID_PZ003);
	//g_B51_pz003_dummy 	= new DummyPZ003(G_B51_PZEM_ID_PZ003_DUMMY);

	//g_B51_pz004 		= new PZ004(G_B51_PZEM_ID_PZ004);


	// and link our port with PZEM object
	g_B51_pz003->attachMsgQ(g_B51_qport);
	//g_B51_pz003_dummy->attachMsgQ(g_B51_qport);

	//g_B51_pz004->attachMsgQ(g_B51_qport);

	// one last step - we must start PortQ tasks to handle messages
	g_B51_qport->startQueues();


	// enable polling
	if (g_B51_pz003->autopoll(true)){
		Serial.println("Autopolling enabled - g_B51_pz003");
	} else {
		Serial.println("Sorry, can't autopoll somehow :( - g_B51_pz003");
	}

	// if (g_B51_pz003_dummy->autopoll(true)){
	// 	Serial.println("Autopolling enabled - g_B51_pz003_dummy");
	// } else {
	// 	Serial.println("Sorry, can't autopoll somehow :( - g_B51_pz003_dummy");
	// }

	// if (g_B51_pz004->autopoll(true)){
	// 	Serial.println("Autopolling enabled - g_B51_pz004");
	// } else {
	// 	Serial.println("Sorry, can't autopoll somehow :( - g_B51_pz004");
	// }

	//g_B51_pz003->getPollrate();
	g_B51_pz003->setPollrate(G_B51_PZEM_REFRESH_PERIOD_MS);

	Serial.println("===");

	B51_ESP32_MEMORY_info_print();

}



void B51_init() {
	// Serial.begin(115200);       // just an ordinary Serial console to interact with

	Serial.printf("\n\n\n\tPZEM004 TimeSeries example\n\n");

	B51_PZEM_init();


	B51_NTP_init();

	//// B51_WIFI_Connect();


}

void B51_run() {
	// we do not need this loop at all :)
	for (;;) {
		delay(1000);
	}
}

// Callback function (get's called when time adjusts via NTP)
void B51_NTP_sync_cb_timeseries(struct timeval* t) {

	Serial.println("Got time adjustment from NTP!");
	Serial.println("\nAllocate sampler buffer");

	// this will create TS object that holds per-second metrics data


	// total 300 samples will keep per-second data for the duration of 5 minute, then it will roll-over.
	// Each sample takes about 28 bytes of (SPI)-RAM, it's not a problem to store thouthands if you have SPI-RAM

	g_B51_TsC_ID_001sec = g_B51_TsC_pz003.addTS(
	//g_B51_TsC_ID_001sec = g_B51_TsC_pz004.addTS(
									  300						// s - number of entries to keep
									, time(nullptr) 			// current timestamp : start_time - timestamp of TS creation
									, 1 						// second interval period - sampling period, all samples that are pushed to TS with lesser interval will be either dropped or averaged if averaging function is provided
									, "TimeSeries - Interval: 1Sec, sample: 300" 	// descr - mnemonic description (pointer MUST be valid for the duraion of life-time, it won't be deep-copied)
																// id - desired ID
								);
	Serial.printf("Add per-second TimeSeries, id: %d\n", g_B51_TsC_ID_001sec);

	// the same for 30-seconds interval, 240 samples totals, will keep data for 120 min
	g_B51_TsC_ID_030sec = g_B51_TsC_pz003.addTS(
	//g_B51_TsC_ID_030sec = g_B51_TsC_pz004.addTS(
									  240											// s - number of entries to keep
									, time(nullptr) 								// current timestamp
									, 30 											// second interval
									, "TimeSeries - Interval: 30Sec, sample: 240" 	// "TimeSeries 30 Seconds" 	// Mnemonic descr
																					// id - desired ID
								);
	Serial.printf("Add 30 second TimeSeries, id: %d\n", g_B51_TsC_ID_030sec);

	// the same for 300-seconds interval, 288 samples totals, will keep data for 24 hours
	g_B51_TsC_ID_300sec = g_B51_TsC_pz003.addTS(
	//g_B51_TsC_ID_300sec = g_B51_TsC_pz004.addTS(
									  288
									, time(nullptr) 								// current timestamp
									, 300 											// second interval
									, "TimeSeries - Interval: 5Min, sample: 288"  	//"TimeSeries 5 Min" 		// Mnemonic descr
								);
	Serial.printf("Add 5 min TimeSeries, id: %d\n", g_B51_TsC_ID_300sec);

	// check memory usage, if SPI-RAM is available on board, than TS will allocate buffer in SPI-RAM
	Serial.println();
	B51_ESP32_MEMORY_info_print();

	//  Now I need to hookup to the PZEM object, it autopolls every second so I can use it's callback
	//  to collect metrics data and push it to TSContainer
	// 이제 PZEM 개체에 연결해야 합니다. 매초 자동 폴링되므로 콜백을 사용하여 메트릭 데이터를 수집하고 TSContainer에 푸시할 수 있습니다.
	auto v_TsC_pz003_ref = &g_B51_TsC_pz003;
	//auto v_TsC_pz004_ref = &g_B51_TsC_pz004;

	// a ref of our Container to feed it to lambda function
	g_B51_pz003->attach_rx_callback([v_TsC_pz003_ref](uint8_t pzid, const RX_msg* m) {
	//g_B51_pz004->attach_rx_callback([v_TsC_pz004_ref](uint8_t pzid, const RX_msg* m) {
		// obtain a pointer to objects metrics and push data to TS container marking it with current timestamp
		// objects 메트릭에 대한 포인터를 얻고 현재 타임스탬프로 표시하는 TS 컨테이너에 데이터를 푸시합니다.
		v_TsC_pz003_ref->push(*(g_B51_pz003->getMetricsPZ003()), time(nullptr));
		//v_TsC_pz003_ref->push(*(g_B51_pz003->getMetricsPZ003()), time(nullptr));

		//v_TsC_pz004_ref->push(*(g_B51_pz004->getMetricsPZ004()), time(nullptr));
	});

	// here I will set a timer to do printing task to serial
	g_B51_TimerHandle_5sec = xTimerCreate(
					"5sec"						// 	pcTimerName
					, pdMS_TO_TICKS(5000)		//	TickType_t xTimerPeriodInTicks,
					, pdTRUE					//	UBaseType_t uxAutoReload
					, nullptr					//	pvTimerID
					, B51_print_wait4data		//	pxCallbackFunction
				);

	xTimerStart(
					g_B51_TimerHandle_5sec, 	// xTimer handle
					100							// xTicksToWait
				);
};


void B51_print_timeseries_each(uint8_t p_B51_TsC_ID_001sec){

	struct tm v_tmstruct;
	char v_dtime_char[30];

	//g_B51_pz003->getState()->model

	// get a ptr to TimeSeries buffer
	auto   v_ts	= g_B51_TsC_pz003.getTS(p_B51_TsC_ID_001sec);
	//auto   v_ts	= g_B51_TsC_pz004.getTS(g_B51_TsC_ID_001sec);


	// get const iterator pointing to the begining of the buffer, i.e. the oldest data sample
	auto   v_iter = v_ts->cbegin();




	// for the sake of simplicity so no to clotter terminal with printin all 300 samples from buffer let's print only 10 most recent samples
	// it will also show how to manipulate with iterators

	size_t v_cnt	= 10;  // we need only 10 samples

	// now we need to shift the iterator from the beginning of the buffer to 'end'-10, i.e. 10 most recent items from the end
	v_iter += v_ts->getSize() - v_cnt;

	Serial.printf("TimeSeries buffer %s has %d items, will only get last %d\n", v_ts->getDescr(), v_ts->getSize(), v_cnt);

	// let's run the iterator and print sampled data

	Serial.println("TimeStamp\t\tdV\tmA\tW\tWh\tdHz\tpf");
	for (v_iter; v_iter != v_ts->cend(); ++v_iter) {

		// now I need to get the timestamp for each sample
		// TS buffer only stores timestamp for the last sample, not for the each item,
		// so I need to calculate the time based on last timestamp, interval and counter

		std::time_t v_timestamp_lastupdate	= v_ts->getTstamp() - (v_ts->cend() - v_iter) * v_ts->getInterval();
		//auto v_timestamp_lastupdate = v_ts->getTstamp() - (v_ts->cend() - v_iter) * v_ts->getInterval();

		localtime_r(&v_timestamp_lastupdate, &v_tmstruct);

		//getLocalTime(&tmstruct, 5000);
		sprintf(v_dtime_char,"%d-%02d-%02d %02d:%02d:%02d",
						(v_tmstruct.tm_year)+1900,
						(v_tmstruct.tm_mon)+1,
						v_tmstruct.tm_mday,
						v_tmstruct.tm_hour,
						v_tmstruct.tm_min,
						v_tmstruct.tm_sec
					);


		Serial.print(v_dtime_char);	Serial.print("\t");
		//Serial.print(v_timestamp_lastupdate);	Serial.print("\t");
		Serial.print(v_iter->voltage);			Serial.print("\t");
		Serial.print(v_iter->current);			Serial.print("\t");
		Serial.print(v_iter->power);			Serial.print("\t");
		Serial.print(v_iter->energy);			Serial.print("\t");
		//// Serial.print(v_iter->freq);				Serial.print("\t");
		//// Serial.print(v_iter->pf);
		Serial.println();
	}

}

// function is triggered by a timer each minute
void B51_print_timeseries() {
	// Print timeseries for 1 30 300 sec
	Serial.println();
	Serial.println("=== Print TimeSeries data ===");


	struct tm v_tmstruct;
	char v_dtime_char[30];

	// Process 1 sec data

	// get a ptr to TimeSeries buffer
	auto   v_ts	= g_B51_TsC_pz003.getTS(g_B51_TsC_ID_001sec);
	//auto   v_ts	= g_B51_TsC_pz004.getTS(g_B51_TsC_ID_001sec);

	// get const iterator pointing to the begining of the buffer, i.e. the oldest data sample
	auto   v_iter = v_ts->cbegin();

	// for the sake of simplicity so no to clotter terminal with printin all 300 samples from buffer let's print only 10 most recent samples
	// it will also show how to manipulate with iterators

	size_t v_cnt	= 10;  // we need only 10 samples

	// now we need to shift the iterator from the beginning of the buffer to 'end'-10, i.e. 10 most recent items from the end
	v_iter += v_ts->getSize() - v_cnt;

	Serial.printf("TimeSeries buffer %s has %d items, will only get last %d\n", v_ts->getDescr(), v_ts->getSize(), v_cnt);

	// let's run the iterator and print sampled data

	Serial.println("TimeStamp\t\tdV\tmA\tW\tWh\tdHz\tpf");
	for (v_iter; v_iter != v_ts->cend(); ++v_iter) {

		// now I need to get the timestamp for each sample
		// TS buffer only stores timestamp for the last sample, not for the each item,
		// so I need to calculate the time based on last timestamp, interval and counter

		std::time_t v_timestamp_lastupdate	= v_ts->getTstamp() - (v_ts->cend() - v_iter) * v_ts->getInterval();
		//auto v_timestamp_lastupdate = v_ts->getTstamp() - (v_ts->cend() - v_iter) * v_ts->getInterval();

		localtime_r(&v_timestamp_lastupdate, &v_tmstruct);

		//getLocalTime(&tmstruct, 5000);
		sprintf(v_dtime_char,"%d-%02d-%02d %02d:%02d:%02d",
						(v_tmstruct.tm_year)+1900,
						(v_tmstruct.tm_mon)+1,
						v_tmstruct.tm_mday,
						v_tmstruct.tm_hour,
						v_tmstruct.tm_min,
						v_tmstruct.tm_sec
					);


		Serial.print(v_dtime_char);	Serial.print("\t");
		//Serial.print(v_timestamp_lastupdate);	Serial.print("\t");
		Serial.print(v_iter->voltage);			Serial.print("\t");
		Serial.print(v_iter->current);			Serial.print("\t");
		Serial.print(v_iter->power);			Serial.print("\t");
		Serial.print(v_iter->energy);			Serial.print("\t");
		//// Serial.print(v_iter->freq);				Serial.print("\t");
		//// Serial.print(v_iter->pf);
		Serial.println();
	}

	// Let's do same for 30 sec TimeSeries
	// just to make it a bit differetnt let't print human-readable date/time instead of time-stamp

	v_ts	 = g_B51_TsC_pz003.getTS(g_B51_TsC_ID_030sec);
	//v_ts	 = g_B51_TsC_pz004.getTS(g_B51_TsC_ID_030sec);
	v_iter 	 = v_ts->cbegin();
	v_cnt	 = 10;	// we need only 10 samples
	v_iter	+= v_ts->getSize() - v_cnt;

	Serial.println();
	Serial.printf("TimeSeries buffer %s has %d items, will only get last %d\n", v_ts->getDescr(), v_ts->getSize(), v_cnt);

	Serial.println("Date/time\t\tdV\tmA\tW\tWh\tdHz\tpf");
	for (v_iter; v_iter != v_ts->cend(); ++v_iter) {
		std::time_t v_timestamp_lastupdate	= v_ts->getTstamp() - (v_ts->cend() - v_iter) * v_ts->getInterval();
		//char*		v_dtime_char 			= std::ctime(&v_timestamp_lastupdate);

		// struct tm v_tmstruct;
		// char v_dtime_char[30];

		localtime_r(&v_timestamp_lastupdate, &v_tmstruct);

		//getLocalTime(&tmstruct, 5000);
		sprintf(v_dtime_char,"%d-%02d-%02d %02d:%02d:%02d",
						(v_tmstruct.tm_year)+1900,
						(v_tmstruct.tm_mon)+1,
						v_tmstruct.tm_mday,
						v_tmstruct.tm_hour,
						v_tmstruct.tm_min,
						v_tmstruct.tm_sec
					);

		Serial.print(v_dtime_char);
		Serial.print("\t");
		Serial.print(v_iter->voltage);
		Serial.print("\t");
		Serial.print(v_iter->current);
		Serial.print("\t");
		Serial.print(v_iter->power);
		Serial.print("\t");
		Serial.print(v_iter->energy);
		//// Serial.print("\t");
		//// Serial.print(v_iter->freq);
		//// Serial.print("\t");
		//// Serial.print(v_iter->pf);
		Serial.println();
	}

	// Same for 5 min TimeSeries
	v_ts	 = g_B51_TsC_pz003.getTS(g_B51_TsC_ID_300sec);
	//v_ts	 = g_B51_TsC_pz004.getTS(g_B51_TsC_ID_030sec);
	v_iter	 = v_ts->cbegin();
	v_cnt	 = 10;											// we need only 10 samples
	v_iter	+= v_ts->getSize() - v_cnt;

	Serial.println();
	Serial.printf("TimeSeries buffer %s has %d items, will only get last %d\n", v_ts->getDescr(), v_ts->getSize(), v_cnt);

	Serial.println("Date/time\t\tdV\tmA\tW\tWh\tdHz\tpf");
	for (v_iter; v_iter != v_ts->cend(); ++v_iter) {
		std::time_t v_timestamp_lastupdate	= v_ts->getTstamp() - (v_ts->cend() - v_iter) * v_ts->getInterval();

		//char*		v_dtime_char 				= std::ctime(&v_timestamp_lastupdate);


		// struct tm v_tmstruct;
		// char v_dtime_char[30];

		localtime_r(&v_timestamp_lastupdate, &v_tmstruct);

		//getLocalTime(&tmstruct, 5000);
		sprintf(v_dtime_char,"%d-%02d-%02d %02d:%02d:%02d",
						(v_tmstruct.tm_year)+1900,
						(v_tmstruct.tm_mon)+1,
						v_tmstruct.tm_mday,
						v_tmstruct.tm_hour,
						v_tmstruct.tm_min,
						v_tmstruct.tm_sec
					);
		//Serial.println(String(v_dtime_char));


		Serial.print(v_dtime_char); 		Serial.print("\t");
		//Serial.print(v_dtime_char); 		Serial.print("\t");
		Serial.print(v_iter->voltage);		Serial.print("\t");
		Serial.print(v_iter->current);		Serial.print("\t");
		Serial.print(v_iter->power);		Serial.print("\t");
		Serial.print(v_iter->energy);		Serial.print("\t");
		//// Serial.print(v_iter->freq);			Serial.print("\t");
		//// Serial.print(v_iter->pf);			Serial.println();
		Serial.println();
	}

	// An example on how to export TS data in json format via WebServer pls see ESPEM Project https://github.com/vortigont/espem
};

// function is triggered by a timer each 5 sec
void B51_print_wait4data(void*) {
	static unsigned v_cnt = 60;

	Serial.print("Pls, wait, collecting data for ");
	Serial.print(v_cnt);
	Serial.println(" seconds more...");

	v_cnt -= 5;

	if (!v_cnt) {
		v_cnt = 60;
		// print our collected data
		B51_print_timeseries();
	}
};




void B51_WIFI_Connect(){


	Serial.println();
	Serial.print("[WiFi] Connecting to ");
	Serial.println(g_B51_ssid);

	WiFi.begin(g_B51_ssid, g_B51_password);

	// Will try for about 10 seconds (20x 500ms)
	int v_WIFI_tryDelay		 = 500;
	int v_WIFI_numberOfTries 	= 20;

	// Wait for the WiFi connection event
	while (true) {
		switch (WiFi.status()) {
			case WL_CONNECTED:
				Serial.println("[WiFi] WiFi is connected!");
				Serial.print("[WiFi] IP address: ");
				Serial.println(WiFi.localIP());
				return;
				break;
			default:
				Serial.print("[WiFi] WiFi Status: ");
				Serial.println(WiFi.status());
				break;
		}
		delay(v_WIFI_tryDelay);

		if (v_WIFI_numberOfTries <= 0) {
			Serial.print("[WiFi] Failed to connect to WiFi!");
			// Use disconnect function to force stop trying to connect
			WiFi.disconnect();
			return;
		} else {
			v_WIFI_numberOfTries--;
		}
	}
}


void B51_NTP_init(){

	// TimeSeries data needs a valid time source to function properly
	// so we will set an NTP server to sync with first
	// and a call-back function to get notification when time will be synchronized with NTP

	// configure TimeZone and ntp server
	configTzTime(g_B51_NTP_TIMEZONE, g_B51_NTP_ServerURL);


	// set notification call-back function
	// initializing TimeSeries buffer requires a timestamp sequence to start with.
	// We use unixtime AKA "epoch time" - the number of seconds that have elapsed since 00:00:00 UTC on 1 January 1970, the Unix epoch.
	// For this we need synchronize with NTP before we can set a starting point for TimeSeries.
	// So we postpone the configuration to the call-back function which will trigger once time synchronization from NTP is complete

	// 알림 콜백 기능 설정
	// TimeSeries 버퍼를 초기화하려면 시작하려면 타임스탬프 시퀀스가 ​​필요합니다.
	// 우리는 Unixtime AKA "epoch 시간"을 사용합니다. 이는 Unix epoch인 1970년 1월 1일 UTC 00:00:00 이후 경과된 초 수입니다.
	// 이를 위해 TimeSeries의 시작점을 설정하기 전에 NTP와 동기화해야 합니다.
	// 따라서 NTP로부터의 동기화가 완료되면 트리거되는 콜백 기능에 대한 구성을 연기합니다.

	sntp_set_time_sync_notification_cb(B51_NTP_sync_cb_timeseries);
}
void B51_ESP32_MEMORY_info_print(){
	// print memory stat
	Serial.printf("SRAM Heap total: %d, free Heap %d\n"				, ESP.getHeapSize(), ESP.getFreeHeap());
	Serial.printf("SPI-RAM heap total: %d, SPI-RAM free Heap %d\n"	, ESP.getPsramSize(), ESP.getFreePsram());

	Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n"	, ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
	Serial.printf("Flash Size %d, Flash Speed %d\n"					, ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
}




// #include <WiFi.h>
// #include <WiFiMulti.h>

// typedef struct {
// 		String		SSID;
// 		String		PWD;
// } B10_T2_WIFI_LOGIN_t;


// const uint8_t			g_W10_T2_Wifi_Login_Arr_Size 	= 5;

// B10_T2_WIFI_LOGIN_t		g_W10_T2_Wifi_Login_Arr[g_W10_T2_Wifi_Login_Arr_Size] = {
// 													  { .SSID = "GalNote10-2540"	, .PWD = "91932542"		}
// 													, { .SSID = "2G_1"				, .PWD = "65641077"		}
// 													, { .SSID = "LSITC_WIFI_AP"		, .PWD = "1234554321"	}
// 													, { .SSID = "U+Net83BF"			, .PWD = "010C020032"	}
// 													, { .SSID = "LGU+_M200_732675"	, .PWD = "55070016"		}
// 												};




// WiFiMulti   			g_W10_T2_WIfiMulti;
