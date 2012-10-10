/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "process.h"
#include "pmiserv.h"
#include "ioloop.h"

/* This struct is used to setup the stdout/err handlers */
typedef struct { int readOut[2], readErr[2]; } iofds;

/* This combined struct is passed into forkprocess to setup both the
   stdout/err handlers and the PMI server support */
typedef struct {
    iofds        stdio;
    PMISetup     pmi;
} iosetup;

typedef struct { char label[32]; int lastNL; } IOLabel;

int mypreamble( void * );
int myprefork( void *, void *, ProcessState * );
int mypostamble( void *, void *, ProcessState * );
int labeler( int, int, void * );

/* 
   Test the argument parsing and processing routines.  See the Makefile
   for typical test choices 
*/
int main( int argc, char *argv[], char *envp[] )
{
    int             rc;
    iosetup         rundata;

    MPIE_ProcessInit();
    MPIE_Args( argc, argv, &pUniv, 0, 0 );
    MPIE_PrintProcessUniverse( stdout, &pUniv );
    
    PMIServInit(0,0);
    PMISetupNewGroup( pUniv.worlds[0].nProcess );
    MPIE_ForkProcesses( &pUniv.worlds[0], envp, mypreamble, &rundata,
			myprefork, 0, mypostamble, 0 );

    MPIE_IOLoop( 10 );
    
    rc = MPIE_ProcessGetExitStatus( );
    
    return rc;
}

/* Redirect stdout and stderr to a handler */
int mypreamble( void *data )
{
    iosetup *rundata = (iosetup *)data;
    /* Each pipe call creates 2 fds, 0 for reading, 1 for writing */
    if (pipe(rundata->stdio.readOut)) return -1;
    if (pipe(rundata->stdio.readErr)) return -1;

    /* Ask PMI to create the sockets that it wants */
    PMISetupSockets( 0, &rundata->pmi );
}
/* Close one side of each pipe pair and replace stdout/err with the pipes */
int myprefork( void *predata, void *data, ProcessState *pState )
{
    iosetup *rundata = (iosetup *)predata;

    close( rundata->stdio.readOut[0] );
    close(1);
    dup2( rundata->stdio.readOut[1], 1 );
    close( rundata->stdio.readErr[0] );
    close(2);
    dup2( rundata->stdio.readErr[1], 2 );

    /* Setup the sockets and communicate the information about the
       fd to the client right before we exec */
    PMISetupInClient( &rundata->pmi );
}
#include <fcntl.h>
/* Close one side of the pipe pair and register a handler for the I/O */
int mypostamble( void *predata, void *data, ProcessState *pState )
{
    iosetup *rundata = (iosetup *)predata;
    IOLabel *leader, *leadererr;

    close( rundata->stdio.readOut[1] );
    close( rundata->stdio.readErr[1] );

    /* We need dedicated storage for the private data */
    leader = (IOLabel *)malloc( sizeof(IOLabel) );
    snprintf( leader->label, sizeof(leader->label), "%d>", pState->wRank );
    leader->lastNL = 1;
    leadererr = (IOLabel *)malloc( sizeof(IOLabel) );
    snprintf( leadererr->label, sizeof(leadererr->label), "err%d>", 
	      pState->wRank );
    leadererr->lastNL = 1;
    MPIE_IORegister( rundata->stdio.readOut[0], IO_READ, labeler, leader );
    MPIE_IORegister( rundata->stdio.readErr[0], IO_READ, labeler, leadererr );

    /* subsequent forks should not get these fds */
    fcntl( rundata->stdio.readOut[0], F_SETFD, FD_CLOEXEC );
    fcntl( rundata->stdio.readErr[0], F_SETFD, FD_CLOEXEC );

    PMISetupFinishInServer( &rundata->pmi, pState );
}
int labeler( int fd, int rdwr, void *data )
{
    IOLabel *label = (IOLabel *)data;
    char buf[1024], *p;
    int n;

    n = read( fd, buf, 1024 );
    if (n == 0) {
	printf( "Closing fd %d\n", fd );
	return 1;  /* ? EOF */
    }

    p = buf;
    while (n > 0) {
	int c;
	if (label->lastNL) {
	    printf( "%s", label->label );
	    label->lastNL = 0;
	}
	c = *p++; n--;
	putchar(c);
	label->lastNL = c == '\n';
    }
    return 0;
}
void mpiexec_usage( const char *str )
{
    fprintf( stderr, "Usage: %s\n", str );
    return 0;
}
