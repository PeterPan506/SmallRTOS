/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#include "OSList.h"

#ifdef __cplusplus
extern "C" {
#endif

void OSListInitialise( tOSList_t * const ptList )
{
	/* The list structure contains a list item which is used to mark the
	end of the list.  To initialise the list the list end is inserted
	as the only list entry. */
	ptList->ptIndex = ( tOSListItem_t * ) &( ptList->tListEnd );

	/* The list end value is the highest possible value in the list to
	ensure it remains at the end of the list. */
	ptList->tListEnd.uxItemValue = OSPEND_FOREVER_VALUE;

	/* The list end next and previous pointers point to itself so we know
	when the list is empty. */
	ptList->tListEnd.ptNext = ( tOSListItem_t * ) &( ptList->tListEnd );
	ptList->tListEnd.ptPrevious = ( tOSListItem_t * ) &( ptList->tListEnd );

	ptList->tListEnd.pvTCB = NULL;	 //not use in tListEnd
	ptList->tListEnd.pvList = NULL; //not use in tListEnd

	ptList->uxNumberOfItems = ( uOSBase_t ) 0U;
}

void OSListItemInitialise( tOSListItem_t * const ptListItem )
{
	/* Make sure the list item is not recorded as being on a list. */
	ptListItem->pvList = NULL;
}

void OSListInsertItemToEnd( tOSList_t * const ptList, tOSListItem_t * const ptNewListItem )
{
	tOSListItem_t * const ptIndex = ptList->ptIndex;

	/* Insert a new list item into ptList, but rather than sort the list,
	makes the new list item the last item */
	ptNewListItem->ptNext = ptIndex;
	ptNewListItem->ptPrevious = ptIndex->ptPrevious;

	ptIndex->ptPrevious->ptNext = ptNewListItem;
	ptIndex->ptPrevious = ptNewListItem;

	/* Remember which list the item is in. */
	ptNewListItem->pvList = ( void * ) ptList;

	( ptList->uxNumberOfItems )++;
}

void OSListInsertItem( tOSList_t * const ptList, tOSListItem_t * const ptNewListItem )
{
	tOSListItem_t *ptIterator;
	const uOSTick_t xValueOfInsertion = ptNewListItem->uxItemValue;

	/* Insert the new list item into the list, sorted in uxItemValue order.

	If the list already contains a list item with the same item value then the
	new list item should be placed after it.  This ensures that TCB's which are
	stored in ready lists (all of which have the same uxItemValue value) get a
	share of the CPU.  However, if the uxItemValue is the same as the back marker
	the iteration loop below will not end.  Therefore the value is checked
	first, and the algorithm slightly modified if necessary. */
	if( xValueOfInsertion == OSPEND_FOREVER_VALUE )
	{
		ptIterator = ptList->tListEnd.ptPrevious;
	}
	else
	{
		for( ptIterator = ( tOSListItem_t * ) &( ptList->tListEnd ); ptIterator->ptNext->uxItemValue <= xValueOfInsertion; ptIterator = ptIterator->ptNext )
		{
			/* There is nothing to do here, just iterating to the wanted
			insertion position. */
		}
	}

	ptNewListItem->ptNext = ptIterator->ptNext;
	ptNewListItem->ptNext->ptPrevious = ptNewListItem;
	ptNewListItem->ptPrevious = ptIterator;
	ptIterator->ptNext = ptNewListItem;

	/* Remember which list the item is in.  This allows fast removal of the
	item later. */
	ptNewListItem->pvList = ( void * ) ptList;

	( ptList->uxNumberOfItems )++;
}

uOSBase_t OSListRemoveItem( tOSListItem_t * const ptItemToRemove )
{
	/* The list item knows which list it is in.  Obtain the list from the list
	item. */
	tOSList_t * const ptList = ( tOSList_t * ) ptItemToRemove->pvList;

	ptItemToRemove->ptNext->ptPrevious = ptItemToRemove->ptPrevious;
	ptItemToRemove->ptPrevious->ptNext = ptItemToRemove->ptNext;

	/* Make sure the index is left pointing to a valid item. */
	if( ptList->ptIndex == ptItemToRemove )
	{
		ptList->ptIndex = ptItemToRemove->ptPrevious;
	}

	ptItemToRemove->pvList = NULL;
	( ptList->uxNumberOfItems )--;

	return ptList->uxNumberOfItems;
}

#ifdef __cplusplus
}
#endif

