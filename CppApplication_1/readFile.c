#include "readFile.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <assert.h>
#include <ctype.h>

#define	MAXLINE 8192	/* max text line length */

#define	SA	struct sockaddr

#define CRLF "\x0d\x0a.\x0d\x0a"
#define CR   "\x0d\x0a"

int errorPass = 0;


/*
 * readConfFile gets the ip's outgoing server (which sends the
 * Outgoing mail) and the incoming server (where we connect to
 * Read mail)
 */

void readConfFile(const char * fileName, char * inServer, char * outServer)
{
    FILE * File;
    File = fopen( fileName, "r" );
    
    // server inServer: from where mails are downloaded
    fscanf( File, "%s", inServer );

    // servidor outServer: to whom we send mails
    fscanf( File, "%s", outServer );
}

int checkConn( char * inServer, int port )
{
	//
    // Check that the username and password are valid
    // Display error message or that they have accessed correctly
    //
    int	sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons( port );
    inet_pton(AF_INET, inServer, &servaddr.sin_addr);

    if ( !connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) ) return sockfd;
    else return -1;
}

void error(const char *msg) { perror(msg); exit(0); }

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t	n, rc;
    char	c, *ptr;
    
    ptr = vptr;
    for (n = 1; n < maxlen; n++)
    {
        if ( (rc = recv(fd, &c, 1, 0)) == 1)
        {
            *ptr++ = c;
            if (c == '\n') break;
        }
        else if (rc == 0)
        {
            if (n == 1) return(0);	/* EOF, no data read */
            else break;		/* EOF, some data was read */
        }
        else return(-1);	/* error */
    }

    *ptr = 0;
    return(n);
}

ssize_t Readline(int fd, void *ptr, size_t maxlen)
{
    ssize_t	n;

    if ( (n = readline(fd, ptr, maxlen)) == -1) error("readline error");
    return(n);
}


int checkUser( char * User, char * Pass, int sockfd)
{
    char recvline[MAXLINE], cmdUser [ 60 ]; bzero(cmdUser,60);

    strcat( cmdUser, "user " );
    strcat( cmdUser, User );
    strcat( cmdUser, "\n" );

    char cmdPass [ 60 ]; bzero(cmdPass,60);

    strcat( cmdPass, "pass " );
    strcat( cmdPass, Pass );
    strcat( cmdPass, "\n" );

    if ( ! errorPass )
        if (Readline(sockfd, recvline, MAXLINE) == 0)
            error("checkUser: server terminated prematurely");

    send(sockfd, cmdUser, strlen(cmdUser),0);

    if (Readline(sockfd, recvline, MAXLINE) == 0)
            error("checkUser: server terminated prematurely");

    send(sockfd, cmdPass, strlen(cmdPass), 0 );

    if (Readline(sockfd, recvline, MAXLINE) == 0)
            error("checkUser: server terminated prematurely");

    if ( recvline[ 0 ] == '-' )
    {
         fputs ( "\nUsuario o Contraseña incorrectos\n\n", stdout );
         errorPass = 1;
         return 0;
    }
    return 1;
}


static char encode(unsigned char u) {

  if(u < 26)  return 'A'+u;
  if(u < 52)  return 'a'+(u-26);
  if(u < 62)  return '0'+(u-52);
  if(u == 62) return '+';

  return '/';

}

static unsigned char decode(char c);


char *encode_base64(int size, unsigned char *src) {

  int i;
  char *out, *p;

  if(!src)
    return NULL;

  if(!size)
    size= strlen((char *)src);

  out= (char*)malloc(sizeof(char)/* size*4/3+4*/);

  p= out;

  for(i=0; i<size; i+=3) {

    unsigned char b1=0, b2=0, b3=0, b4=0, b5=0, b6=0, b7=0;

    b1 = src[i];

    if(i+1<size)
      b2 = src[i+1];

    if(i+2<size)
      b3 = src[i+2];

    b4= b1>>2;
    b5= ((b1&0x3)<<4)|(b2>>4);
    b6= ((b2&0xf)<<2)|(b3>>6);
    b7= b3&0x3f;

    *p++= encode(b4);
    *p++= encode(b5);

    if(i+1<size) {
      *p++= encode(b6);
    } else {
      *p++= '=';
    }

    if(i+2<size) {
      *p++= encode(b7);
    } else {
      *p++= '=';
    }

  }

  return out;

}

int sendMail( char * outServer, int port, char *user, char *pass )
{
    int sockfd = checkConn( outServer, port );


    if ( sockfd == -1 )
    {
        fputs ( "\nERROR DE CONEXIÓN: COMPRUEBE FICHERO DE CONFIGURACIÓN.\n", stdout );
        return 0;
    }
    // HELO
    char cmd[ 2 * MAXLINE ], recvline[ MAXLINE ];
    bzero( cmd, 2 * MAXLINE ); bzero( recvline, MAXLINE );

    strcat( cmd, "HELO SERVER\x0d\x0a" );

    if (send( sockfd, cmd, strlen( cmd ), 0 ) == -1) printf("ERROR");

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

    bzero( cmd, 2 * MAXLINE );
    strcat( cmd, "AUTH LOGIN\x0d\x0a" );
    if (send( sockfd, cmd, strlen( cmd ), 0 ) == -1) printf("ERROR");
    // autentficar

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

    // Base 64
    char *p = encode_base64(strlen(user), user);
    strcat(p,CR);
    if (send( sockfd, p, strlen( p ), 0 ) == -1) printf("ERROR");

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

     p = encode_base64(strlen(pass), pass); strcat(p,CR);

     if (send( sockfd, p, strlen( p ), 0 ) == -1) printf("ERROR");

      bzero( recvline, MAXLINE );
      if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

     // send commands

    char To[ 100 ], Subject[ 100 ], CC[ 100 ], BCC[ 100 ];
    bzero( To, 100 ); bzero( Subject, 100 ); bzero( CC, 100 ); bzero( BCC, 100 );

    printf( "Enter recipient email: " );
    scanf("%s", To );

    printf( "Enter subject: " );

    getc(stdin);

    fgets( Subject, 100, stdin );

    int option = 0;

    do
    {
        printf( "Send copies?:\n\t[ 0 ] - No\n\t[ 1 ] - CC\n\t[ 2 ] - BCC\n\t[ 3 ] - Both: " );

        scanf( "%d", &option );
        switch( option )
        {
            case 1: printf( "CC: " );  scanf("%s", CC );  break;

            case 2: printf( "BCC: " ); scanf("%s", BCC ); break;

            case 3: printf( "CC: " );  scanf("%s", CC );
                    printf( "BCC: " ); scanf("%s", BCC );
            break;

            default: break;
        }
    }
    while ( option < 1 && option > 3 );

    bzero( cmd, sizeof( strlen( cmd ) ) );
    strcat( cmd, "MAIL FROM:YOURUSER@DOMAIN.COM\x0d\x0a" ); // <<<<<<<<<<<<< ADD USER

    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

    if ( ! ( recvline[ 0 ] == '2' && recvline[ 1 ] == '5' && recvline[ 2 ] == '0' ) )
    {
        printf ( "\nMessage could not be sent: Error in MAIL FROM.\n" );
        return 0;
    }
    bzero( cmd, sizeof( strlen( cmd ) ) );
    strcat( cmd, "RCPT TO:" ); strcat( cmd, To ); strcat( cmd, CR );

    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

    if ( ! ( recvline[ 0 ] == '2' && recvline[ 1 ] == '5' && recvline[ 2 ] == '0' ) )
    {
        printf ( "\nMessage could not be sent: Error in RCPT TO.\n" );
        return 0;
    }
    //CC ---------------------------------------------
    bzero( cmd, sizeof( strlen( cmd ) ) );
    if ( strlen(CC) ) strcat( cmd,  "RCPT TO:" ); strcat( cmd, CC ); strcat( cmd, CR );

    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

    if ( ! ( recvline[ 0 ] == '2' && recvline[ 1 ] == '5' && recvline[ 2 ] == '0' ) )
    {
        printf ( "\nMessage could not be sent: Error in CC.\n" );
        return 0;
    }
    //Bcc ---------------------------------------------
    bzero( cmd, sizeof( strlen( cmd ) ) );
    if ( strlen(BCC) ) strcat( cmd, "RCPT TO:" ); strcat( cmd, BCC ); strcat( cmd, CR );

    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

    if ( ! ( recvline[ 0 ] == '2' && recvline[ 1 ] == '5' && recvline[ 2 ] == '0' ) )
    {
        printf ( "\nMessage could not be sent: Error in BCC.\n" );
        return 0;
    }

    bzero( cmd, sizeof( strlen( cmd ) ) );
    strcat( cmd, "DATA\x0d\x0a" );

    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("sendMail: server terminated prematurely");

    if ( ! ( recvline[ 0 ] == '3' && recvline[ 1 ] == '5' && recvline[ 2 ] == '4' ) )
    {
        printf ( "\nMessage could not be sent (Error in DATA).\n" );
        return 0;
    }



    bzero( cmd, strlen( cmd ) );
    strcat( cmd, "FROM:YOURUSER@DOMAIN.COM\x0d\x0a");
    int i, j;
    char AUX[ MAXLINE ];
    bzero(AUX,MAXLINE);
    for(i = 0, j = 0; i < strlen(To); i++)
    {
        if (To[i] == '\n')
        {
            AUX[j++]   = '\x0d';
            AUX[j++] = '\x0a';
        }
        else AUX[j++] = To[i];

    }
    bzero(To,strlen(To));
    //strcpy(To, AUX);
    strcat( cmd, "TO:" ); strcat( cmd, AUX ); strcat( cmd, CR );
    bzero( AUX, MAXLINE );
    for(i = 0, j = 0; i < strlen(Subject); i++)
    {
        if (Subject[i] == '\n')
        {
            AUX[j++]   = '\x0d';
            AUX[j++] = '\x0a';
        }
        else AUX[j++] = Subject[i];

    }
    strcat( cmd, "SUBJECT:" ); strcat( cmd, AUX ); //strcat( cmd, CR );
   
    if ( strlen( CC ) ) { strcat( cmd, "CC:" ); strcat( cmd, CC ); strcat( cmd, CR ); }
  
    if ( strlen( BCC ) ) { strcat( cmd, "BCC:" ); strcat( cmd, BCC ); strcat( cmd, CR );}

    strcat( cmd, CR );
   
    
    char MSG[ MAXLINE ];
    bzero( MSG, MAXLINE );

     
    printf( "Compose your message: \n\n" );

    getc(stdin);

   bzero(AUX,MAXLINE);
   char *Q;
   while (fgets( AUX, MAXLINE, stdin ))
   {

       if (Q = strstr(AUX,"__MF__"))
       {
            Q[0] = 0;
            strcat(MSG,AUX);
            printf ("%s\n",MSG);
            break;
       }
       strcat(MSG,AUX);

   }

   for(i = 0, j = 0; i < strlen(MSG); i++)
   {
       if (MSG[i] == '\n')
       {
           AUX[j++]   = '\x0d';
           AUX[j++] = '\x0a';
       }
       else AUX[j++] = MSG[i];
    }
    strcat( cmd, AUX );
    
    strcat( cmd, CRLF );



    send( sockfd, cmd, strlen( cmd ), 0 );
    //send( sockfd, MSG, strlen( MSG ), 0 );
    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("str_cli: server terminated prematurely");

    if ( ! ( recvline[ 0 ] == '2' && recvline[ 1 ] == '5' && recvline[ 2 ] == '0' ) )
    {
        printf ( "\nMessage could not be sent.\n" );
        return 0;
    }
    else printf ( "\nMessage sent correctly.\n" );

    bzero( cmd, strlen( cmd ) );
    strcat( cmd, "QUIT" ); strcat ( cmd, CR );
    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("str_cli: server terminated prematurely");

    return 1; // mail enviado con éxito
}


void getField( char * source, char * field )
{
    int i = 0, j = 0;

    while ( source[ i++ ] != ':' );

    while ( source[ i ] != '\r' ) field[ j++ ] = source[ i++ ];
}

void reverse(char s[])
{
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--)
     {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
}


void itoa(int n, char s[])
{
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}

/*
 * Returns 0 if mail has already been read or downloaded
 * 1 in other case.
 */
int notDownloaded( char * fileName )
{
      FILE *fp;
      char path[ 1035 ];

      bzero( path, 1035 );
      strcat(path, "/bin/ls ../MAILS/ | grep " );

      int i = 0;
      int length = strlen( fileName );
      for ( ; i < length; i++ ) 
          if( fileName[ i ] == 32
           || fileName[ i ] == '@'
           || fileName[ i ] == ','
           || fileName[ i ] == ':'
           || fileName[ i ] == '>'
           || fileName[ i ] == '<' ) fileName[ i ] = '_';

      strcat(path, ".txt" );

      fp = popen( path, "r");

      if (fp == NULL) return 0;
      bzero(path,1035);
      char aux[MAXLINE];
      /* Read the output a line at a time - output it. */
      while ( fgets( path, sizeof( path ) - 1, fp ) != NULL)
      {
          bzero(aux, MAXLINE); strcat(aux, "../MAILS/"); strcat(aux, path);
          if ( strstr ( aux, fileName ) )
          {
              pclose( fp );
              return 0;
          }
      }

      /* close */
      pclose(fp);
      
      return 1;
}

int readNewMail( int sockfd )
{
    int N = getNumberOfRemoteMsgs( sockfd );

    if ( ! N )
    {
        printf( "Warning: inbox is empty !!!" );
        return 0;
    }

    int mailNumber = 0;

    char * remoteFroms [ N ];
    char * remoteDates [ N ];

    getRemoteHeaders( sockfd, remoteFroms, NULL, remoteDates ); // show headers to client
    printf( "Read e-mail Number: " ); scanf( "%d", & mailNumber );

    if ( N < mailNumber )
    {
        printf( "There are %d, messages", N );
        return 0;
    }

    // DOWNLOAD MAIL TO LOCAL FOLDER

    char recvline[ MAXLINE ]; bzero( recvline, MAXLINE );
    char saveline[ MAXLINE ]; bzero( saveline, MAXLINE );
    char cmd[ 15 ];           bzero( cmd, 15 );

    char number[ 6 ];         bzero( number, 6 );
    itoa( mailNumber, number );

    strcat( cmd, "retr " ); strcat( cmd, number ); strcat( cmd, CR );

    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
            error( "readMail: server terminated prematurely" );

    int  startedReading = 0, isEnd = 0;
    char MSG[ MAXLINE ]; bzero( MSG, MAXLINE );

    int cr = 0, lf = 0;

    while ( 1 ) // get msg
    {
        if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
            error( "readMail: server terminated prematurely" );

        strcat( saveline, recvline );

        if ( recvline[ 0 ] == '\xd' ) cr++;
        else if( recvline[ 0 ] == '\xa' ) lf++;

        if ( cr == 2 || lf == 2 ) startedReading = 1;

        if( recvline[ 0 ] == '.' && startedReading )
            if( recvline[ 1 ] == '\xd' ) isEnd = 1;
        
        if ( startedReading && ! isEnd ) strcat( MSG, recvline );

        if ( isEnd ) break;
    }

    FILE * F;

    char fileName [ MAXLINE ]; bzero( fileName, MAXLINE );
    strcat(fileName, "../MAILS/");

    int n = getNumberOfLocalMsgs();

    strcat(fileName, remoteFroms[ mailNumber - 1 ] );
    strcat(fileName, remoteDates[ mailNumber - 1 ] );

    strcat(fileName, ".txt");

    int i = 0;
    for ( ; i < N; i++ )
    {
        free( remoteDates[ i ] );
        free( remoteFroms[ i ] );
    }

    if ( notDownloaded( fileName ) )
    {

        F = fopen( fileName, "w" );

        if ( F == NULL)
        {
            printf( "There has been an error when trying to read mail!\n" );
            return 0;
        }

        fprintf( F, "%s", saveline );

        fclose( F );

        F = fopen( "../MAILS/NMAILS", "w" );

        if ( F == NULL)
        {
            printf( "There has been an error when trying to read the chosen mail!\n" );
            return 0;
        }

        fscanf( F, "%d", &n ); n++;
        fprintf( F, "%d ", n );
        fclose( F );
    }

    printf( "Your message ...\n\n\n%s\n\n", MSG );

    bzero( cmd, 15 );

    strcat( cmd, "dele " ); strcat( cmd, number ); strcat( cmd, CR );

    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
            error( "readMail: server terminated prematurely" );

    return 0;
}

int getNumberOfLocalMsgs()
{
    FILE * F;

    F = fopen( "../MAILS/NMAILS", "r" );

    if ( F == NULL)
    {
        printf( "getNumberOfLocalMsgs: There has been an error when trying to read the chosen mail!\n" );
        return 0;
    }

    int n = 0;
    fscanf( F, "%d", &n );

    fclose( F );
    return n;
}

void getLocalHeaders( char * lsMail[] )
{
    int NoOfMessages = getNumberOfLocalMsgs(), index = 1;
    if( ! NoOfMessages ){ printf( "\n\tNo new mail\n" ); return; }

    char recvline[ MAXLINE ]; bzero( recvline, MAXLINE );
    char route[ MAXLINE ];

    char Subject[ MAXLINE ], Date[ MAXLINE ], From[ MAXLINE ];

    char number[ 6 ];

    printf( "N# - From:\t\t\t\tSubject:\t\t\t\tDate:\n" );
    
    FILE * F;

    FILE *fp;
    char path[1035];

    /* Open the command for reading. */
    fp = popen("/bin/ls ../MAILS/ | grep .txt", "r");
    if (fp == NULL) return;

    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path)-1, fp) != NULL)
    {
        path[ strlen( path ) - 1 ] = 0;

        lsMail[ index - 1 ] = strdup( path );

        bzero( route, MAXLINE ); strcat(route, "../MAILS/"); strcat(route, path);

        F = fopen( route, "r" );

        if ( F == NULL ) { printf("Error reading mail %s.", number); return; }

        int Exit = 0;

        while ( 1 )
        {
            fgets( recvline, MAXLINE, F );
            switch( recvline[ 0 ] )
            {
                case '\xd':
                    printf( "%d - ", index++ );
                    printf( "%s\t\t", From );
                    printf( "%s\t\t", Subject );
                    printf( "%s\n\n", Date );
                    Exit = 1;
                break;
                case 'S': bzero( Subject, MAXLINE ); getField( recvline, Subject ); break;
                case 'D': bzero( Date, MAXLINE );    getField( recvline, Date );    break;
                case 'F': bzero( From, MAXLINE );    getField( recvline, From );    break;
            }
            if( Exit ) { Exit = 0; break; }
        }

        fclose( F );

        NoOfMessages--;
    }

    /* close */
    pclose(fp);
}


int readOldMail()
{
    int N = getNumberOfLocalMsgs();

    if ( ! N )
    {
        printf( "Warning: inbox empty !!!" );
        return 0;
    }

    int mailNumber = 0;

    char * localMailList[ N ];
    getLocalHeaders( localMailList ); // show headers to client

    printf( "Read e-mail Number: " ); scanf( "%d", & mailNumber );

    if ( N < mailNumber )
    {
        printf( "There are %d, menssages", N );
        return 0;
    }
    
    FILE * F;

    char route[ MAXLINE ]; bzero( route, MAXLINE );
    strcat(route, "../MAILS/"); strcat( route, localMailList[ mailNumber - 1 ] );

    F = fopen( route, "r" );

    if ( F != NULL )
    {
        int  startedReading = 0, isEnd = 0;
        char MSG[ MAXLINE ]; bzero( MSG, MAXLINE );
        char recvline[ MAXLINE ]; bzero( recvline, MAXLINE );

        int cr = 0, lf = 0;
        while ( 1 ) // obtenemos el mensaje
        {
            fgets( recvline, MAXLINE, F );

            if ( recvline[ 0 ] == '\xd' ) cr++;
            else if( recvline[ 0 ] == '\xa' ) lf++;

            if ( cr == 2 || lf == 2 ) startedReading = 1;

            if( recvline[ 0 ] == '.' && startedReading )
                if( recvline[ 1 ] == '\xd' ) isEnd = 1;

            if ( startedReading && ! isEnd ) strcat( MSG, recvline );

            if ( isEnd ) break;
        }

        printf( "Your message ...\n\n\n%s\n\n", MSG );

        fclose( F );

        int i = 0;
        for ( ; i < N; i++ ) free( localMailList[ i ] );
        return 1;
    }
    else
    {
        printf( "There has been an error when trying to read mail!\n" );

        int i = 0;
        for ( ; i < N; i++ ) free( localMailList[ i ] );

        return 0;
    }
}

void getRemoteHeaders( int sockfd,
                       char * remoteFroms[],
                       char * remoteSubjects[],
                       char * remoteDates[] )
{
    const int NofMessages = getNumberOfRemoteMsgs( sockfd );
    if( ! NofMessages ){ printf( "\n\tNo new mail\n" ); return; }

    char recvline[ MAXLINE ]; bzero( recvline, MAXLINE );
    char cmd[ MAXLINE ]; bzero( cmd, MAXLINE );

    char Subject[ MAXLINE ], Date[ MAXLINE ], From[ MAXLINE ];

    int index = 1;

    char number[ 6 ];

    printf( "N# - From:\t\t\t\tSubject:\t\t\t\tDate:\n" );

    int b = 1;

    while ( index != NofMessages + 1 )
    {
        bzero( number, 6 ); itoa( index, number ); bzero( cmd, MAXLINE );
        strcat( cmd, "top " ); strcat( cmd, number ); strcat( cmd, " 0\x0d\x0a" );

        if ( b ) { send( sockfd, cmd, strlen( cmd ), 0 ); b = 0; }

        if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
            error("getHeaders: server terminated prematurely");

        switch( recvline[ 0 ] )
        {
            case '.':
                if ( remoteFroms )    remoteFroms[ index - 1 ]    = strdup( From );
                if ( remoteSubjects ) remoteSubjects[ index - 1 ] = strdup( Subject );
                if ( remoteDates )    remoteDates[ index - 1 ]    = strdup( Date );

                printf( "%d - ", index++ );
                printf( "%s\t\t", From );
                printf( "%s\t\t", Subject );
                printf( "%s\n\n", Date );
                b = 1;
            break;
            case 'S': bzero( Subject, MAXLINE ); getField( recvline, Subject ); break;
            case 'D': bzero( Date, MAXLINE );    getField( recvline, Date );    break;
            case 'F': bzero( From, MAXLINE );    getField( recvline, From );    break;
        }
    }
}

int getNumberOfRemoteMsgs( int sockfd )
{
    char recvline[ MAXLINE ]; bzero( recvline, MAXLINE );
    char cmd[ MAXLINE ]; bzero( cmd, MAXLINE );

    strcat( cmd, "stat\x0d\x0a" );

    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("getNumberOfMsgs: terminated prematurely");

    int i = 0, j = 0;

    char number[ 6 ]; bzero( number, 6 );

    while ( recvline[ i++ ] != 32 );

    while ( recvline[ i ] != 32 ) number[ j++ ] = recvline[ i++ ];

    return atoi( number );
}

int delFromServer( int sockfd )
{
    int N = getNumberOfRemoteMsgs( sockfd );

    if ( ! N )
    {
        printf( "Warning: inbox is empty !!!" );
        return 0;
    }

    printf("\n What mail do you want to delete ?\n");
    getRemoteHeaders( sockfd, NULL, NULL, NULL );
    int option = 0;
    printf("Mail number: ");
    scanf("%d", & option );

    if ( N < option )
    {
        printf( "There are just %d, Messages", N );
        return 0;
    }

    char cmd[ 15 ], number[ 6 ], recvline[ MAXLINE ];
    bzero( number, 6 ); bzero( cmd, 15 ); bzero( recvline, MAXLINE );
    itoa( option, number );
    strcat( cmd, "dele " ); strcat( cmd, number ); strcat( cmd, CR );

    send( sockfd, cmd, strlen( cmd ), 0 );

    if ( Readline( sockfd, recvline, MAXLINE ) == 0 )
        error("delFromServer: terminated prematurely");

    if ( recvline[ 0 ] == '+' ) printf ( "Message %d will be deleted when the session is ended\n", option );
    else printf( "There have been errors when trying to delete the message" );

    return 1;
}

int delFromLocal()
{
    int N = getNumberOfLocalMsgs();

    if ( ! N )
    {
        printf( "Warning: Inbox is empty !!!" );
        return 0;
    }

    printf("\n What mail do you want to delete ?\n");

    char * localMailList[ N ];
    getLocalHeaders( localMailList );
    int option = 0;
    printf("Correo Número: ");
    scanf("%d", & option );

    if ( N < option )
    {
        printf( "There are just %d, Messages", N );
        return 0;
    }

    char cmd[ 100 ], number[ 6 ];
    bzero( number, 6 ); bzero( cmd, 100 );
    itoa( option, number );

    strcat( cmd, "rm -f ../MAILS/" ); strcat( cmd, localMailList[ option - 1 ] );
    
    system( cmd );

    FILE * F;

    F = fopen( "../MAILS/NMAILS", "w" );

    if ( F == NULL)
    {
        printf( "delFromLocal: Error when trying to delete msg in local!\n" );
        return 0;
    }

    N--;
    fprintf( F, "%d ", N );

    fclose( F );

    system("rm -f ../MAILS/*~");
    
    printf ( "Msg deleted correctly\n" );

    int i = 0;
    for ( ; i < N; i++ ) free( localMailList[ i ] ) ;

    return 1;
}
