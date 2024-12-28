#include <Arduino.h>

#define 	DBG_PROGRESS1
#define		DBG_ERROR

#include "W10_Wifi_001.h"


//#define		BB010				//	02_SinglePZEM003	: Build-Ok
#ifdef		BB010
	#include "B010/B10_PZEM_edl_DC_Single_001.h"
#endif

//#define			B018		: Build-Ok
#ifdef		B018
	#include "B010/B18_PZEM_edl_DC_single_FAKE_001.h"
#endif


//#define			B021			//	03_MultiplePZEM004 : Build-Ok
#ifdef		B021
	#include "B020/B21_PZEM_edl_DC_Multi_003.h"
#endif

//#define			B020			//	03_MultiplePZEM004 : Build-Ok
#ifdef		B020
	#include "B020/B20_PZEM_edl_DC_Multi_002.h"
#endif


//#define			B040			//	B40_PZEM_MixedPool_001
#ifdef		B040
	#include "B040/B40_PZEM_MixedPool_001.h"
#endif

//#define			B050		// 	05_TimeSeries	:  : Build-fail
#ifdef		B050
	#include "B051/B50_PZEM_edi_DC_Timeseries_003.h"
#endif

#define			B051		// 	05_TimeSeries	:  : Build-fail
#ifdef		B051
	#include "B051/B51_PZEM_edi_DC_Timeseries_014.h"
	//#include "B51_PZEM_edi_DC_Timeseries_013.h"
	// #include "B51_PZEM_edi_DC_Timeseries_012.h"
	// #include "B51_PZEM_edi_DC_Timeseries_011.h"
#endif


//#define			B070		//  PZEM_CLI		:  : Build-fail
#ifdef		B070
	#include "B070/B70_PZEM_CLI_002.h"
#endif

//#define			C010		//
#ifdef		C010
	#include "C040_AsyncBtn/C10_ESP_AsyncEvent_Button_001.h"
#endif

//#define			C020		//
#ifdef		C020
	#include "C040_AsyncBtn/C20_BasicEvent_001.h"
#endif

//#define			C030		//
#ifdef		C030
	#include "C040_AsyncBtn/C30_callbackMenu_001.h"
#endif

//#define			C040		//
#ifdef		C040
	#include "C040_AsyncBtn/C40_Encoder_001.h"
#endif

void setup() {


	Serial.begin(115200);	 // just an ordinary Serial console to interact with

	W10_T2_Wifi_init();

	delay(1000);

	#ifdef BB010
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

	#ifdef B040
		B40_init();
	#endif


	#ifdef B050
		B50_init();
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
	#ifdef BB010
		B10_run();
	#endif

	#ifdef B021

	#endif
	#ifdef B020

	#endif

	#ifdef B040
		B40_run();
	#endif

	#ifdef B050
		B50_run();
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

