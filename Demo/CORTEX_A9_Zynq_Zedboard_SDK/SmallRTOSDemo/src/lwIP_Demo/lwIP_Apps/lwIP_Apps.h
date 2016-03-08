#ifndef LWIP_APPS_H
#define LWIP_APPS_H

/* Functions used to obtain and release exclusive access to the Tx buffer.  The
Get function will block if the Tx buffer is not available - use with care! */
signed char *pcLwipAppsBlockingGetTxBuffer( void );
void vLwipAppsReleaseTxBuffer( void );

#endif /* LWIP_APPS_H */

