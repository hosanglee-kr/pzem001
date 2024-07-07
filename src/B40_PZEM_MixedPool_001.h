
// This is a small sketch that shows how to run multiple PZEM instances over two serial ports:

// A setup consists of:
//  - 3 PZEM004 devices attached to UART_NUM_1 and monitoring AC lines
//  - 2 PZEM003 devices attached to UART_NUM_2 and monitoring DC lines

// Pre Steps
//     - each PZEM device must be configured with a unique MODBUS address prior to attaching it to the shared serial lines.
//     - check 'pzem_cli' example for an easy way to read/modify PZEM address
//     - check 01_SinglePZEM example to get the idea of basic operations

//  - create PZPool instance
//  - allocate UART port to the pool
//  - create PZEM objects
//  - mannualy poll for data
//  - enable auto-polling
//  - use call-backs

// 1. Connect several PZEM devices to ESP32's UART port(s) using default or any other custom pins
// 2. Build the sketch and use some terminal programm like platformio's devmon, putty or Arduino IDE to check for sketch output

#include <Arduino.h>

#include "pzem_edl.hpp"

void B40_PZPool_RX_callback(uint8_t id, const RX_msg *m);

using namespace pzmbus;	 // use general pzem abstractions

#define G_B40_PZEM_UART_PORT_AC 			UART_NUM_0	 		// UART_NUM_1 // port attached to pzem (UART_NUM_1 is 2-nd uart port on ESP32, 1-st one is usually busy with USB-to-serial chip)
#define G_B40_PZEM_UART_PORT_DC 			UART_NUM_1	 		// UART_NUM_2	// assume we use 2 ports

#define G_B40_PZEM_PORT_AC		 			10	 // an ID for port 1
#define G_B40_PZEM_PORT_DC		 			20	 // an ID for port 2

#define G_B40_PZEM_UART_PORT_AC_RX_PIN		22	 // custom RX pin number for port 1
#define G_B40_PZEM_UART_PORT_AC_TX_PIN		19	 // custom TX pin number for port 1


typedef struct {
	uint8_t 	id;
	uint8_t 	addr;
	char		desc[11];
} T_B40_PZEM_AC_ST;



#define							G_B40_PZEM_AC_CNT	3
static	T_B40_PZEM_AC_ST		g_40_PZEM_AC[G_B40_PZEM_AC_CNT]
											= {
												  {42, 10, "AC Phase_R"}
												, {43, 11, "AC Phase_S"}
												, {44, 12, "AC Phase_T"}
											};

// // those are IDs for AC lines
// #define G_B40_PZEM_AC_R_ID		 			42	 // this is a unique PZEM ID just for reference, it is NOT slave MODBUS address, just some random id
// #define G_B40_PZEM_AC_S_ID		 			43	 // (we all know why '42' is THE number, right? :) )
// #define G_B40_PZEM_AC_T_ID		 			44

// // those are PZEM004 modbus addresses, must be programmed into PZEM in advance! (use pzem_cli tool to do this)
// #define G_B40_PZEM_AC_R_ADDR		 		10
// #define G_B40_PZEM_AC_S_ADDR		 		11
// #define G_B40_PZEM_AC_T_ADDR		 		12

// #define G_B40_PZEM_AC_R_DESC				"AC Phase_R"
// #define G_B40_PZEM_AC_S_DESC				"AC Phase_S"
// #define G_B40_PZEM_AC_T_DESC				"AC Phase_T"


#define							G_B40_PZEM_DC_CNT	2
static	T_B40_PZEM_AC_ST		g_40_PZEM_DC[G_B40_PZEM_DC_CNT]
											= {
												  {50, 24, "DC Solar"}
												, {50, 25, "Accu's"}
											};


// // those are IDs for DC lines
// #define G_B40_PZEM_DC_1_ID		 			50
// #define G_B40_PZEM_DC_2_ID		 			51

// // those are PZEM003 modbus addresses, must be programmed into PZEM in advance! (use pzem_cli tool to do this)
// #define G_B40_PZEM_DC_1_ADDR		 		24
// #define G_B40_PZEM_DC_2_ADDR		 		25

// #define G_B40_PZEM_DC_1_DESC		 		"DCSolarPanel"
// #define G_B40_PZEM_DC_2_DESC		 		"Accu's"



//   Let's set a pool of two ports and 5 PZEM devices
//   Port one has 3 PZEM's for AC lines
//   Port two has 2 PZEM's for DC lines


PZPool *g_B40_PZPool_Meters;


void	B40_init() {

	// Serial.begin(115200);       // just an ordinary Serial console to interact with

	Serial.printf("\n\n\n\tPZEM multiple instance example\n\n");

	// create a new PZPool object
	g_B40_PZPool_Meters		  = new PZPool();

	// now we must set UART ports
	// for port object we need a config struct this describes port_1 for AC PZEMs
	auto v_pzem_ac_port_cfg = UART_cfg(
								G_B40_PZEM_UART_PORT_AC,							// uart number
								G_B40_PZEM_UART_PORT_AC_RX_PIN,						// rx pin remapped
								G_B40_PZEM_UART_PORT_AC_TX_PIN						// tx pin remapped
							);

	// Ask PZPool object to create a PortQ object based on config provided
	// it will automatically start event queues for the port and makes it available for PZEM assignment
	if (g_B40_PZPool_Meters->addPort(
										G_B40_PZEM_PORT_AC,
										v_pzem_ac_port_cfg,
										"Phase_lines"
										)
									) {
		Serial.printf("Added port id:%d\n", G_B40_PZEM_PORT_AC);
	} else {
		Serial.printf("ERR: Can't add port id:%d\n", G_B40_PZEM_PORT_AC);
	}


	//  Now, we create PZEM instances one by one attaching it to the corresponding Port IDs
	// 	each PZEM instance must have:
	// 	- unique ID within a pool
	// 	- unique modbus address per port, different ports are allowed to have PZEM's with same address
	// 	- an existing port id to attach to

	// port_1 devs - AC lines
	for (int i=1; i <= G_B40_PZEM_AC_CNT; i++){
		if (g_B40_PZPool_Meters->addPZEM(G_B40_PZEM_PORT_AC, g_40_PZEM_AC[i].id, g_40_PZEM_AC[i].addr, pzmodel_t::pzem004v3, g_40_PZEM_AC[i].desc)){
			Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", g_40_PZEM_AC[i].id, g_40_PZEM_AC[i].addr, g_40_PZEM_AC[i].desc);
		}
	}

	// port_1 devs - AC lines
	// if (g_B40_PZPool_Meters->addPZEM(G_B40_PZEM_PORT_AC, G_B40_PZEM_AC_R_ID, G_B40_PZEM_AC_R_ADDR, pzmodel_t::pzem004v3, G_B40_PZEM_AC_R_DESC)){
	// 	Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", G_B40_PZEM_AC_R_ID, G_B40_PZEM_AC_R_ADDR, G_B40_PZEM_UART_PORT_AC);
	// }
	// if (g_B40_PZPool_Meters->addPZEM(G_B40_PZEM_PORT_AC, G_B40_PZEM_AC_S_ID, G_B40_PZEM_AC_S_ADDR, pzmodel_t::pzem004v3, G_B40_PZEM_AC_S_DESC)){
	// 	Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", G_B40_PZEM_AC_S_ID, G_B40_PZEM_AC_S_ADDR, G_B40_PZEM_UART_PORT_AC);
	// }
	// if (g_B40_PZPool_Meters->addPZEM(G_B40_PZEM_PORT_AC, G_B40_PZEM_AC_T_ID, G_B40_PZEM_AC_T_ADDR, pzmodel_t::pzem004v3, G_B40_PZEM_AC_T_DESC)){
	// 	Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", G_B40_PZEM_AC_T_ID, G_B40_PZEM_AC_T_ADDR, G_B40_PZEM_UART_PORT_AC);
	// }




	// and another port for attaching to PZEM003 DC lines
	auto v_pzem_dc_port_cfg	= UART_cfg(
								G_B40_PZEM_UART_PORT_DC,
								UART_PIN_NO_CHANGE,		// using default pins, no remapping
								UART_PIN_NO_CHANGE	// using default pins, no remapping
							);

	// PZEM003 requires custom config for serial port
	v_pzem_dc_port_cfg.uartcfg.stop_bits = UART_STOP_BITS_2;	// PZEM003 needs 2 stop bits

	// Ask PZPool object to create a PortQ object based on config provided
	// it will automatically start event queues for the port and makes it available for PZEM assignment
	if (g_B40_PZPool_Meters->addPort(G_B40_PZEM_PORT_DC, v_pzem_dc_port_cfg, "DC_lines")) {
		Serial.printf("Added port id:%d\n", G_B40_PZEM_PORT_DC);
	} else {
		Serial.printf("ERR: Can't add port id:%d\n", G_B40_PZEM_PORT_DC);
	}


	// port_2 devs - DC lines
	for (int i=1; i <= G_B40_PZEM_DC_CNT; i++){
		if (g_B40_PZPool_Meters->addPZEM(G_B40_PZEM_PORT_DC, g_40_PZEM_DC[i].id, g_40_PZEM_DC[i].addr, pzmodel_t::pzem003, g_40_PZEM_DC[i].desc)){
			Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", g_40_PZEM_DC[i].id, g_40_PZEM_DC[i].addr, g_40_PZEM_DC[i].desc);
		}
	}

	// if (g_B40_PZPool_Meters->addPZEM(G_B40_PZEM_PORT_DC, G_B40_PZEM_DC_1_ID, G_B40_PZEM_DC_1_ADDR, pzmodel_t::pzem003, G_B40_PZEM_DC_1_DESC)){
	// 	Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", G_B40_PZEM_DC_1_ID, G_B40_PZEM_DC_1_ADDR, G_B40_PZEM_UART_PORT_DC);
	// }

	// if (g_B40_PZPool_Meters->addPZEM(G_B40_PZEM_PORT_DC, G_B40_PZEM_DC_2_ID, G_B40_PZEM_DC_2_ADDR, pzmodel_t::pzem003, G_B40_PZEM_DC_1_DESC)){
	// 	Serial.printf("Added PZEM id:%d addr:%d, port id:%d\n", G_B40_PZEM_DC_2_ID, G_B40_PZEM_DC_2_ADDR, G_B40_PZEM_UART_PORT_DC);
	// }

	// now it is all ready to exchange data with PZEMs
	// check 'Single PZEM' example for detailed description

	// let's update metrics for all devs
	g_B40_PZPool_Meters->updateMetrics();

	// take some sleep while all devs are polled
	delay(200);	// for 200 ms

	// Let's check our 'Phase_1's power

	// obtain a reference to Metrics structure of a specific PZEM instance,
	// it is reuired to cast it to structure for the specific model
	const auto *v_m1 = (const pz004::metrics *)g_B40_PZPool_Meters->getMetrics(g_40_PZEM_AC[0].id);
	//const auto *v_m1 = (const pz004::metrics *)g_B40_PZPool_Meters->getMetrics(G_B40_PZEM_AC_R_ID);

	if (v_m1) {  // sanity check - make sure that a requested PZEM ID is valid and we have a real struct reference
		Serial.printf("Power value for '%s' is %f watts\n", g_B40_PZPool_Meters->getDescr(g_40_PZEM_AC[0].id), v_m1->asFloat(meter_t::pwr));
		//Serial.printf("Power value for '%s' is %f watts\n", g_B40_PZPool_Meters->getDescr(G_B40_PZEM_AC_R_ID), v_m1->asFloat(meter_t::pwr));
	}

	// Let's check our solar panels voltage

	// obtain a reference to Metrics structure of a specific PZEM instance,
	// it is reuired to cast it to structure for the specific model
	const auto *v_m4 = (const pz003::metrics *)g_B40_PZPool_Meters->getMetrics(g_40_PZEM_DC[0].id);
	//const auto *v_m4 = (const pz003::metrics *)g_B40_PZPool_Meters->getMetrics(G_B40_PZEM_DC_1_ID);

	if (v_m4) {  // sanity check - make sure that a requested PZEM ID is valid and we have a real struct reference
		Serial.printf("Voltage for '%s' is %d volts\n", g_B40_PZPool_Meters->getDescr(g_40_PZEM_DC[0].id), v_m4->voltage);
	}

	//    Run autopollig in background for all devs in pool
	if (g_B40_PZPool_Meters->autopoll(true)) {
		Serial.println("Autopolling enabled");
	} else {
		Serial.println("Sorry, can't autopoll somehow :(");
	}

	// let's assign our callback to the PZPool instance.
	// I'm using lambda here to provide functional callback
	g_B40_PZPool_Meters->attach_rx_callback([](uint8_t pzid, const RX_msg *m) {
		// I can do all the required things here, but to keep it sepparate -
		// let's just call our function
		B40_PZPool_RX_callback(pzid, m);
	});

	/*
		just as an example so not to flood console let's change poll period to a lesser rate
		Normally you should not do this, better to always have fresh data and access it on demand
	*/
	g_B40_PZPool_Meters->setPollrate(5000);  // 5 sec

	// now I do not need to do anything
	// I can just halt here in an endless loop, but every second with a new message
	// console will print metrics data from PZEM
	for (;;) {
		delay(1000);
	}
}

void B40_run() {
	// we do not need this loop at all :)
	// might even kill it's task
	vTaskDelete(NULL);
}

/**
 * @brief this is a custom callback for newly arrived data from PZEM
 *
 * @param id - this will be the ID of PZEM object, receiving the data
 * @param m - this will be the struct with PZEM data (not only metrics, but any one)
 */
void B40_PZPool_RX_callback(uint8_t id, const RX_msg *m) {
	Serial.printf("\nTime: %ld / Heap: %d - Callback triggered for PZEM ID: %d, name: %s\n", millis(), ESP.getFreeHeap(), id, g_B40_PZPool_Meters->getDescr(id));

	/*
		Since we have mexed pool of PZEM devies, we need to find out wich device in particular we've got this message from,
		than we can either use the packet data directly or access metrics struct for the specific PZEM instance
	*/
	switch (g_B40_PZPool_Meters->getState(id)->model) {
		case pzmodel_t::pzem004v3: {
			pz004::rx_msg_prettyp(m);  // parse incoming message and print some nice info

			// or we can access struct data for the updated object (an example)
			auto *s = (const pz004::state *)g_B40_PZPool_Meters->getState(id);
			Serial.printf("===\nPower alarm: %s\n", s->alarm ? "present" : "absent");
			Serial.printf("Power factor: %d\n", s->data.pf);
			Serial.printf("Current value: %f\n", s->data.asFloat(meter_t::cur));
			break;
		}
		case pzmodel_t::pzem003: {
			pz003::rx_msg_prettyp(m);  // parse incoming message and print some nice info

			// or we can access struct data for the updated object
			auto *s = (const pz003::state *)g_B40_PZPool_Meters->getState(id);
			Serial.printf("===\nPower high alarm: %s\n", s->alarmh ? "present" : "absent");
			Serial.printf("Power low alarm: %s\n", s->alarml ? "present" : "absent");
			Serial.printf("Energy: %d\n", s->data.energy);
			Serial.printf("Current value: %f\n", s->data.asFloat(meter_t::cur));
			break;
		}
		default:
			break;
	}

	// that's the end of a callback
}
