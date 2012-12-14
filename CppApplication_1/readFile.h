/* 
 * File:   readFile.h
 * Author: root
 *
 * Created on 16 de marzo de 2011, 9:26
 */

#ifndef READFILE_H
#define	READFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


/* Following shortens all the type casts of pointer arguments */

/* Define bzero() as a macro if it's not in standard C library. */
#ifndef	HAVE_BZERO
#define	bzero(ptr,n) memset(ptr, 0, n)
#endif

/*
 *
 */

void error(const char *msg);

int sendMail( char * outServer, int port, char * user, char * pass );

int readNewMail( int sockfd );

int readOldMail();

int delFromServer( int sockfd );

int delFromLocal();

void getRemoteHeaders( int sockfd,
                        char * remoteFroms[],
                        char * remoteSubjects[],
                        char * remoteDates[] );

void getLocalHeaders( char * lsMail[] );

int getNumberOfRemoteMsgs( int sockfd );

int getNumberOfLocalMsgs();

ssize_t readline(int fd, void *vptr, size_t maxlen);

ssize_t Readline(int fd, void *ptr, size_t maxlen);

int checkUser( char * User, char * Pass, int sockfd );

void readConfFile( const char * fileName, char * inServer, char * outServer );

int checkConn( char * inServer, int port );

#endif	/* READFILE_H */

