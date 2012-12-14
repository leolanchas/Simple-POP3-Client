
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "readFile.h"

#define	SA      struct sockaddr

#define	MAXLINE	8192	/* max text line length */

#define SMTP    587
#define POP3    110

#define HELP  "\nYou Can:\n\n\t [ 0 ] Read NEW mail                    \
               \n\t [ 1 ] Read OLD mail.\n\t [ 2 ] Send mail.          \
               \n\t [ 3 ] Delete NEW mail.\n\t [ 4 ] Delete OLD mail.  \
               \n\t [ 5 ] Exit.\n\nYour option: "

int main()
{

  
    printf( "Welcome to the experimental pop3 client\n\n" );

    // read IP addresses
    char * inServer  = ( char * ) malloc( sizeof ( char ) * 12 );
    char * outServer = ( char * ) malloc( sizeof ( char ) * 12 );
    readConfFile( "../client.txt", inServer, outServer );

    int	sockfd = checkConn( inServer, POP3 );
    if( sockfd == -1 ) return 0;

    // log in
    char User[ 50 ], Pass[ 50 ];
    do
    {
        bzero( User, 50 );
        printf( "Your user: " );
        scanf("%s", User );

        bzero( Pass, 50 );
        printf( "Your pass: " );
        scanf("%s", Pass );
    }
    while ( ! checkUser( User, Pass, sockfd ) );

    // show headers
    getRemoteHeaders( sockfd, NULL, NULL, NULL );

    // options

    int option = 0;

    while ( 1 )
    {
        printf( HELP );
        scanf( "%d", &option );

        switch( option )
        {
            case 0:  readNewMail( sockfd );       break;
            case 1:  readOldMail();               break;
            case 2:  sendMail( outServer, SMTP, User, Pass ); break;
            case 3:  delFromServer( sockfd );     break;
            case 4:  delFromLocal( sockfd );      break;
            case 5:  send( sockfd, "QUIT\x0d\x0a", strlen( "QUIT\x0d\x0a" ), 0 ); return 0;
            default: printf( "\nIt's been an error with your selection:" );
        }
    }

    return 0;
}
