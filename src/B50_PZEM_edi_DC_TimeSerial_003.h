/*
PZEM EDL - PZEM Event Driven Library

This code implements communication and data exchange with PZEM004T V3.0 module using MODBUS proto
and provides an API for energy metrics monitoring and data processing.

This file is part of the 'PZEM event-driven library' project.

Copyright (C) Emil Muratov, 2021
GitHub: https://github.com/vortigont/pzem-edl
*/

#include <Arduino.h>

#include "pzem_edl.hpp"
#include "B55_PZEM003Dummy.hpp"

#include "timeseries.hpp"


void B50_PZEM_rx_callback(uint8_t p_PZEM_id, const RX_msg* p_PZEM_RX_msg);
//void mycallback(uint8_t id, const RX_msg *m);

#include <esp32/himem.h>
#include <esp32/spiram.h>



//	This example shows how to collect TimeSeries data for PZEM metrics
//	Pls, check previous examples for basic operations



using namespace pz003;			   									// we will need this namespace for PZEM004v3.0 device
//using namespace pz004;			   								// we will need this namespace for PZEM004v3.0 device

#define 	G_B50_PZEM_PORT1_UART 			UART_NUM_1  			// port attached to pzem (UART_NUM_1 is 2-nd uart port on ESP32, 1-st one is usually busy with USB-to-serial chip)

#define 	G_B50_PZEM_PORT1_PIN_RX		   	22  					// custom RX pin number
#define 	G_B50_PZEM_PORT1_PIN_TX		   	19  					// custom TX pin number

#define 	G_B50_PZEM_PORT1_ID		   		42  					// this is a unique PZEM ID for reference, it is NOT slave MODBUS address, just some random id
						   											// (we all know why '42' is THE number, right? :) )


UartQ*		g_B50_UartQueue;										// first, we need a UartQ object to handle RX/TX message queues


PZ003*		g_B50_PZ003;											// Also we need a placeholder for PZEM object
PZ004*		g_B50_PZ004;

PZ004*      g_B50_PZ004_Dummy = nullptr;
PZ003*      g_B50_PZ003_Dummy = nullptr;


void B50_MEMORY_Print(){

	Serial.printf("\nspiram size %u\n"								, esp_spiram_get_size());
	Serial.printf("himem free %u\n"									, esp_himem_get_free_size());
	Serial.printf("himem phys %u\n"									, esp_himem_get_phys_size());
	Serial.printf("himem reserved %u\n"								, esp_himem_reserved_area_size());


	// print memory stat
	Serial.printf("SRAM Heap total: %d, free Heap %d\n"				, ESP.getHeapSize(), ESP.getFreeHeap());
	Serial.printf("SPI-RAM heap total: %d, SPI-RAM free Heap %d\n"	, ESP.getPsramSize(), ESP.getFreePsram());
	Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n"	, ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
	Serial.printf("Flash Size %d, Flash Speed %d\n"					, ESP.getFlashChipSize(), ESP.getFlashChipSpeed());


}

void B50_PZEM_get_Metrics_PZ003_single(){
	// and try to check the voltage value

	auto *m = g_B50_PZ003->getMetricsPZ003();		// obtain a pointer to objects metrics
	//auto *m = g_B50_PZ004->getMetricsPZ004();		// obtain a pointer to objects metrics

	// now we should have some response with updated values, let's check again
	Serial.printf("PZEM voltage: %d (decivolts)\n", m->voltage);

	// and find how long ago we had a reply came back while we were sleeping
	Serial.printf("PZEM data has been updated %lld ms ago\n", g_B50_PZ003->getState()->dataAge());

	// let's check other metrics
	Serial.printf("PZEM current: %u (mA)\n", m->current);

	// Or if someone likes floats instead? (I don't)
	Serial.printf("PZEM current as float: %.3f (Amps)\n", m->asFloat(pzmbus::meter_t::cur));	// allowed arguments are in enum meter_t:uint8_t { vol, cur, pwr, enrg, frq, pf, alrm }

}


void B50_PZEM_Test1(TSContainer<pz003::metrics>* p_ts_Container, uint8_t p_ts_Container_id){

	// 전부 안되는 것 같음

	//	Now we can do all kind of other stuff and anytime we ask for values from PZEM object it will give us some fresh data
	//	(default poll time is 1 second)

	//	Let's make a simple task - we will sleep for random time from 0 to 5 seconds and on wake we will check PZEM for new data
	

	/*
		int times = 5;
		do {
			long t = random(5000);
			Serial.printf("Going to sleep for %ld ms\n", t);
			delay(t);

			Serial.println("Wake up!");

			Serial.printf("PZEM voltage: %d (decivolts), last update time %lld ms ago\n\n", m->voltage, g_B50_PZ003->getState()->dataAge());
		} while(--times);
	*/

	
	// Serial.println("Release sampler");
	
	// //g_B50_PZ003->ts = new TSNode<pz004::metrics>(512);
	// //auto t = g_B50_PZ003->ts;
	// g_B50_PZ003->ts = nullptr;
	// delete t;

	// Serial.printf("Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
	// Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
	

	/*
		TimeSeries<pz004::metrics>ts(1, 30, 0);
		pz004::metrics mt;

			int x = 10;
			Serial.println("Fill ring buff");

			for (int i = 1; i != 31; ++i){
				mt.voltage = x++;
				ts.push(mt, i);
			}
	*/

	/*
		auto chain = p_ts_Container.getTSchain();
		const auto ts = chain->get(2);
	//
			Serial.println("mod ring buff");
			for (auto j = ts->begin(); j != ts->end(); ++j){
				j->voltage += 10;
			}
	//
	*/
}


void B50_PZEM_Test2(TSContainer<pz003::metrics>* p_ts_Container, uint8_t p_ts_Container_id){
		// Print timeseries for 1 2 30 sec
		Serial.println("Print fwd ring buff");

		auto ts = p_ts_Container->getTS(p_ts_Container_id);

		for (auto _i = ts->cbegin(); _i != ts->cend(); ++_i) {
			const auto &d = _i;
			Serial.printf("PZEM voltage, cur, pwr: %d\t%d\t%d\t%d\t%d\n", d->voltage, d->current, d->power, _i->energy, 0);
		}

		//Serial.printf("Data count: %d\n", ts->data.get() .size());

		//Serial.printf("Data count: %d\n", g_B50_PZ003->ts->data.size());



}

void B50_PZEM_Test3(TSContainer<pz003::metrics>* p_ts_Container, uint8_t p_ts_Container_id){

	auto ts = p_ts_Container->getTS(p_ts_Container_id);

	//for (const auto& d : g_B50_PZ003->ts){

	for (auto _i = ts->cbegin(); _i != ts->cend(); ++_i){
	//for (auto _i = g_B50_PZ003->ts->cbegin(); _i != g_B50_PZ003->ts->cend(); ++_i){	
	//for (const auto& _i = g_B50_PZ003->ts->cbegin(); _i != g_B50_PZ003->ts->cend(); ++_i){
		const auto &d = _i;
		auto x = *_i;
	//for (int i = 0; i != g_B50_PZ003->ts->size; ++i){
		//auto d = g_B50_PZ003->ts->get()[i];
		//d->voltage = 100;
		Serial.printf("PZEM voltage, cur, pwr: %d\t%d\t%d\t%d\n"
		//Serial.printf("PZEM voltage, cur, pwr: %d\t%d\t%d\t%d\t%d\n"
																	, d->voltage
																	, d->current
																	, d->power
																	, _i->energy
																	// , x.freq
																	);
		//Serial.printf("PZEM voltage, cur, pwr: %d\t%d\t%d\n", d[i].voltage, d[i].current, d[i].power);
	}

	

	// test iterators



	for (auto _i = ts->cbegin(); _i != ts->cend(); ++_i){
		const auto &d = _i;
		//const auto x = _i->;
		Serial.printf("PZEM voltage, cur, pwr: %d\t%d\t%d\t%d\n", d->voltage, d->current, d->power, _i->energy);
		//auto x = &_i;

	}

	///auto ts = p_ts_Container->getTS(p_ts_Container_id);

	const TimeSeries<pz003::metrics>* ts2 = p_ts_Container->getTS(p_ts_Container_id);
	
	// // No iterator
	// for (int i = 0; i != p_ts_Container->getTSsize(); ++i){ //} -> ->ts->size; ++i){
	// 	auto d = g_B50_PZ003->ts->get()[i];

	// 	// auto d = g_B50_PZ003->ts->get()[i];
	// 	//d->voltage = 100;
	// 	Serial.printf("PZEM voltage, cur, pwr: %d\t%d\t%d\n", d.voltage, d.current, d.power);
	// 	//Serial.printf("PZEM voltage, cur, pwr: %d\t%d\t%d\n", d[i].voltage, d[i].current, d[i].power);
	// }
	
}

void B50_PZEM_Init() {
	Serial.printf("\n\n\n\tPZEM004 TimeSeries example\n\n");

	B50_MEMORY_Print();

	// OR we can map port to any custom pins
	g_B50_UartQueue = new UartQ(G_B50_PZEM_PORT1_UART, G_B50_PZEM_PORT1_PIN_RX, G_B50_PZEM_PORT1_PIN_TX);  // or use custom pins


	// modbus_addr Slave addressing
		// #define ADDR_BCAST              0x00    // 0		// broadcast address    (slaves are not supposed to answer here)
		// #define ADDR_MIN                0x01    // 1		// lowest slave address
		// #define ADDR_MAX                0xF7    // 247	// highest slave address
		// #define ADDR_ANY                0xF8    // 248 	// default catch-all address

	g_B50_PZ003	= new PZ003(
								  G_B50_PZEM_PORT1_ID
								, ADDR_ANY
								,"PZEM003-02"			//char[9]);   // i.e. PZEM-123
							);		

    g_B50_PZ004_Dummy = new DummyPZ004(G_B50_PZEM_PORT1_ID, ADDR_ANY);
    g_B50_PZ003_Dummy = new DummyPZ003(G_B50_PZEM_PORT1_ID, ADDR_ANY);
	
    // first run
    //#ifdef ESPEM_DUMMY
    //  pz = new DummyPZ004(PZEM_ID, ADDR_ANY);
    //#else
    //  pz = new PZ004(PZEM_ID, ADDR_ANY);
    //#endif
	
	//g_B50_PZ004	= new PZ004(G_B50_PZEM_PORT1_ID);

	Serial.printf("PZEM General Info: n");
	Serial.printf("\t PZEM id = %d, modbus_addr = %d, Desc = %s \n"
									, g_B50_PZ003->id
									, g_B50_PZ003->getaddr()
									, g_B50_PZ003->getDescr()
									);

	// and link our port with PZEM object
	
	g_B50_PZ003->attachMsgQ(g_B50_UartQueue);
	//g_B50_PZ004->attachMsgQ(g_B50_UartQueue);


	// one last step - we must start PortQ tasks to handle messages
	g_B50_UartQueue->startQueues();

	// now it is all ready to exchange data with PZEM.  let's update it's metrics
	g_B50_PZ003->updateMetrics();
	//g_B50_PZ004->updateMetrics();

	Serial.println("===");

	B50_MEMORY_Print();



	Serial.println("\nAllocate sampler buffer");

	// Create Container object for TimeSeries buffers
	TSContainer<pz003::metrics> v_ts_Container;
	//TSContainer<pz004::metrics> v_ts_Container;
	

	
	// this will create TS object that holds per-second metrics data total 60 samples. 
	// Each sample takes about 20 bytes of (SPI)-RAM, It not a problem to stora thouthands if you have SPI-RAM
	// 그러면 초당 측정항목 데이터 총 60개 샘플을 보유하는 TS 개체가 생성됩니다.
	// 각 샘플은 약 20바이트의 (SPI)-RAM을 사용하며, SPI-RAM이 있으면 저장하는데 문제가 되지 않습니다.
	 
	// (esp_timer_get_time() >> 20) is a timestamp marking starting point . actually it is just ~seconds from boot
	 

	// uint8_t TSContainer<T>::addTS(size_t s, uint32_t start_time, uint32_t period = 1, const char *descr = nullptr, uint8_t id = 0);

	uint8_t v_ts_id_01sec = v_ts_Container.addTS(	
								  60								// size_t 		SampleCount 
								, esp_timer_get_time() >> 20		// uint32_t 	start_time
								, 1U								// uint32_t 	period 		= 1
								, nullptr							// char *		descr 		= nullptr
								, 0									// uint8_t 		id 			= 0
							);	// each

	Serial.printf("Add per-second TimeSeries, id: %d\n", v_ts_id_01sec);

	
	 // the same for 5-seconds interval, 60 samples totals no averaging is done, just saving probes every 5 seconds
	 // 5초 간격에도 동일, 총 60개 샘플 평균화는 수행되지 않고 5초마다 프로브를 저장합니다.
	 
	uint8_t v_ts_id_05sec = v_ts_Container.addTS(
								  60
								, esp_timer_get_time() >> 20
								, 5
							);

	Serial.printf("Add per-second TimeSeries, id: %d\n", v_ts_id_05sec);

	
	// the same for 30-seconds interval, 100 samples totals no averaging is done, just saving probes every 5 seconds
	// 30초 간격에도 동일, 총 100개 샘플 평균화는 수행되지 않고 5초마다 프로브만 저장됩니다.
	

	uint8_t v_ts_id_30sec = v_ts_Container.addTS(
								  100
								, esp_timer_get_time() >> 20
								, 30
							);
	Serial.printf("Add per-second TimeSeries, id: %d\n", v_ts_id_30sec);

	// check memry usage, if SPI-RAM is available on board, than TS will allocate buffer in SPI-RAM
	Serial.println();
	Serial.printf("SRAM Heap total: %d, free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
	Serial.printf("SPI-RAM heap total: %d, SPI-RAM free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());



	
	// Now I need to hookup to the PZEM object, it autopolls every second so I can use it's callback to collect metrics data to TSContainer
	// 이제 PZEM 객체에 연결해야 합니다. PZEM 객체는 매초 자동 폴링되므로 콜백을 사용하여 TSContainer에 대한 메트릭 데이터를 수집할 수 있습니다.
	
	auto v_ts_Container_ref = &v_ts_Container;			// a v_ts_Container_ref of our Container to feed it the labda function

	// g_B50_PZ003->attach_rx_callback([v_ts_Container_ref](uint8_t p_PZEM_id, const RX_msg* p_PZEM_RX_msg) {
	// 	B50_PZEM_rx_callback(p_PZEM_id, p_PZEM_RX_msg);
	// });

	g_B50_PZ003->attach_rx_callback([v_ts_Container_ref](uint8_t pzid, const RX_msg *m) {
		auto *data = g_B50_PZ003->getMetricsPZ003();			 							// obtain a pointer to objects metrics
		v_ts_Container_ref->push(*data, esp_timer_get_time() >> 20);	 					// push data to TS container and mark it with "seconds' timer 데이터를 TS 컨테이너에 푸시하고 '초' 타이머로 표시합니다.
	});

	// g_B50_PZ004->attach_rx_callback([v_ts_Container_ref](uint8_t pzid, const RX_msg *m) {
	// 		auto *data = g_B50_PZ004->getMetricsPZ004();			 	// obtain a pointer to objects metrics
	// 		v_ts_Container_ref->push(*data, esp_timer_get_time() >> 20);	 	// push data to TS container and mark it with "seconds' timer 데이터를 TS 컨테이너에 푸시하고 '초' 타이머로 표시합니다.
	// });


	// OK, 1000 ms is too long to print garbage, let's just take some sleep
	delay(2000);	// for 200 ms


	B50_PZEM_get_Metrics_PZ003_single();



	if (g_B50_PZ003->autopoll(true)) {
		Serial.println("Autopolling enabled");
	} else {
		Serial.println("Sorry, can't autopoll somehow :(");
	}


	B50_PZEM_Test1(&v_ts_Container, v_ts_id_01sec );
	




	// now I do not need to do anything
	// I can just halt here in an endless loop, but every second with a new message console will print metrics data from PZEM
	// 이제 난 아무것도 할 필요가 없어
    // 여기서 무한 루프를 멈출 수 있지만 새 메시지 콘솔을 사용하면 매초마다 PZEM의 메트릭 데이터가 인쇄됩니다.
	for (;;) {
		delay(5000);

		B50_PZEM_Test2(&v_ts_Container, v_ts_id_01sec );

		B50_PZEM_Test2(&v_ts_Container, v_ts_id_05sec );



		B50_PZEM_Test3(&v_ts_Container, v_ts_id_05sec );
		

		
	}
}

void B50_PZEM_run() {
	// we do not need this loop at all :)
	for (;;) {
		delay(1000);
	}
}

/**
 * @brief this is a custom callback for newly arrived data from PZEM
 *
 * @param id - this will be the ID of PZEM object, receiving the data
 * @param m - this will be the struct with PZEM data (not only metrics, but any one)
 */

void B50_PZEM_rx_callback(uint8_t p_PZEM_id, const RX_msg* p_PZEM_RX_msg){
//void mycallback(uint8_t id, const RX_msg *m) {
	
	
	
	//g_B50_PZ003->attach_rx_callback([v_ts_Container_ref](uint8_t pzid, const RX_msg *m) {
	auto *data = g_B50_PZ003->getMetricsPZ003();			 		// obtain a pointer to objects metrics
	//auto *data = g_B50_PZ003->getMetricsPZ004();			 	// obtain a pointer to objects metrics
		
	//////	v_ts_Container_ref->push(*data, esp_timer_get_time() >> 20);	 	// push data to TS container and mark it with "seconds' timer 데이터를 TS 컨테이너에 푸시하고 '초' 타이머로 표시합니다.
	//});

	
	// I can get the id of PZEM (might get handy if have more than one attached)
	Serial.printf("\nCallback triggered for PZEM ID: %d\n", p_PZEM_id);

	/*
		It is possible to obtain a fresh new data same way as before

		Serial.printf("PZEM current as float: %.3f (Amps)\n", g_B50_PZ003->getMetricsPZ004()->asFloat(pzmbus::meter_t::cur));
	*/

	/*
		It is also possible to work directly on a raw data from PZEM
		let's call for a little help here and use a pretty_printer() function
		that parses and prints RX_msg to the stdout
	*/
	rx_msg_prettyp(p_PZEM_RX_msg);

	// that's the end of a callback
}
