/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef __OS_LIST_H
#define __OS_LIST_H

#include "OSType.h"


#ifdef __cplusplus
extern "C" {
#endif

struct OSListItem
{
	volatile uOSTick_t 					uxItemValue;	
	struct OSListItem * volatile 		ptNext;
	struct OSListItem * volatile 		ptPrevious;
	void * 								pvTCB;
	void * volatile 					pvList;
};
typedef struct OSListItem 				tOSListItem_t;	

typedef struct OSList
{
	volatile uOSBase_t 					uxNumberOfItems;
	tOSListItem_t * volatile 			ptIndex;
	tOSListItem_t						tListEnd;
} tOSList_t;

#define OSListItemSetTCB( ptListItem, ptTCB )			( ( ptListItem )->pvTCB = ( void * ) ( ptTCB ) )
#define OSListItemGetTCB( ptListItem )					( ( ptListItem )->pvTCB )
#define OSListItemSetValue( ptListItem, xValue )		( ( ptListItem )->uxItemValue = ( xValue ) )
#define OSListItemGetValue( ptListItem )				( ( ptListItem )->uxItemValue )
#define OSListItemGetNextItem( ptListItem )				( ( ptListItem )->ptNext )
#define OSListItemGetList( ptListItem ) 				( ( ptListItem )->pvList )

#define OSlistGetHeadItemValue( ptList )				( ( ( ptList )->tListEnd ).ptNext->uxItemValue )
#define OSListGetHeadItem( ptList )						( ( ( ptList )->tListEnd ).ptNext )
#define OSListGetEndMarkerItem( ptList )				( ( tOSListItem_t const * ) ( &( ( ptList )->tListEnd ) ) )
#define OSListIsEmpty( ptList )							( ( uOSBase_t ) ( ( ptList )->uxNumberOfItems == ( uOSBase_t ) 0 ) )
#define OSListGetLength( ptList )						( ( ptList )->uxNumberOfItems )

#define OSListGetNextItemTCB(ptList, ptTCB)													\
{																							\
	tOSList_t * const ptConstList = ( ptList );												\
	/* Increment the index to the next item and return the item, ensuring */				\
	/* we don't return the marker used at the end of the list.  */							\
	( ptConstList )->ptIndex = ( ptConstList )->ptIndex->ptNext;							\
	if( ( void * ) ( ptConstList )->ptIndex == ( void * ) &( ( ptConstList )->tListEnd ) )	\
	{																						\
		( ptConstList )->ptIndex = ( ptConstList )->ptIndex->ptNext;						\
	}																						\
	( ptTCB ) = ( ptConstList )->ptIndex->pvTCB;											\
}

#define OSListGetHeadItemTCB( ptList )  				( (&( ( ptList )->tListEnd ))->ptNext->pvTCB )
#define OSListContainListItem( ptList, ptListItem ) 	( ( sOSBase_t ) ( ( ptListItem )->pvList == ( void * ) ( ptList ) ) )
#define OSListISInitialised( ptList ) 					( ( ptList )->tListEnd.uxItemValue == OSPEND_FOREVER_VALUE )

void OSListItemInitialise( tOSListItem_t * const ptListItem );

void OSListInitialise( tOSList_t * const ptList );
void OSListInsertItem( tOSList_t * const ptList, tOSListItem_t * const ptNewListItem );
void OSListInsertItemToEnd( tOSList_t * const ptList, tOSListItem_t * const ptNewListItem );
uOSBase_t OSListRemoveItem( tOSListItem_t * const ptItemToRemove );

#ifdef __cplusplus
}
#endif

#endif //__OS_TIMER_HPP
