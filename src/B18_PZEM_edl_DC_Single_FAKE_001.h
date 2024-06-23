
// The 'C:\Users\2540\.vscode\extensions\ms-vscode.cpptools-1.16.3-win32-x64\LLVM\bin\clang-format.exe' command is not available.  Please check your clang-format.executable user setting and ensure it is installed.

/*
PZEM EDL - PZEM Event Driven Library

This code implements communication and data exchange with PZEM004T V3.0 module using MODBUS proto
and provides an API for energy metrics monitoring and data processing.

This file is part of the 'PZEM event-driven library' project.

Copyright (C) Emil Muratov, 2021
GitHub: https://github.com/vortigont/pzem-edl
*/

// pzem-edl > examples > 02_SinglePZEM003

#include <Arduino.h>

#include "pzem_edl.hpp"

#include "pzem_edl_fake_PZ003_001.hpp"

void B18_PZEM_rx_callback(uint8_t p_PZEM_id, const RX_msg* p_PZEM_RX_msg);

//   will need this namespace's methods to work with PZEM-003 (same as PZEM-017) DC 10/50/100/200/300 Amps meter
//   NOTE: I do not have any of PZEM003 DC devies to make real test-case scenarios,
//   I only imlemented MODBUS RTU proto according to specs.
//   Pls, provide any feedback

using namespace pz003;

// This is a small sketch that shows how to run a single PZEM003 instance:

//  - create an event-driven UART Queue object
//  - create PZ003 instance
//  - attach UART Queue to the PZEM instance
//  - mannualy poll for data
//  - enable auto-polling
//  - use call-backs

// 1. Connect only ONE device to ESP32's UART port using default or any other custom pins
// 2. Build the sketch and use terminal programm like platformio's devmon, putty or Arduino IDE to check for sketch output

#define B18_PZEM_UART_PORT UART_NUM_1  // port attached to pzem (UART_NUM_1 is 2-nd uart port on ESP32, 1-st one is usually busy with USB-to-serial chip)

#define RX_PIN			   22  // custom RX pin number
#define TX_PIN			   19  // custom TX pin number

#define B18_PZEM_ID		   42  // this is a unique PZEM ID for reference, it is NOT slave MODBUS address, just some random id (we all know why '42' is THE number, right? :) )

UartQ* g_B18_UartQueue;	 // first, we need a UartQ object to handle RX/TX message queues

DummyPZ003* g_B18_PZ003;	 // Also we need a placeholder for PZEM object


// g_B18_PZ003->resetEnergyCounter() 


void   B18_Init() {

	
	Serial.printf("\n\n\n\tPZEM003 single instance example\n\n");

	// PZEM003 requires custom config for serial port
	uart_config_t v_uart_cfg = {
		.baud_rate = PZEM_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity	 = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_2,  // PZEM003 need 2 stop bits
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

	// we can map port to any custom pins and use our config for serial port setup
	g_B18_UartQueue = new UartQ(B18_PZEM_UART_PORT, v_uart_cfg, RX_PIN, TX_PIN);	// or use custom pins

	// Now let's create a PZEM003 object
		// modbus_addr Slave addressing
	// #define ADDR_BCAST              0x00    // broadcast address    (slaves are not supposed to answer here)
	// #define ADDR_MIN                0x01    // lowest slave address
	// #define ADDR_MAX                0xF7    // highest slave address
	// #define ADDR_ANY                0xF8    // default catch-all address

	g_B18_PZ003	  = new DummyPZ003(B18_PZEM_ID, ADDR_ANY,"PZEM003-01"); //char[9]);   // i.e. PZEM-123

	// and link our port with PZEM object
	g_B18_PZ003->attachMsgQ(g_B18_UartQueue);

	// one last step - we must start PortQ tasks to handle messages
	g_B18_UartQueue->startQueues();

	// set shunt type
	g_B18_PZ003->setShunt(shunt_t::type_50A);

	// now it is all ready to exchange data with PZEM
	// let's update it's metrics
	g_B18_PZ003->updateMetrics();

	// and try to check the voltage value
	auto* m = g_B18_PZ003->getMetricsPZ003();	 // obtain a pointer to objects metrics

	//auto* m = g_B18_PZ003->getMetricsPZ003();	 // obtain a pointer to objects metrics

	Serial.printf("PZEM voltage: %d (decivolts)\n", m->voltage);

	//  But, hey! Wait a minute! Why the voltage value printed in console is ZERO?
	//   That's the trick! We are currently enqueued request message to PZEM and it will respond back
	//   some time in the future, somewhere in about ~100 ms... Almost an eternity in CPU cycles :)
	//   When we checked our metrics for data and printed the result - request might be still on it's
	//   way to PZEM over serial line.

	//   Let's not waste the time we have while PZEM is making a reply and do something usefull meanwhile

	Serial.println("We can do some funny stuff while PZEM is preparing a reply");

	// OK, 1000 ms is too long to print garbage, let's just take some sleep
	delay(200);  // for 200 ms

	// now we should have some response with updated values, let's check again
	Serial.printf("PZEM voltage: %d (decivolts)\n", m->voltage);

	// and find how long ago we had a reply came back while we were sleeping
	Serial.printf("PZEM data has been updated %lld ms ago\n", g_B18_PZ003->getState()->dataAge());

	// let's check other metrics
	Serial.printf("PZEM current: %u (mA)\n", m->current);

	// Or if someone likes floats instead? (I don't)
	Serial.printf("PZEM current as float: %.3f (Amps)\n", m->asFloat(pzmbus::meter_t::cur));	// allowed arguments are in enum meter_t:uint8_t { vol, cur, pwr, enrg, frq, pf, alrm }

	//   OK, you may not like this updateMetrics(), than wait, than get values approach in the code, right?
	//   If u want just some simple 'gimme values' style? Here is the proper way - run autopollig in background!
	//   Let's enable it!

	// run autopollig(updateMetrics) in background!
	// #define POLLER_PERIOD       PZEM_REFRESH_PERIOD         // auto update period in ms
	// #define PZEM_REFRESH_PERIOD     1000    // ms, PZEM updates it's internal register data every ~1 sec

	if (g_B18_PZ003->autopoll(true)) {
		Serial.println("Autopolling enabled");
	} else {
		Serial.println("Sorry, can't autopoll somehow :(");
	}

	//g_B18_PZ003->getPollrate();
	//g_B18_PZ003->setPollrate(1000);	// 1000ms

	//   Now we can do all kind of other stuff and anytime we ask for values from PZEM object it will give us some fresh data (default poll time is 1 second)
	//   Let's make a simple task - we will sleep for random time from 0 to 5 seconds and on wake we will check PZEM for new data
	// 간단한 작업을 만들어 보겠습니다. 0~5초 사이의 임의 시간 동안 잠을 자고 깨어날 때 PZEM에서 새 데이터를 확인합니다.

	int times = 5;
	do {
		long t = random(5000);
		Serial.printf("Going to sleep for %ld ms\n", t);
		delay(t);

		Serial.println("Wake up!");

		Serial.printf("PZEM voltage: %d (decivolts), last update time %lld ms ago\n\n", m->voltage, g_B18_PZ003->getState()->dataAge());
	} while (--times);

	// Cool, huh? :)

	//   And the last trick - assume I don't want this sleep/check also, I want to call some action once new data from PZEM arrives,
	//   i.e. send this data over websocket to a nice GUI (something like im my ESPEM project https://github.com/vortigont/espem).
	//   I have my own call-back routine and I want PZEM object to pull the string once it gets a fresh reply.
	//   Check the function B18_PZEM_rx_callback() below for now....

	// let's assign our callback to the PZEM instance.
	// I'm using lambda here to provide functional callback
	g_B18_PZ003->attach_rx_callback([](uint8_t p_PZEM_id, const RX_msg* p_PZEM_RX_msg) {
		// I can do all the required things here, but to keep it sepparate -
		// let's just call our function
		B18_PZEM_rx_callback(p_PZEM_id, p_PZEM_RX_msg);
	});

	// now I do not need to do anything
	// I can just halt here in an endless loop, but every second with a new message
	// console will print metrics data from PZEM
	for (;;) {
		delay(1000);
	}
}

void B18_run() {
	// we do not need this loop at all :)
	for (;;) {
		delay(1000);
	}
}

// @brief this is a custom callback for newly arrived data from PZEM
//
// @param id - this will be the ID of PZEM object, receiving the data
// @param m - this will be the struct with PZEM data (not only metrics, but any one)



void B18_PZEM_rx_callback(uint8_t p_PZEM_id, const RX_msg* p_PZEM_RX_msg) {
	// I can get the id of PZEM (might get handy if have more than one attached)
	Serial.printf("\nCallback triggered for PZEM ID: %d\n", p_PZEM_id);

	//	Here it is possible to obtain a fresh new data same way as before
	//	Serial.printf("PZEM current as float: %.3f (Amps)\n", g_B18_PZ003->getMetricsPZ003()->asFloat(pzmbus::meter_t::cur));

	// It is also possible to work directly on a raw data from PZEM
	// let's call for a little help here and use a pretty_printer() function
	// that parses and prints RX_msg to the stdout

	rx_msg_prettyp(p_PZEM_RX_msg);

	// that's the end of a callback
}
