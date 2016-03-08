/*

*/
#include "SmallRTOS.h"

/* Xilinx includes. */
#include "xscutimer.h"
#include "xscugic.h"

#define XSCUTIMER_CLOCK_HZ ( XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ / 2UL )

static XScuTimer xTimer;

/*
 * The application must provide a function that configures a peripheral to
 * create the SmallRTOS tick interrupt, then define configSETUP_TICK_INTERRUPT()
 * in SmallRTOSConfig.h to call the function.  This file contains a function
 * that is suitable for use on the Zynq SoC.
 */
void vConfigureTickInterrupt( void )
{
static XScuGic xInterruptController; 	/* Interrupt controller instance */
uint32_t xStatus;
extern void FitOSTickISR( void );
XScuTimer_Config *pxTimerConfig;
XScuGic_Config *pxGICConfig;
const uint8_t ucRisingEdge = 3;

	(void)xStatus;
	/* This function is called with the IRQ interrupt disabled, and the IRQ
	interrupt should be left disabled.  It is enabled automatically when the
	scheduler is started. */

	/* Ensure XScuGic_CfgInitialize() has been called.  In this demo it has
	already been called from prvSetupHardware() in main(). */
	pxGICConfig = XScuGic_LookupConfig( XPAR_SCUGIC_SINGLE_DEVICE_ID );
	xStatus = XScuGic_CfgInitialize( &xInterruptController, pxGICConfig, pxGICConfig->CpuBaseAddress );

	/* The priority must be the lowest possible. */
	XScuGic_SetPriorityTriggerType( &xInterruptController, XPAR_SCUTIMER_INTR, FitLOWEST_USABLE_INTERRUPT_PRIORITY << FitPRIORITY_SHIFT, ucRisingEdge );

	/* Install the SmallRTOS tick handler. */
	xStatus = XScuGic_Connect( &xInterruptController, XPAR_SCUTIMER_INTR, (Xil_ExceptionHandler) FitOSTickISR, ( void * ) &xTimer );

	/* Initialise the timer. */
	pxTimerConfig = XScuTimer_LookupConfig( XPAR_SCUTIMER_DEVICE_ID );
	xStatus = XScuTimer_CfgInitialize( &xTimer, pxTimerConfig, pxTimerConfig->BaseAddr );

	/* Enable Auto reload mode. */
	XScuTimer_EnableAutoReload( &xTimer );

	/* Load the timer counter register. */
	XScuTimer_LoadTimer( &xTimer, XSCUTIMER_CLOCK_HZ / OSTICK_RATE_HZ );

	/* Start the timer counter and then wait for it to timeout a number of
	times. */
	XScuTimer_Start( &xTimer );

	/* Enable the interrupt for the xTimer in the interrupt controller. */
	XScuGic_Enable( &xInterruptController, XPAR_SCUTIMER_INTR );

	/* Enable the interrupt in the xTimer itself. */
	vClearTickInterrupt();
	XScuTimer_EnableInterrupt( &xTimer );
}
/*-----------------------------------------------------------*/

void vClearTickInterrupt( void )
{
	XScuTimer_ClearInterruptStatus( &xTimer );
}
/*-----------------------------------------------------------*/

void vApplicationIRQHandler( uint32_t ulICCIAR )
{
extern const XScuGic_Config XScuGic_ConfigTable[];
static const XScuGic_VectorTableEntry *pxVectorTable = XScuGic_ConfigTable[ XPAR_SCUGIC_SINGLE_DEVICE_ID ].HandlerTable;
uint32_t ulInterruptID;
const XScuGic_VectorTableEntry *pxVectorEntry;

	/* Re-enable interrupts. */
    __asm ( "cpsie i" );

	/* The ID of the interrupt is obtained by bitwise anding the ICCIAR value
	with 0x3FF. */
	ulInterruptID = ulICCIAR & 0x3FFUL;
	if( ulInterruptID < XSCUGIC_MAX_NUM_INTR_INPUTS )
	{
		/* Call the function installed in the array of installed handler functions. */
		pxVectorEntry = &( pxVectorTable[ ulInterruptID ] );
		pxVectorEntry->Handler( pxVectorEntry->CallBackRef );
	}
}



