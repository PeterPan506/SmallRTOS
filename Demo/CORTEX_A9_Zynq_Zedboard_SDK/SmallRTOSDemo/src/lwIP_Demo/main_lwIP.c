
/* Kernel includes. */
#include "SmallRTOS.h"

/* lwIP includes. */
#include "lwip/tcpip.h"

/*
 * Defined in lwIPApps.c.
 */
extern void lwIPAppsInit( void *pvArguments );

/*-----------------------------------------------------------*/

void main_lwIP( void )
{
	/* Init lwIP and start lwIP tasks. */
	tcpip_init( lwIPAppsInit, NULL );

	/* Start the tasks and timer running. */
	OSStart();

	for( ;; );
}

