/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef __OS_MEMORY_HPP
#define __OS_MEMORY_HPP

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OSMEM_SIZE			OSTOTAL_HEAP_SIZE

/* OSMEM_SIZE would have to be aligned, but using 64000 here instead of
 * 65535 leaves some room for alignment...
 */
#if OSMEM_SIZE > 64000L
typedef uOS32_t uOSMemSize_t;
#else
typedef uOS16_t uOSMemSize_t;
#endif /* OSMEM_SIZE > 64000 */

void  OSMemInit(void);
void *OSMemTrim(void *pMem, uOSMemSize_t size);
void *OSMemMalloc(uOSMemSize_t size);
void *OSMemCalloc(uOSMemSize_t count, uOSMemSize_t size);
void  OSMemFree(void *pMem);

/** Calculate memory size for an aligned buffer - returns the next highest
 * multiple of OSMEM_ALIGNMENT (e.g. OSMEM_ALIGN_SIZE(3) and
 * OSMEM_ALIGN_SIZE(4) will both yield 4 for OSMEM_ALIGNMENT == 4).
 */
#ifndef OSMEM_ALIGN_SIZE
#define OSMEM_ALIGN_SIZE(size) (((size) + OSMEM_ALIGNMENT - 1) & ~(OSMEM_ALIGNMENT-1))
#endif

/** Calculate safe memory size for an aligned buffer when using an unaligned
 * type as storage. This includes a safety-margin on (OSMEM_ALIGNMENT - 1) at the
 * start (e.g. if buffer is UINT16[] and actual data will be UINT32*)
 */
#ifndef OSMEM_ALIGN_BUFFER
#define OSMEM_ALIGN_BUFFER(size) (((size) + OSMEM_ALIGNMENT - 1))
#endif

/** Align a memory pointer to the alignment defined by OSMEM_ALIGNMENT
 * so that ADDR % OSMEM_ALIGNMENT == 0
 */
#ifndef OSMEM_ALIGN_ADDR
#define OSMEM_ALIGN_ADDR(addr) ((void *)(((uOS32_t)(addr) + OSMEM_ALIGNMENT - 1) & ~(uOS32_t)(OSMEM_ALIGNMENT-1)))
#endif

#ifdef __cplusplus
}
#endif

#endif //__OS_MEMORY_HPP
