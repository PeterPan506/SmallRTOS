
/* Standard includes. */
#include "stdlib.h"
#include "string.h"

/* lwIP core includes */
#include "lwip/opt.h"
#include "lwip/sockets.h"

#include "SmallRTOS.h"


void TCPIPDemoTask( void *pvParameters )
{
long lSocket, lClientFd, lBytes, lAddrLen = sizeof( struct sockaddr_in );//, lInputIndex;
struct sockaddr_in sLocalAddr;
struct sockaddr_in client_addr;
char buf[1000];

	( void ) pvParameters;

	lSocket = lwip_socket(AF_INET, SOCK_STREAM, 0);

	if( lSocket >= 0 )
	{
		memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));
		sLocalAddr.sin_family = AF_INET;
		sLocalAddr.sin_len = sizeof(sLocalAddr);
		sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		sLocalAddr.sin_port = ntohs( ( ( unsigned short ) TCP_PORT ) );

		if( lwip_bind( lSocket, ( struct sockaddr *) &sLocalAddr, sizeof( sLocalAddr ) ) < 0 )
		{
			lwip_close( lSocket );
			OSTaskDelete(NULL);
		}

		if( lwip_listen( lSocket, 20 ) != 0 )
		{
			lwip_close( lSocket );
			OSTaskDelete(NULL);
		}

		for( ;; )
		{

			lClientFd = lwip_accept(lSocket, ( struct sockaddr * ) &client_addr, ( u32_t * ) &lAddrLen );

			if( lClientFd > 0L )
			{
				do
				{
					lBytes = lwip_recv( lClientFd, buf, 1000, 0 );

					if( lBytes >= 0L )
					{
						lwip_send( lClientFd, buf, lBytes, 0 );
					}
					else
					{
						lwip_close( lClientFd );
						break;
					}

				} while( lBytes > 0L );

				 lwip_close( lClientFd );
			}
		}
	}

	/* Will only get here if a listening socket could not be created. */
	OSTaskDelete(NULL);
}

