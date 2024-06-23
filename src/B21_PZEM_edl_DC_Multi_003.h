// https://github.com/vortigont/pzem-edl

#include <Arduino.h>

#include "pzem_edl.hpp"

void B21_PZEM_rx_callback(uint8_t p_pzem_client_id, const RX_msg* p_pzem_rx_msg);

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
PZPool* 	g_B21_PZEM_Pools;


///////////////// PZEM Port

#define G_B21_PZEM_PORT1_UART 			UART_NUM_1	 		// port attached to pzem (UART_NUM_1 is 2-nd uart port on ESP32, 1-st one is usually busy with USB-to-serial chip)

#define G_B21_PZEM_PORT1_ID		 		10	 				// an ID for port instance
#define G_B21_PZEM_PORT1_DESC			"Phase_lines"	 	// an ID for port instance


#define G_B21_PZEM_PORT1_PIN_RX			22	 				// custom RX pin number
#define G_B21_PZEM_PORT1_PIN_TX			19	 				// custom TX pin number


void B21_PZEM_Port_init(){

	// now we must set UART port

	// for port object we need a config struct
	auto v_pzem_port1_cfg = UART_cfg(
									G_B21_PZEM_PORT1_UART,		// uart number
									G_B21_PZEM_PORT1_PIN_RX,		// rx pin remapped
									G_B21_PZEM_PORT1_PIN_TX			// tx pin remapped
								);

	// Ask PZPool object to create a PortQ object based on config provided
	// it will automatically start event queues for the port and makes it available for PZEM assignment
	if (g_B21_PZEM_Pools->addPort(
										G_B21_PZEM_PORT1_ID,	   		// some unique port id
										v_pzem_port1_cfg,	   			// uart config struct
										G_B21_PZEM_PORT1_DESC
									)  // mnemonic name for the port (optional)
	) {
		Serial.printf("Added PZEM port id:%d, name:%s\n", 			G_B21_PZEM_PORT1_ID, G_B21_PZEM_PORT1_DESC );
	} else {
		Serial.printf("ERR: Can't add PZEM port id:%d, name:%s\n", 	G_B21_PZEM_PORT1_ID, G_B21_PZEM_PORT1_DESC);
	}
}



typedef struct {
	uint8_t 	id;
	uint8_t 	addr;
	char		desc[10];
} T_B21_PZEM_Client_ST;



#define								G_B21_PZEM_CLIENT_CNT	3
static	T_B21_PZEM_Client_ST		g_B21_PZEM_Clients[G_B21_PZEM_CLIENT_CNT];


int B21_PZEM_Client_init(){
	////////////////////////// PZEM Client - Start

	g_B21_PZEM_Clients[0].id 				= 42;			// this is a unique PZEM ID just for reference, it is NOT slave MODBUS address, just some random id
															// (we all know why '42' is THE number, right? :) )
	g_B21_PZEM_Clients[0].addr 				= 10;			// those are PZEM modbus addresses,
															// must be programmed into PZEM in advance!
															// (use pzem_cli tool to do this - https://github.com/vortigont/pzem-edl/tree/main/examples/pzem_cli)
	strcpy(g_B21_PZEM_Clients[0].desc		, "Phase_1\0");

	g_B21_PZEM_Clients[1].id 				= 43;
	g_B21_PZEM_Clients[1].addr 				= 11;
	strcpy(g_B21_PZEM_Clients[1].desc		, "Phase_2\0");

	g_B21_PZEM_Clients[2].id 				= 44;
	g_B21_PZEM_Clients[2].addr 				= 12;
	strcpy(g_B21_PZEM_Clients[2].desc		, "Phase_3\0");


	// Now, we create PZEM instances one by one attaching it to the corresponding Port IDs
	// 	each PZEM instance must have:
	// 	- unique ID within a pool
	// 	- unique modbus address per port, different ports are allowed to have PZEM's with same address
	// 	- an existing port id to attach to


	// port_1 devs
	for (uint8_t i=0; i<G_B21_PZEM_CLIENT_CNT; i++ ){
		if (g_B21_PZEM_Pools->addPZEM(
												  G_B21_PZEM_PORT1_ID
												, g_B21_PZEM_Clients[i].id
												, g_B21_PZEM_Clients[i].addr
												, pzmodel_t::pzem004v3
												, g_B21_PZEM_Clients[i].desc
											)
												){
				Serial.printf("Added PZEM Client id:%d, name:%s, modbus addr:%d, port id:%d\n"
									, g_B21_PZEM_Clients[i].id
									, g_B21_PZEM_Clients[i].desc
									, g_B21_PZEM_Clients[i].addr
									, G_B21_PZEM_PORT1_UART
							);
			}
	}

	return 0;
	//printPerson(&arr[i]);
}



void B21_PZEM_get_Meters_single(){
	// now it is all ready to exchange data with PZEMs
	// check 'Single PZEM' example for detailed description

	// let's update metrics for all devs at once
	g_B21_PZEM_Pools->updateMetrics();

	// take some sleep while all devs are polled
	delay(200);	// for 200 ms

	// Let's check our 'Phase_1's power

	// obtain a reference to Metrics structure of a specific PZEM instance,
	// it is required to cast it to structure for the specific model

	for (uint8_t i=0; i<G_B21_PZEM_CLIENT_CNT; i++ ){
		const auto* v_PZEM_Meter = (const pz004::metrics*)g_B21_PZEM_Pools->getMetrics(g_B21_PZEM_Clients[i].id);

		if (v_PZEM_Meter) {	 // sanity check - make sure that a requested PZEM ID is valid and we have a real struct reference

			Serial.printf("Packet with metrics data------------\n");

			Serial.printf("PZEM Client ID:\t%d, \t Name: %s\n"		, g_B21_PZEM_Clients[i].id	, g_B21_PZEM_Pools->getDescr(g_B21_PZEM_Clients[i].id));

			Serial.printf("PZEM data has been updated %lld ms ago\n", g_B21_PZEM_Pools->getState(g_B21_PZEM_Clients[i].id)->dataAge() );

			Serial.printf("Voltage:\t%d dV\t~ %.1f volts\n"			, v_PZEM_Meter->voltage	, v_PZEM_Meter->asFloat(meter_t::vol));
			Serial.printf("Current:\t%u mA\t~ %.3f amperes\n"		, v_PZEM_Meter->current	, v_PZEM_Meter->asFloat(meter_t::cur));
			Serial.printf("Power:\t\t%u dW\t~ %.1f watts\n"			, v_PZEM_Meter->power	, v_PZEM_Meter->asFloat(meter_t::pwr));
			Serial.printf("Energy:\t\t%u Wh\t~ %.3f kWatt*hours\n"	, v_PZEM_Meter->energy	, v_PZEM_Meter->asFloat(meter_t::enrg)/1000 );
			Serial.printf("Frequency:\t%d dHz\t~ %.1f Herz\n"		, v_PZEM_Meter->freq	, v_PZEM_Meter->asFloat(meter_t::frq));
			Serial.printf("Power factor:\t%d/100\t~ %.2f\n"			, v_PZEM_Meter->pf		, v_PZEM_Meter->asFloat(meter_t::pf));
			Serial.printf("Power Alarm:\t%s\n"						, v_PZEM_Meter->alarm ? "Yes":"No");

		}
	}

}


int B21_PZEM_set_AutoPollConfig(uint8_t p_autopoll_enable, size_t p_autopoll_rate_ms){

	if (p_autopoll_enable == true) {
		if (g_B21_PZEM_Pools->autopoll(true)) {

			Serial.print("Autopolling enabled, ");

			Serial.print( "current Pollrate: ");
			Serial.print( g_B21_PZEM_Pools->getPollrate());
			Serial.print( " ms");
			Serial.println();

			g_B21_PZEM_Pools->setPollrate(p_autopoll_rate_ms);		// 1000ms

			Serial.print( "Changed Pollrate: ");
			Serial.print( g_B21_PZEM_Pools->getPollrate());
			Serial.print( " ms");
			Serial.println();

			//	just as an example so not to flood console let's change poll period to a lesser rate
			//	Normally you should not do this, better to always have fresh data and access it on demand

			// g_B21_PZEM_Pools->setPollrate(5000);  // 5 sec

			return 1;
		} else {

			return -1;
		}

	} else if (p_autopoll_enable == false){
		if (g_B21_PZEM_Pools->autopoll(false)) {

			Serial.print("Autopolling disabled ");
			return 1;
		} else {
			Serial.print("Autopolling disable Fail ");
			return -1;
		}
	}





	// let's assign our callback to PZPool instance.  I'm using lambda here to provide functional callback
	g_B21_PZEM_Pools->attach_rx_callback([](uint8_t p_pzem_client_id, const RX_msg* p_pzem_rx_msg) {
		// I can do all the required things here, but to keep it sepparate - let's just call our function
		B21_PZEM_rx_callback(p_pzem_client_id, p_pzem_rx_msg);
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

void	B21_Init() {
	// Serial.begin(115200);       // just an ordinary Serial console to interact with

	Serial.printf("\n\n\n\tPZEM multiple instance example\n\n");

	g_B21_PZEM_Pools		  = new PZPool();

	B21_PZEM_Port_init();

	B21_PZEM_Client_init();

	B21_PZEM_get_Meters_single();

	B21_PZEM_set_AutoPollConfig(true, 1000);


}

void B21_run() {

	vTaskDelete(NULL);
}



/**
 * @brief this is a custom callback for newly arrived data from PZEM
 *
 * @param id - this will be the ID of PZEM object, receiving the data
 * @param m - this will be the struct with PZEM data (not only metrics, but any one)
 */

void B21_PZEM_rx_callback(uint8_t p_pzem_client_id, const RX_msg* p_pzem_rx_msg) {

	// Here I can get the id of PZEM (might get handy if have more than one attached)
	Serial.printf(
					"\nTime: %ld / Heap: %d - Callback triggered for PZEM Client ID: %d, name: %s\n"
									, millis()
									, ESP.getFreeHeap()
									, p_pzem_client_id
									, g_B21_PZEM_Pools->getDescr(p_pzem_client_id)
				);



    //So now we have a notification that pzem device with ID 'id' has been updated, we can print (or send somewhere new data)

    if (g_B21_PZEM_Pools->getState(p_pzem_client_id)->dataStale()){
        return;   // something is wrong, message is either bad or not a data packet
	}

    auto s = (const pz004::state*)g_B21_PZEM_Pools->getState(p_pzem_client_id);

    Serial.printf("===\nPower alarm: %s\n"					, s->alarm ? "present" : "absent");

    Serial.printf("Voltage:\t%d dV\t~ %.1f volts\n"			, s->data.voltage, 	s->data.asFloat(pzmbus::meter_t::vol)		);
    Serial.printf("Current:\t%u mA\t~ %.3f amperes\n"		, s->data.current, 	s->data.asFloat(pzmbus::meter_t::cur)		);
    Serial.printf("Power:\t\t%u dW\t~ %.1f watts\n"			, s->data.power, 	s->data.asFloat(pzmbus::meter_t::pwr)		);
    Serial.printf("Energy:\t\t%u Wh\t~ %.3f kWatt*hours\n"	, s->data.energy, 	s->data.asFloat(pzmbus::meter_t::enrg)/1000 );
    Serial.printf("Frequency:\t%d dHz\t~ %.1f Herz\n"		, s->data.freq, 	s->data.asFloat(pzmbus::meter_t::frq)		);
    Serial.printf("Power factor:\t%d/100\t~ %.2f\n"			, s->data.pf, 		s->data.asFloat(pzmbus::meter_t::pf)		);


	// auto *m = (const pz004::metrics*)g_B21_PZEM_Pools->getMetrics(PZEM_ID_2)
	// Serial.printf("PZEM '%s' current as float: %.3f (Amps)\n", g_B21_PZEM_Pools->getDescr(PZEM_ID_2), m->asFloat(meter_t::cur));



	// It is also possible to work directly on a raw data from PZEM
	// let's call for a little help here and use a pretty_printer() function that parses and prints RX_msg to the stdout

	// since I gave only PZEM004 devices attached, I call pz004::rx_msg_prettyp() method
	pz004::rx_msg_prettyp(p_pzem_rx_msg);

}
