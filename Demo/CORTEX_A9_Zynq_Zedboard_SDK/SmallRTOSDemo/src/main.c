/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/
#include <stdio.h>
#include "platform.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "interrupt.h"
#include "SmallRTOS.h"

/* Xilinx includes. */
#include "platform.h"
#include "xparameters.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "xil_exception.h"

#ifdef __cplusplus
extern "C" {
#endif

/* mainSELECTED_APPLICATION is used to select between three demo applications.
 *
 * When mainSELECTED_APPLICATION is set to 0 the simple task example will
 * be run.
 *
 * When mainSELECTED_APPLICATION is set to 1 the led task application will
 * be run. In this demo, you should load the bit file "zedboard_wrapper.bit"
 * to the chip first!!!!!
 *
 * When mainSELECTED_APPLICATION is set to 2 the lwIP example will be run.
 */
#define mainSELECTED_APPLICATION	0

#if ( mainSELECTED_APPLICATION == 0 )
	extern void main_general( void );
#elif ( mainSELECTED_APPLICATION == 1 )
	extern void main_led( void );
#elif ( mainSELECTED_APPLICATION == 2 )
	extern void main_lwIP( void );
#else
	extern void main_general( void );
#endif


static void SetupHardware( void );
extern void FitInstallSmallRTOSVectorTable( void );
/* The interrupt controller is initialised in this file, and made available to
other modules. */
XScuGic xInterruptController;

int main( void )
{

    SetupHardware();
    init_platform();
	
	/* The mainSELECTED_APPLICATION setting is described at the top
	of this file. */
	#if( mainSELECTED_APPLICATION == 0 )
	{
		main_general();
	}
	#elif( mainSELECTED_APPLICATION == 1 )
	{
		/* In this demo, you should load the bit file "zedboard_wrapper.bit"
		to the chip first!!!!!*/
		main_led();
	}
	#elif( mainSELECTED_APPLICATION == 2 )
	{
		main_lwIP();
	}	
	#else
	{
		main_general();
	}
	#endif

    return 0;
}

static void SetupHardware( void )
{
	uOS32_t xStatus;
	XScuGic_Config *pxGICConfig;

	(void)xStatus;
	/* Ensure no interrupts execute while the scheduler is in an inconsistent
	state.  Interrupts are automatically enabled when the scheduler is
	started. */
	FitDISABLE_INTERRUPTS();

	/* Obtain the configuration of the GIC. */
	pxGICConfig = XScuGic_LookupConfig( XPAR_SCUGIC_SINGLE_DEVICE_ID );

	/* Sanity check the SmallRTOSConfig.h settings are correct for the
	hardware. */
	/* Install a default handler for each GIC interrupt. */
	xStatus = XScuGic_CfgInitialize( &xInterruptController, pxGICConfig, pxGICConfig->CpuBaseAddress );

	/* The Xilinx projects use a BSP that do not allow the start up code to be
	altered easily.  Therefore the vector table used by SmallRTOS is defined in
	SmallRTOS_asm_vectors.S, which is part of this project.  Switch to use the
	SmallRTOS vector table. */
	FitInstallSmallRTOSVectorTable();
}


#ifdef __cplusplus
}
#endif
