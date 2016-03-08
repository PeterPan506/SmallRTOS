/******************************************************************************

******************************************************************************/
/*****************************************************************************/
/**
* @file asm_vectors.s
*
* This file contains the initial vector table for the Cortex A9 processor
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------- -------- ---------------------------------------------------
* 1.00a ecm/sdm 10/20/09 Initial version
* 3.05a sdm	02/02/12 Save lr when profiling is enabled
* 3.10a srt     04/18/13 Implemented ARM Erratas. Please refer to file
*			 'xil_errata.h' for errata description
* </pre>
*
* @note
*
* None.
*
******************************************************************************/

#include "xil_errata.h"

.org 0
.text
.arm

.global _boot
.global _smallrtos_vector_table

.global FIQInterrupt
.global DataAbortInterrupt
.global PrefetchAbortInterrupt
.global FitInstallSmallRTOSVectorTable

.extern SmallRTOS_IRQ_Handler
.extern SmallRTOS_SWI_Handler

.section .smallrtos_vectors
_smallrtos_vector_table:
	B	  _boot
	B	  SmallRTOS_Undefined
	ldr   pc, _swi
	B	  SmallRTOS_PrefetchAbortHandler
	B	  SmallRTOS_DataAbortHandler
	NOP	  /* Placeholder for address exception vector*/
	LDR   PC, _irq
	B	  SmallRTOS_FIQHandler

_irq:   .word SmallRTOS_IRQ_Handler
_swi:   .word SmallRTOS_SWI_Handler


.align 4
SmallRTOS_FIQHandler:			/* FIQ vector handler */
	stmdb	sp!,{r0-r3,r12,lr}	/* state save from compiled code */
FIQLoop:
	blx	FIQInterrupt			/* FIQ vector */
	ldmia	sp!,{r0-r3,r12,lr}	/* state restore from compiled code */
	subs	pc, lr, #4			/* adjust return */

.align 4
SmallRTOS_Undefined:				/* Undefined handler */
	b		.

.align 4
SmallRTOS_DataAbortHandler:		/* Data Abort handler */
#ifdef CONFIG_ARM_ERRATA_775420
	dsb
#endif
	stmdb	sp!,{r0-r3,r12,lr}	/* state save from compiled code */
	blx	DataAbortInterrupt		/*DataAbortInterrupt :call C function here */
	ldmia	sp!,{r0-r3,r12,lr}	/* state restore from compiled code */
	subs	pc, lr, #4			/* adjust return */

.align 4
SmallRTOS_PrefetchAbortHandler:	/* Prefetch Abort handler */
#ifdef CONFIG_ARM_ERRATA_775420
	dsb
#endif
	stmdb	sp!,{r0-r3,r12,lr}	/* state save from compiled code */
	blx	PrefetchAbortInterrupt	/* PrefetchAbortInterrupt: call C function here */
	ldmia	sp!,{r0-r3,r12,lr}	/* state restore from compiled code */
	subs	pc, lr, #4			/* adjust return */

.align 4
.type FitInstallSmallRTOSVectorTable, %function
FitInstallSmallRTOSVectorTable:

	/* Set VBAR to the vector table that contains the SmallRTOS handlers. */
	ldr	r0, =_smallrtos_vector_table
	mcr	p15, 0, r0, c12, c0, 0
	dsb
	isb
	bx lr


.end




