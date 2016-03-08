/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef  _interrupt_h_
#define  _interrupt_h_

#include "xil_types.h"
#include "xparameters.h"

#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"

#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define INTC_DEVICE_INT_ID	61


int ConfigInterrupt(u16 DeviceId, Xil_ExceptionHandler InterruptHandler, int intID);
int SetUpInterrupt(XScuGic *XScuGicInstancePtr);

XScuGic* gpInterruptController; 	     /* Instance of the Interrupt Controller */

volatile static int InterruptProcessed = FALSE;


#endif /* _interrupt_h_ */
