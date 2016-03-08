/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#include "SmallRTOS.h"
#include "OSMemory.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The heap is made up as a list of structs of this type.
 * This does not have to be aligned since for getting its size,
 * we only use the macro SIZEOF_OSMEM_ALIGNED, which automatically alignes.
 */
typedef struct _tOSMem 
{
  uOSMemSize_t NextMem;	/** index (-> gpOSMemBegin[NextMem]) of the next struct */
  uOSMemSize_t PrevMem;	/** index (-> gpOSMemBegin[PrevMem]) of the previous struct */
  uOS8_t used;					/** 1: this area is used; 0: this area is unused */
}tOSMem_t;

/** All allocated blocks will be MIN_SIZE bytes big, at least!
 * MIN_SIZE can be overridden to suit your needs. Smaller values save space,
 * larger values could prevent too small blocks to fragment the RAM too much. */
#ifndef MIN_SIZE
#define MIN_SIZE             12
#endif /* MIN_SIZE */
/* some alignment macros: we define them here for better source code layout */
#define OSMIN_SIZE_ALIGNED		OSMEM_ALIGN_SIZE(MIN_SIZE)
#define SIZEOF_OSMEM_ALIGNED	OSMEM_ALIGN_SIZE(sizeof(tOSMem_t))
#define OSMEM_SIZE_ALIGNED		OSMEM_ALIGN_SIZE(OSMEM_SIZE)

/** If you want to relocate the heap to external memory, simply define
 * OSRAM_HEAP_POINTER as a void-pointer to that location.
 * If so, make sure the memory at that location is big enough (see below on
 * how that space is calculated). */
#ifndef OSRAM_HEAP_POINTER
/** the heap. we need one tOSMem_t at the end and some room for alignment */
uOS8_t OSRamHeap[OSMEM_SIZE_ALIGNED + (2*SIZEOF_OSMEM_ALIGNED) + OSMEM_ALIGNMENT];
#define OSRAM_HEAP_POINTER OSRamHeap
#endif /* OSRAM_HEAP_POINTER */

/** pointer to the heap (OSRamHeap): for alignment, gpOSMemBegin is now a pointer instead of an array */
static uOS8_t *gpOSMemBegin = NULL;//ram;
/** the last entry, always unused! */
static tOSMem_t *gpOSMemEnd = NULL;//ram_end;
/** pointer to the lowest free block, this is used for faster search */
static tOSMem_t *gpOSMemLFree = NULL;//lfree;


/**
 * "OSMemCombine" by combining adjacent empty struct mems.
 * After this function is through, there should not exist
 * one empty tOSMem_t pointing to another empty tOSMem_t.
 *
 * @param pOSMem this points to a tOSMem_t which just has been freed
 * @internal this function is only called by OSMemFree() and OSMemTrim()
 *
 * This assumes access to the heap is protected by the calling function
 * already.
 */
static void OSMemCombine(tOSMem_t *pOSMem)
{
	tOSMem_t *pNextOSMem;
	tOSMem_t *pPrevOSMem;

	pNextOSMem = (tOSMem_t *)(void *)&gpOSMemBegin[pOSMem->NextMem];
	if (pOSMem != pNextOSMem && pNextOSMem->used == 0 && (uOS8_t *)pNextOSMem != (uOS8_t *)gpOSMemEnd) 
	{
		// if pOSMem->NextMem is unused and not end of gpOSMemBegin, combine pOSMem and pOSMem->NextMem 
		if (gpOSMemLFree == pNextOSMem) 
		{
			gpOSMemLFree = pOSMem;
		}
		pOSMem->NextMem = pNextOSMem->NextMem;
		((tOSMem_t *)(void *)&gpOSMemBegin[pNextOSMem->NextMem])->PrevMem = (uOSMemSize_t)((uOS8_t *)pOSMem - gpOSMemBegin);
	}

	// Combine backward 
	pPrevOSMem = (tOSMem_t *)(void *)&gpOSMemBegin[pOSMem->PrevMem];
	if (pPrevOSMem != pOSMem && pPrevOSMem->used == 0) 
	{
		// if pOSMem->PrevMem is unused, combine pOSMem and pOSMem->PrevMem 
		if (gpOSMemLFree == pOSMem) 
		{
			gpOSMemLFree = pPrevOSMem;
		}
		pPrevOSMem->NextMem = pOSMem->NextMem;
		((tOSMem_t *)(void *)&gpOSMemBegin[pOSMem->NextMem])->PrevMem = (uOSMemSize_t)((uOS8_t *)pPrevOSMem - gpOSMemBegin);
	}
}

/**
 * Zero the heap and initialize start, end and lowest-free
 */
void OSMemInit(void)
{
	tOSMem_t *ptOSMemTemp;

	// align the heap 
	gpOSMemBegin = (uOS8_t *)OSMEM_ALIGN_ADDR(OSRAM_HEAP_POINTER);
	// initialize the start of the heap 
	ptOSMemTemp = (tOSMem_t *)(void *)gpOSMemBegin;
	ptOSMemTemp->NextMem = OSMEM_SIZE_ALIGNED;
	ptOSMemTemp->PrevMem = 0;
	ptOSMemTemp->used = 0;
	// initialize the end of the heap 
	gpOSMemEnd = (tOSMem_t *)(void *)&gpOSMemBegin[OSMEM_SIZE_ALIGNED];
	gpOSMemEnd->used = 1;
	gpOSMemEnd->NextMem = OSMEM_SIZE_ALIGNED;
	gpOSMemEnd->PrevMem = OSMEM_SIZE_ALIGNED;

	// initialize the lowest-free pointer to the start of the heap 
	gpOSMemLFree = (tOSMem_t *)(void *)gpOSMemBegin;
}

/**
 * Put a tOSMem_t back on the heap
 *
 * @param pMem is the data portion of a tOSMem_t as returned by a previous
 *             call to OSMemMalloc()
 */
void OSMemFree(void *pMem)
{
	tOSMem_t *ptOSMemTemp;
//	MEM_FREE_DECL_PROTECT();

	if (pMem == NULL) 
	{
		return;
	}

	if ((uOS8_t *)pMem < (uOS8_t *)gpOSMemBegin || (uOS8_t *)pMem >= (uOS8_t *)gpOSMemEnd) 
	{
		return;
	}
	// protect the heap from concurrent access 
	OS_ENTER_CRITICAL();
	// Get the corresponding tOSMem_t ... 
	ptOSMemTemp = (tOSMem_t *)(void *)((uOS8_t *)pMem - SIZEOF_OSMEM_ALIGNED);
	// ... which has to be in a used state ... 
	//ptOSMemTemp->used must be 1
	// ... and is now unused. 
	ptOSMemTemp->used = 0;

	if (ptOSMemTemp < gpOSMemLFree) 
	{
		// the newly freed struct is now the lowest 
		gpOSMemLFree = ptOSMemTemp;
	}

	// finally, see if prev or next are free also 
	OSMemCombine(ptOSMemTemp);

	OS_EXIT_CRITICAL();
}

/**
 * Shrink memory returned by OSMemMalloc().
 *
 * @param pMem pointer to memory allocated by OSMemMalloc the is to be shrinked
 * @param newsize required size after shrinking (needs to be smaller than or
 *                equal to the previous size)
 * @return for compatibility reasons: is always == pMem, at the moment
 *         or NULL if newsize is > old size, in which case pMem is NOT touched
 *         or freed!
 */
void* OSMemTrim(void *pMem, uOSMemSize_t newsize)
{
	uOSMemSize_t size;
	uOSMemSize_t ptr, ptr2;
	tOSMem_t *ptOSMemTemp, *ptOSMemTemp2;
	// use the FREE_PROTECT here: it protects with sem OR SYS_ARCH_PROTECT 
//	MEM_FREE_DECL_PROTECT();

	// Expand the size of the allocated memory region so that we can adjust for alignment. 
	newsize = OSMEM_ALIGN_SIZE(newsize);

	if(newsize < OSMIN_SIZE_ALIGNED) 
	{
		// every data block must be at least OSMIN_SIZE_ALIGNED long 
		newsize = OSMIN_SIZE_ALIGNED;
	}

	if (newsize > OSMEM_SIZE_ALIGNED) 
	{
		return NULL;
	}

	if ((uOS8_t *)pMem < (uOS8_t *)gpOSMemBegin || (uOS8_t *)pMem >= (uOS8_t *)gpOSMemEnd) 
	{
		return pMem;
	}
	// Get the corresponding tOSMem_t ... 
	ptOSMemTemp = (tOSMem_t *)(void *)((uOS8_t *)pMem - SIZEOF_OSMEM_ALIGNED);
	// ... and its offset pointer 
	ptr = (uOSMemSize_t)((uOS8_t *)ptOSMemTemp - gpOSMemBegin);

	size = ptOSMemTemp->NextMem - ptr - SIZEOF_OSMEM_ALIGNED;
	if (newsize > size) 
	{
		// not supported
		return NULL;
	}
	if (newsize == size) 
	{
		// No change in size, simply return 
		return pMem;
	}

	// protect the heap from concurrent access 
	OS_ENTER_CRITICAL();

	ptOSMemTemp2 = (tOSMem_t *)(void *)&gpOSMemBegin[ptOSMemTemp->NextMem];
	if(ptOSMemTemp2->used == 0) 
	{
		// The next struct is unused, we can simply move it at little 
		uOSMemSize_t NextMem;
		// remember the old next pointer 
		NextMem = ptOSMemTemp2->NextMem;
		// create new tOSMem_t which is moved directly after the shrinked ptOSMemTemp 
		ptr2 = ptr + SIZEOF_OSMEM_ALIGNED + newsize;
		if (gpOSMemLFree == ptOSMemTemp2) 
		{
			gpOSMemLFree = (tOSMem_t *)(void *)&gpOSMemBegin[ptr2];
		}
		ptOSMemTemp2 = (tOSMem_t *)(void *)&gpOSMemBegin[ptr2];
		ptOSMemTemp2->used = 0;
		// restore the next pointer 
		ptOSMemTemp2->NextMem = NextMem;
		// link it back to ptOSMemTemp 
		ptOSMemTemp2->PrevMem = ptr;
		// link ptOSMemTemp to it 
		ptOSMemTemp->NextMem = ptr2;
		// last thing to restore linked list: as we have moved ptOSMemTemp2,
		// let 'ptOSMemTemp2->NextMem->PrevMem' point to ptOSMemTemp2 again. but only if ptOSMemTemp2->NextMem is not
		// the end of the heap 
		if (ptOSMemTemp2->NextMem != OSMEM_SIZE_ALIGNED) 
		{
			((tOSMem_t *)(void *)&gpOSMemBegin[ptOSMemTemp2->NextMem])->PrevMem = ptr2;
		}
		// no need to combine, we've already done that 
	} 
	else if (newsize + SIZEOF_OSMEM_ALIGNED + OSMIN_SIZE_ALIGNED <= size) 
	{
		// Next struct is used but there's room for another tOSMem_t with
		// at least OSMIN_SIZE_ALIGNED of data.
		// Old size ('size') must be big enough to contain at least 'newsize' plus a tOSMem_t
		// ('SIZEOF_OSMEM_ALIGNED') with some data ('OSMIN_SIZE_ALIGNED').
		// @todo we could leave out OSMIN_SIZE_ALIGNED. We would create an empty
		//       region that couldn't hold data, but when ptOSMemTemp->NextMem gets freed,
		//       the 2 regions would be combined, resulting in more free memory 
		ptr2 = ptr + SIZEOF_OSMEM_ALIGNED + newsize;
		ptOSMemTemp2 = (tOSMem_t *)(void *)&gpOSMemBegin[ptr2];
		if (ptOSMemTemp2 < gpOSMemLFree) 
		{
			gpOSMemLFree = ptOSMemTemp2;
		}
		ptOSMemTemp2->used = 0;
		ptOSMemTemp2->NextMem = ptOSMemTemp->NextMem;
		ptOSMemTemp2->PrevMem = ptr;
		ptOSMemTemp->NextMem = ptr2;
		if (ptOSMemTemp2->NextMem != OSMEM_SIZE_ALIGNED) 
		{
			((tOSMem_t *)(void *)&gpOSMemBegin[ptOSMemTemp2->NextMem])->PrevMem = ptr2;
		}
		// the original ptOSMemTemp->NextMem is used, so no need to combine! 
	}
/*	else 
	{
		next tOSMem_t is used but size between ptOSMemTemp and ptOSMemTemp2 is not big enough
		to create another tOSMem_t
		-> don't do anyhting. 
		-> the remaining space stays unused since it is too small
	} 
*/
  OS_EXIT_CRITICAL();
  return pMem;
}

/**
 * Adam's OSMemMalloc() plus solution for bug #17922
 * Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param size is the minimum size of the requested block in bytes.
 * @return pointer to allocated memory or NULL if no free memory was found.
 *
 * Note that the returned value will always be aligned (as defined by OSMEM_ALIGNMENT).
 */
void* OSMemMalloc(uOSMemSize_t size)
{
	uOS8_t * pResult = NULL;
	uOSMemSize_t ptr, ptr2;
	tOSMem_t *ptOSMemTemp, *ptOSMemTemp2;

//	MEM_ALLOC_DECL_PROTECT();

	if(gpOSMemEnd==NULL)
	{
		OSMemInit();
		if(gpOSMemEnd==NULL)
		{
			return pResult;
		}
	}
	if (size == 0) 
	{
		return pResult;
	}

	// Expand the size of the allocated memory region so that we can
	// adjust for alignment. 
	size = OSMEM_ALIGN_SIZE(size);

	if(size < OSMIN_SIZE_ALIGNED) 
	{
		// every data block must be at least OSMIN_SIZE_ALIGNED long 
		size = OSMIN_SIZE_ALIGNED;
	}

	if (size > OSMEM_SIZE_ALIGNED) 
	{
		return pResult;
	}

	// protect the heap from concurrent access 
	//  sys_mutex_lock(&mem_mutex);
	OS_ENTER_CRITICAL();

	// Scan through the heap searching for a free block that is big enough,
	// beginning with the lowest free block.
	for (ptr = (uOSMemSize_t)((uOS8_t *)gpOSMemLFree - gpOSMemBegin); ptr < OSMEM_SIZE_ALIGNED - size;
		ptr = ((tOSMem_t *)(void *)&gpOSMemBegin[ptr])->NextMem) 
	{
		ptOSMemTemp = (tOSMem_t *)(void *)&gpOSMemBegin[ptr];

		if ((!ptOSMemTemp->used) && (ptOSMemTemp->NextMem - (ptr + SIZEOF_OSMEM_ALIGNED)) >= size) 
		{
			// ptOSMemTemp is not used and at least perfect fit is possible:
			// ptOSMemTemp->NextMem - (ptr + SIZEOF_OSMEM_ALIGNED) gives us the 'user data size' of ptOSMemTemp 

			if (ptOSMemTemp->NextMem - (ptr + SIZEOF_OSMEM_ALIGNED) >= (size + SIZEOF_OSMEM_ALIGNED + OSMIN_SIZE_ALIGNED)) 
			{
				// (in addition to the above, we test if another tOSMem_t (SIZEOF_OSMEM_ALIGNED) containing
				// at least OSMIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'ptOSMemTemp')
				// -> split large block, create empty remainder,
				// remainder must be large enough to contain OSMIN_SIZE_ALIGNED data: if
				// ptOSMemTemp->NextMem - (ptr + (2*SIZEOF_OSMEM_ALIGNED)) == size,
				// tOSMem_t would fit in but no data between ptOSMemTemp2 and ptOSMemTemp2->NextMem
				// @todo we could leave out OSMIN_SIZE_ALIGNED. We would create an empty
				//       region that couldn't hold data, but when ptOSMemTemp->NextMem gets freed,
				//       the 2 regions would be combined, resulting in more free memory
				ptr2 = ptr + SIZEOF_OSMEM_ALIGNED + size;
				// create ptOSMemTemp2 struct 
				ptOSMemTemp2 = (tOSMem_t *)(void *)&gpOSMemBegin[ptr2];
				ptOSMemTemp2->used = 0;
				ptOSMemTemp2->NextMem = ptOSMemTemp->NextMem;
				ptOSMemTemp2->PrevMem = ptr;
				// and insert it between ptOSMemTemp and ptOSMemTemp->NextMem 
				ptOSMemTemp->NextMem = ptr2;
				ptOSMemTemp->used = 1;

				if (ptOSMemTemp2->NextMem != OSMEM_SIZE_ALIGNED) 
				{
					((tOSMem_t *)(void *)&gpOSMemBegin[ptOSMemTemp2->NextMem])->PrevMem = ptr2;
				}
//				MEM_STATS_INC_USED(used, (size + SIZEOF_OSMEM_ALIGNED));
			} 
			else 
			{
				// (a ptOSMemTemp2 struct does no fit into the user data space of ptOSMemTemp and ptOSMemTemp->NextMem will always
				// be used at this point: if not we have 2 unused structs in a row, OSMemCombine should have
				// take care of this).
				// -> near fit or excact fit: do not split, no ptOSMemTemp2 creation
				// also can't move ptOSMemTemp->NextMem directly behind ptOSMemTemp, since ptOSMemTemp->NextMem
				// will always be used at this point!
				ptOSMemTemp->used = 1;
//				MEM_STATS_INC_USED(used, ptOSMemTemp->NextMem - (uOSMemSize_t)((uOS8_t *)ptOSMemTemp - gpOSMemBegin));
			}

			if (ptOSMemTemp == gpOSMemLFree) 
			{
				// Find next free block after ptOSMemTemp and update lowest free pointer 
				while (gpOSMemLFree->used && gpOSMemLFree != gpOSMemEnd) 
				{
					//OS_EXIT_CRITICAL();
					// prevent high interrupt latency... 
					//OS_ENTER_CRITICAL();
					gpOSMemLFree = (tOSMem_t *)(void *)&gpOSMemBegin[gpOSMemLFree->NextMem];
				}
			}
//			OS_EXIT_CRITICAL();
			//sys_mutex_unlock(&mem_mutex);
			pResult = (uOS8_t *)ptOSMemTemp + SIZEOF_OSMEM_ALIGNED;
			break;
		}
	}

	OS_EXIT_CRITICAL();
	//sys_mutex_unlock(&mem_mutex);

	return pResult;
}

/**
 * Contiguously allocates enough space for count objects that are size bytes
 * of memory each and returns a pointer to the allocated memory.
 *
 * The allocated memory is filled with bytes of value zero.
 *
 * @param count number of objects to allocate
 * @param size size of the objects to allocate
 * @return pointer to allocated memory / NULL pointer if there is an error
 */
void* OSMemCalloc(uOSMemSize_t count, uOSMemSize_t size)
{
	void *pMem;

	// allocate 'count' objects of size 'size' 
	pMem = OSMemMalloc(count * size);
	if (pMem) 
	{
		// zero the memory 
		memset(pMem, 0, count * size);
	}
	return pMem;
}

#ifdef __cplusplus
}
#endif
