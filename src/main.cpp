#include <Arduino.h>


//#define		B010				//	02_SinglePZEM003	: Build-Ok
#ifdef		B010
	#include "B10_PZEM_edl_DC_Single_001.h"
#endif

//#define			B018		: Build-Ok
#ifdef		B018
	#include "B18_PZEM_edl_DC_single_FAKE_001.h"
#endif


//#define			B021			//	03_MultiplePZEM004 : Build-Ok
#ifdef		B021
	#include "B21_PZEM_edl_DC_Multi_003.h"
#endif

//#define			B020			//	03_MultiplePZEM004 : Build-Ok
#ifdef		B020
	#include "B20_PZEM_edl_DC_Multi_002.h"
#endif


#define			B050		// 	05_TimeSeries	:  : Build-fail
#ifdef		B050
	#include "B50_PZEM_edi_DC_TimeSerial_003.h"
#endif

#define			B051		// 	05_TimeSeries	:  : Build-fail
#ifdef		B051
	#include "B51_PZEM_edi_DC_Timeseries_010.h"
#endif


//#define			B070		//  PZEM_CLI		:  : Build-fail
#ifdef		B070
	#include "B70_PZEM_CLI_002.h"
#endif

#define			C010		//  
#ifdef		C010
	#include "C10_ESP_AsyncEvent_Button_001.h"
#endif

#define			C020		//  
#ifdef		C020
	#include "C20_BasicEvent_001.h"
#endif

#define			C030		//  
#ifdef		C030
	#include "C30_callbackMenu_001.h"
#endif

#define			C040		//  
#ifdef		C040
	#include "C40_Encoder_001.h"
#endif

void setup() {


	Serial.begin(115200);	 // just an ordinary Serial console to interact with

	#ifdef B010
		B10_Init();
	#endif

	#ifdef B018
		B18_Init();
	#endif

	#ifdef B021
		B21_Init();
	#endif

	#ifdef B020
		B20_Init();
	#endif

	#ifdef B050
		B50_PZEM_Init();
	#endif

	#ifdef B051
		B51_init();
	#endif


	
	#ifdef	B070
		B70_PZEM_CLI_init();
	#endif

    #ifdef	C010
		C10_init();
	#endif

	#ifdef	C020
		C20_init();
	#endif

	#ifdef	C030
		C30_init();
	#endif

	#ifdef	C040
		C40_init();
	#endif
	
}

void loop() {
	#ifdef B010
		B10_run();
	#endif

	#ifdef B021

	#endif
	#ifdef B020

	#endif

	#ifdef B050
		B50_PZEM_run();
	#endif

	#ifdef B051
		B51_run();
	#endif
	
	#ifdef	B070
		B70_PZEM_CLI_run();
	#endif

	#ifdef	C010
		C10_run();
	#endif
	#ifdef	C020
		C20_run();
	#endif
	#ifdef	C030
		C30_run();
	#endif
	#ifdef	C040
		C40_run();
	#endif
	
}

