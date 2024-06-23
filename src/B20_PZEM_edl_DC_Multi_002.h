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

void B20_PZEM_rx_callback(uint8_t p_pzem_client_id, const RX_msg* p_pzem_rx_msg);
//void B20_PZEM_rx_callback(uint8_t id, const RX_msg* m);

using namespace pzmbus;	 // Use generic PZEM namespace


	// This is a small sketch that shows how to run multiple PZEM004 instances over a single serial port:

	// Pre Steps
	// 	- each PZEM device must be configured with a unique MODBUS address prior to attaching it to the shared serial lines.
	// 	- check 'pzem_cli' example for an easy way to read/modify PZEM address
	// 	- check 01_SinglePZEM example to get the idea of basic operations

	//  - create PZPool instance
	//  - allocate UART port to the pool
	//  - create PZEM objects
	//  - mannualy poll for data
	//  - enable auto-polling
	//  - use call-backs


	// 1. Connect several PZEM devices to ESP32's UART port using default or any other custom pins
	// 2. Build the sketch and use some terminal programm like platformio's devmon, putty or Arduino IDE to check for sketch output



// We'll need a placeholder for PZPool object
PZPool* 	g_B20_PZEM_DC_Meters;



///////////////// PZEM Port

#define G_B20_PZEM_PORT1_UART 			UART_NUM_1	 		// port attached to pzem (UART_NUM_1 is 2-nd uart port on ESP32, 1-st one is usually busy with USB-to-serial chip)

#define G_B20_PZEM_PORT1_ID		 		10	 				// an ID for port instance
#define G_B20_PZEM_PORT1_DESC			"Phase_lines"	 	// an ID for port instance


#define G_B20_PZEM_PORT1_PIN_RX			22	 			// custom RX pin number
#define G_B20_PZEM_PORT1_PIN_TX			19	 			// custom TX pin number


void B20_PZEM_Port_init(){

	// now we must set UART port

	// for port object we need a config struct
	auto v_pzem_port1_cfg = UART_cfg(
									G_B20_PZEM_PORT1_UART,		// uart number
									G_B20_PZEM_PORT1_PIN_RX,		// rx pin remapped
									G_B20_PZEM_PORT1_PIN_TX			// tx pin remapped
								);

	// Ask PZPool object to create a PortQ object based on config provided
	// it will automatically start event queues for the port and makes it available for PZEM assignment
	if (g_B20_PZEM_DC_Meters->addPort(
										G_B20_PZEM_PORT1_ID,	   		// some unique port id
										v_pzem_port1_cfg,	   	// uart config struct
										G_B20_PZEM_PORT1_DESC
									)  // mnemonic name for the port (optional)
	) {
		Serial.printf("Added PZEM port id:%d, name:%s\n", 			G_B20_PZEM_PORT1_ID, G_B20_PZEM_PORT1_DESC );
	} else {
		Serial.printf("ERR: Can't add PZEM port id:%d, name:%s\n", 	G_B20_PZEM_PORT1_ID, G_B20_PZEM_PORT1_DESC);
	}
}


////////////////////////// PZEM Client - Start

// #define PZEM_ID_1		 			42	 			// this is a unique PZEM ID just for reference, it is NOT slave MODBUS address, just some random id
// #define PZEM_ID_2		 			43	 				// (we all know why '42' is THE number, right? :) )
// #define PZEM_ID_3		 			44

// // those are PZEM modbus addresses, must be programmed into PZEM in advance! (use pzem_cli tool to do this - https://github.com/vortigont/pzem-edl/tree/main/examples/pzem_cli)
// #define PZEM_1_ADDR		 10
// #define PZEM_2_ADDR		 11
// #define PZEM_3_ADDR		 12

// #define PZEM_1_DESCR	 "Phase_1"
// #define PZEM_2_DESCR	 "Phase_2"
// #define PZEM_3_DESCR	 "Phase_3"



typedef struct {
	uint8_t 	id;
	uint8_t 	addr;
	char		desc[10];
} PZEM_Client_ST;



#define						G_B20_PZEM_CLIENT_CNT	3
static	PZEM_Client_ST		g_B20_PZEM_Clients[G_B20_PZEM_CLIENT_CNT];


int B20_PZEM_Client_init(){

	g_B20_PZEM_Clients[0].id 				= 42;
	g_B20_PZEM_Clients[0].addr 				= 10;
	strcpy(g_B20_PZEM_Clients[0].desc		, "Phase_1\0");

	g_B20_PZEM_Clients[1].id 				= 43;
	g_B20_PZEM_Clients[1].addr 				= 11;
	strcpy(g_B20_PZEM_Clients[1].desc		, "Phase_2\0");

	g_B20_PZEM_Clients[2].id 				= 44;
	g_B20_PZEM_Clients[2].addr 				= 12;
	strcpy(g_B20_PZEM_Clients[2].desc		, "Phase_3\0");


	/* Now, we create PZEM instances one by one attaching it to the corresponding Port IDs
		each PZEM instance must have:
		- unique ID within a pool
		- unique modbus address per port, different ports are allowed to have PZEM's with same address
		- an existing port id to attach to
	*/
	// port_1 devs

	for (uint8_t i=0; i<G_B20_PZEM_CLIENT_CNT; i++ ){
		if (g_B20_PZEM_DC_Meters->addPZEM(
												  G_B20_PZEM_PORT1_ID
												, g_B20_PZEM_Clients[i].id
												, g_B20_PZEM_Clients[i].addr
												, pzmodel_t::pzem004v3
												, g_B20_PZEM_Clients[i].desc
											)
												){
				Serial.printf("Added PZEM Client id:%d, name:%s, modbus addr:%d, port id:%d\n", g_B20_PZEM_Clients[i].id, g_B20_PZEM_Clients[i].desc, g_B20_PZEM_Clients[i].addr, G_B20_PZEM_PORT1_UART);
			}
	}
	// if (g_B20_PZEM_DC_Meters->addPZEM(
	// 									  G_B20_PZEM_PORT1_ID
	// 									, PZEM_ID_1
	// 									, PZEM_1_ADDR
	// 									, pzmodel_t::pzem004v3
	// 									, PZEM_1_DESCR
	// 								)
	// 									){
	// 	Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", PZEM_ID_1, PZEM_1_ADDR, G_B20_PZEM_PORT1_UART);
	// }

	// if (g_B20_PZEM_DC_Meters->addPZEM(G_B20_PZEM_PORT1_ID, PZEM_ID_2, PZEM_2_ADDR, pzmodel_t::pzem004v3, PZEM_2_DESCR)){
	// 	Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", PZEM_ID_2, PZEM_2_ADDR, G_B20_PZEM_PORT1_UART);
	// }

	// if (g_B20_PZEM_DC_Meters->addPZEM(G_B20_PZEM_PORT1_ID, PZEM_ID_3, PZEM_3_ADDR, pzmodel_t::pzem004v3, PZEM_3_DESCR)){
	// 	Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", PZEM_ID_3, PZEM_3_ADDR, G_B20_PZEM_PORT1_UART);
	// }

	return 0;
	//printPerson(&arr[i]);
}








void B20_PZEM_get_Meters_single(){
	// now it is all ready to exchange data with PZEMs
	// check 'Single PZEM' example for detailed description

	// let's update metrics for all devs at once
	g_B20_PZEM_DC_Meters->updateMetrics();

	// take some sleep while all devs are polled
	delay(200);	// for 200 ms

	// Let's check our 'Phase_1's power

	// obtain a reference to Metrics structure of a specific PZEM instance,
	// it is required to cast it to structure for the specific model

	for (uint8_t i=0; i<G_B20_PZEM_CLIENT_CNT; i++ ){
		const auto* v_PZEM_Meter = (const pz004::metrics*)g_B20_PZEM_DC_Meters->getMetrics(g_B20_PZEM_Clients[i].id);

		if (v_PZEM_Meter) {	 // sanity check - make sure that a requested PZEM ID is valid and we have a real struct reference
			// Serial.printf("Power value for '%s' is %f watts\n"
			// 												, g_B20_PZEM_DC_Meters->getDescr(g_B20_PZEM_Clients[i].id)
			// 												, v_PZEM_Meter->asFloat(meter_t::pwr)
			// 			);

			Serial.printf("Packet with metrics data------------\n");

			Serial.printf("PZEM Client ID:\t%d, \t Name: %s\n"		, g_B20_PZEM_Clients[i].id	, g_B20_PZEM_DC_Meters->getDescr(g_B20_PZEM_Clients[i].id));



			Serial.printf("PZEM data has been updated %lld ms ago\n", g_B20_PZEM_DC_Meters->getState(g_B20_PZEM_Clients[i].id)->dataAge() );
			//Serial.printf("PZEM data has been updated %lld ms ago\n", g_B20_PZEM_DC_Meters->getState(g_B20_PZEM_Clients[i].id)-> );


			Serial.printf("Voltage:\t%d dV\t~ %.1f volts\n"			, v_PZEM_Meter->voltage	, v_PZEM_Meter->asFloat(meter_t::vol));
			Serial.printf("Current:\t%u mA\t~ %.3f amperes\n"		, v_PZEM_Meter->current	, v_PZEM_Meter->asFloat(meter_t::cur));
			Serial.printf("Power:\t\t%u dW\t~ %.1f watts\n"			, v_PZEM_Meter->power	, v_PZEM_Meter->asFloat(meter_t::pwr));
			Serial.printf("Energy:\t\t%u Wh\t~ %.3f kWatt*hours\n"	, v_PZEM_Meter->energy	, v_PZEM_Meter->asFloat(meter_t::enrg)/1000 );
			Serial.printf("Frequency:\t%d dHz\t~ %.1f Herz\n"		, v_PZEM_Meter->freq	, v_PZEM_Meter->asFloat(meter_t::frq));
			Serial.printf("Power factor:\t%d/100\t~ %.2f\n"			, v_PZEM_Meter->pf		, v_PZEM_Meter->asFloat(meter_t::pf));
			Serial.printf("Power Alarm:\t%s\n"						, v_PZEM_Meter->alarm ? "Yes":"No");

			// Serial.printf("Voltage:\t%d dV\t~ %.1f volts\n", pz.data.voltage, pz.data.asFloat(meter_t::vol));
			// Serial.printf("Current:\t%u mA\t~ %.3f amperes\n", pz.data.current, pz.data.asFloat(meter_t::cur));
			// Serial.printf("Power:\t\t%u dW\t~ %.1f watts\n", pz.data.power, pz.data.asFloat(meter_t::pwr));
			// Serial.printf("Energy:\t\t%u Wh\t~ %.3f kWatt*hours\n", pz.data.energy, pz.data.asFloat(meter_t::enrg)/1000 );
			// Serial.printf("Frequency:\t%d dHz\t~ %.1f Herz\n", pz.data.freq, pz.data.asFloat(meter_t::frq));
			// Serial.printf("Power factor:\t%d/100\t~ %.2f\n", pz.data.pf, pz.data.asFloat(meter_t::pf));
			// Serial.printf("Power Alarm:\t%s\n", pz.data.alarm ? "Yes":"No");

		}




			// uint16_t voltage=0;
			// uint32_t current=0;
			// uint32_t power=0;
			// uint32_t energy=0;
			// uint16_t freq=0;
			// uint16_t pf=0;
			// uint16_t alarm=0;

	}

}


int B20_PZEM_set_AutoPollConfig(uint8_t p_autopoll_enable, size_t p_autopoll_rate_ms){

	if (p_autopoll_enable == true) {
		if (g_B20_PZEM_DC_Meters->autopoll(true)) {

			Serial.print("Autopolling enabled, ");

			Serial.print( "current Pollrate: ");
			Serial.print( g_B20_PZEM_DC_Meters->getPollrate());
			Serial.print( " ms");
			Serial.println();

			g_B20_PZEM_DC_Meters->setPollrate(p_autopoll_rate_ms);		// 1000ms

			Serial.print( "Changed Pollrate: ");
			Serial.print( g_B20_PZEM_DC_Meters->getPollrate());
			Serial.print( " ms");
			Serial.println();

			return 1;
		} else {

			return -1;
		}

	} else if (p_autopoll_enable == false){
		if (g_B20_PZEM_DC_Meters->autopoll(false)) {

			Serial.print("Autopolling disabled ");
			return 1;
		} else {
			Serial.print("Autopolling disable Fail ");
			return -1;
		}
	}

	// //    Run autopollig in background for all devs in pool
	// if (g_B20_PZEM_DC_Meters->autopoll(true)) {

	// 	Serial.print("Autopolling enabled, ");

	// 	Serial.print( "current Pollrate: ");
	// 	Serial.print( g_B20_PZEM_DC_Meters->getPollrate());
	// 	Serial.print( " ms");
	// 	Serial.println();

	// 	g_B20_PZEM_DC_Meters->setPollrate(1000);		// 1000ms

	// 	Serial.print( "Changed Pollrate: ");
	// 	Serial.print( g_B20_PZEM_DC_Meters->getPollrate());
	// 	Serial.print( " ms");
	// 	Serial.println();

	// 	//	just as an example so not to flood console let's change poll period to a lesser rate
	// 	//	Normally you should not do this, better to always have fresh data and access it on demand

	// 	// g_B20_PZEM_DC_Meters->setPollrate(5000);  // 5 sec


	// } else {
	// 	Serial.println("Sorry, can't autopoll somehow :(");
	// }


	// let's assign our callback to PZPool instance.  I'm using lambda here to provide functional callback

	g_B20_PZEM_DC_Meters->attach_rx_callback([](uint8_t p_pzem_client_id, const RX_msg* p_pzem_rx_msg) {
	//g_B20_PZEM_DC_Meters->attach_rx_callback([](uint8_t pzid, const RX_msg* m) {
		// I can do all the required things here, but to keep it sepparate - let's just call our function
		B20_PZEM_rx_callback(p_pzem_client_id, p_pzem_rx_msg);
	});



	// I do not need to do anything else,
	// I can just halt here in an endless loop, but every second with a new message
	// console will print metrics data from PZEMs
	for (;;) {
		delay(1000);
	}
}

//  Let's set a pool running one serial port and 3 PZEM devices
//  We will also give it some fancy names

void	B20_Init() {
	// Serial.begin(115200);       // just an ordinary Serial console to interact with

	Serial.printf("\n\n\n\tPZEM multiple instance example\n\n");

	// create a new PZPool object
	g_B20_PZEM_DC_Meters		  = new PZPool();


	B20_PZEM_Port_init();

	B20_PZEM_Client_init();


	B20_PZEM_get_Meters_single();


	B20_PZEM_set_AutoPollConfig(true, 1000);


}

// void B20_run() {
// 	// we do not need this loop at all :)
// 	vTaskDelete(NULL);
// }



/**
 * @brief this is a custom callback for newly arrived data from PZEM
 *
 * @param id - this will be the ID of PZEM object, receiving the data
 * @param m - this will be the struct with PZEM data (not only metrics, but any one)
 */

void B20_PZEM_rx_callback(uint8_t p_pzem_client_id, const RX_msg* p_pzem_rx_msg) {
	// Here I can get the id of PZEM (might get handy if have more than one attached)
	Serial.printf(
					"\nTime: %ld / Heap: %d - Callback triggered for PZEM Client ID: %d, name: %s\n"
									, millis()
									, ESP.getFreeHeap()
									, p_pzem_client_id
									, g_B20_PZEM_DC_Meters->getDescr(p_pzem_client_id)
				);

	/*
		Here it is possible to obtain a fresh new data same way as before, accessing metrics and state data via references

		auto *m = (const pz004::metrics*)g_B20_PZEM_DC_Meters->getMetrics(PZEM_ID_2)
		Serial.printf("PZEM '%s' current as float: %.3f (Amps)\n", g_B20_PZEM_DC_Meters->getDescr(PZEM_ID_2), m->asFloat(meter_t::cur));
	*/

	/*
		It is also possible to work directly on a raw data from PZEM
		let's call for a little help here and use a pretty_printer() function that parses and prints RX_msg to the stdout
	*/
	// since I gave only PZEM004 devices attached, I call pz004::rx_msg_prettyp() method
	pz004::rx_msg_prettyp(p_pzem_rx_msg);

	// that's the end of a callback
}
