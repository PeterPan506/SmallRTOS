/**********************************************************************************************************
SmallRTOS - Copyright (C) 2012~2016 SmallRTOS.ORG All rights reserved.
http://www.SmallRTOS.org - Documentation, latest information, license and contact details.
http://www.SmallRTOS.com - Commercial support, development, porting, licensing and training services.
***********************************************************************************************************/

#ifndef __FIT_TYPE_H
#define __FIT_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned   char    		uOS8_t;
typedef      char    			sOS8_t;
typedef unsigned   short  		uOS16_t;
typedef signed     short   		sOS16_t;
typedef unsigned   int    		uOS32_t;
typedef signed     int    		sOS32_t;


typedef		uOS32_t 				uOSStack_t;
typedef		sOS32_t 				sOSBase_t;
typedef		uOS32_t 				uOSBase_t;
typedef		uOS32_t 				uOSTick_t;

#define 	FITSTACK_GROWTH			( -1 )
#define		FITBYTE_ALIGNMENT		( 8 )

#ifdef __cplusplus
}
#endif

#endif //__FIT_TYPE_H
