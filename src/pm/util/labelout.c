/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file defines routines that can be called by the pre and postamble
 * functions in MPIE_ForkProcesses to add output line labeling to stdout 
 * and to stderr. 
 *
 * These functions are very simple and assume that either stdout/err won't
 * block, or that causing mpiexec to block until stdout/err unblock is
 * acceptable.  A more sophisticated approach would queue up the output
 * and take advantage of the IOLoop routine (in ioloop.c) to control
 * output.
 */

/* Allow printf in label output */
/* style: allow:printf:1 sig:0 */

#include "pmutilconf.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "pmutil.h"
#include "ioloop.h"
#include "process.h"
#include "labelout.h"

#ifdef HAVE_SNPRINTF
#define MPIU_Snprintf snprintf
#ifdef NEEDS_SNPRINTF_DECL
/* style: allow:sprintf:1 sig:0 */
int snprintf(char *, size_t, const char *, ...);
#endif
#endif

#define MAX_LABEL 32

typedef struct { char label[MAX_LABEL]; int lastNL; FILE *dest; } IOLabel;

static int IOLabelWriteLine( int, int, void * );
static int IOLabelSetLabelText( const char [], char [], int, int, int );

/* labels for stdout and stderr, initialized to the default values */
static char outLabelPattern[MAX_LABEL] = "%W@[%w]@%d>";
static char errLabelPattern[MAX_LABEL] = "%W@[%w]@%d(err)>";
/* By default, labels are turned off */
static int  useLabels = 0;

void IOLabelSetDefault( int flag )
{
    useLabels = 1;
}

/* Redirect stdout and stderr to a handler */
int IOLabelSetupFDs( IOLabelSetup *iofds )
{
    /* Each pipe call creates 2 fds, 0 for reading, 1 for writing */
    if (pipe(iofds->readOut)) return -1;
    if (pipe(iofds->readErr)) return -1;
    return 0;
}

/* Close one side of each pipe pair and replace stdout/err with the pipes */
int IOLabelSetupInClient( IOLabelSetup *iofds )
{
    close( iofds->readOut[0] );
    close(1);
    dup2( iofds->readOut[1], 1 );
    close( iofds->readErr[0] );
    close(2);
    dup2( iofds->readErr[1], 2 );

    return 0;
}

/* Close one side of the pipe pair and register a handler for the I/O */
int IOLabelSetupFinishInServer( IOLabelSetup *iofds, ProcessState *pState )
{
    IOLabel *leader, *leadererr;

    close( iofds->readOut[1] );
    close( iofds->readErr[1] );

    /* We need dedicated storage for the private data */
    leader = (IOLabel *)MPIU_Malloc( sizeof(IOLabel) );
    if (useLabels) {
	IOLabelSetLabelText( outLabelPattern, 
			     leader->label, sizeof(leader->label),
			     pState->wRank, pState->app->pWorld->worldNum );
    } 
    else {
	leader->label[0] = 0;
    }
    leader->lastNL = 1;
    leader->dest   = stdout;
    leadererr = (IOLabel *)MPIU_Malloc( sizeof(IOLabel) );
    if (useLabels) {
	IOLabelSetLabelText( errLabelPattern, 
			     leadererr->label, sizeof(leadererr->label),
			     pState->wRank, pState->app->pWorld->worldNum );
    }
    else {
	leadererr->label[0] = 0;
    }
    leadererr->lastNL = 1;
    leadererr->dest   = stderr;
    MPIE_IORegister( iofds->readOut[0], IO_READ, IOLabelWriteLine, leader );
    MPIE_IORegister( iofds->readErr[0], IO_READ, IOLabelWriteLine, leadererr );

    /* subsequent forks should not get these fds */
    fcntl( iofds->readOut[0], F_SETFD, FD_CLOEXEC );
    fcntl( iofds->readErr[0], F_SETFD, FD_CLOEXEC );

    return 0;
}

static int IOLabelWriteLine( int fd, int rdwr, void *data )
{
    IOLabel *label = (IOLabel *)data;
    char buf[1024], *p;
    int n;

    MPIE_SYSCALL(n,read,( fd, buf, 1024 ));
    if (n == 0) {
	/* If read blocks, then returning a 0 is end-of-file */
	return 1;  /* ? EOF */
    }

    p = buf;
    while (n > 0) {
	int c;
	if (label->lastNL) {
	    if (label->label[0]) {
		fprintf( label->dest, "%s", label->label );
	    }
	    label->lastNL = 0;
	}
	c = *p++; n--;
	if (fputc( c, label->dest ) != c) return 1;
	label->lastNL = (c == '\n');
    }
    return 0;
}

/* 
   Get the prefix to use on output lines from the environment. 
   This is a common routine so that the same environment variables
   and effects can be used by many versions of mpiexec
   
   The choices are these:
   MPIEXEC_PREFIX_DEFAULT set:
       stdout gets "rank>"
       stderr gets "rank(err)>"
   MPIEXEX_PREFIX_STDOUT and MPIEXEC_PREFIX_STDERR replace those
   choices.
   
   The value can contain %d for the rank and eventually %w for the
   number of MPI_COMM_WORLD (e.g., 0 for the original one and > 0 
   for any subsequent spawned processes.  
   The syntax %W@...@ means:
   use @...@ as the syntax if world num is > 0, use NULL (i.e., no 
   characters) if world num is 0.  Thus, 
   
   %W@[%w]:@%d>

   for process 1 in the original MPI_COMM_WORLD gives 

   1>

   and for process 3 in a the second spawned process gives
   
   [2]:3>

   (%W takes the following char and uses that character to deliniate the
   %W string.  E.g., %W'...' and %W@...@ have the same effect, as long as
   the string between the deliminters contains neither @ nor '.

*/
/* Internal routine used to set the label text */
static int IOLabelSetLabelText( const char pattern[], char label[], 
				int maxlabel, int rank, int worldnum )
{
    const char *pin = pattern;
    char       *pout = label;
    int         dlen;
    int         lenleft = maxlabel-1;
    char        rankAsChar[12];
    char        worldnumAsChar[12];

    /* Convert the rank in world to characters */
    MPIU_Snprintf( rankAsChar, sizeof(rankAsChar), "%d", rank );
    /* Convert the world number to characters */
    MPIU_Snprintf( worldnumAsChar, sizeof(worldnumAsChar), "%d", worldnum );

    pout[0] = 0;
    /* Copy the pattern looking for format commands */
    while (lenleft > 0 && *pin) {
	if (*pin == '%') {
	    pin++;
	    /* Get the control */
	    switch (*pin) {
	    case '%': *pout++ = '%'; lenleft--; break;
	    case 'd': 
		dlen = strlen( rankAsChar );
		if (dlen < lenleft) {
		    MPIU_Strncpy( pout, rankAsChar, lenleft );
		    pout += dlen;
		    lenleft -= dlen;
		}
		else {
		    *pout++ = '*';
		    lenleft--;
		}
		break;
	    case 'w':
		dlen = strlen(worldnumAsChar);
		if (dlen < lenleft) {
		    MPIU_Strncpy( pout, worldnumAsChar, lenleft );
		    pout += dlen;
		    lenleft -= dlen;
		}
		else {
		    *pout++ = '*';
		    lenleft--;
		}
		break;
	    case 'W':
		{
		    char        delim = *++pin;
		    char        wPattern[MAX_LABEL], wLabel[MAX_LABEL];
		    char       *wptr = wPattern;

		    pin++;
		    while (*pin && *pin != delim && 
			   (wptr - wPattern) < MAX_LABEL) {
			*wptr++ = *pin++;
		    }
		    *wptr = 0;
		    if (worldnum > 0) {
			/* Recursively invoke the label routine */
			IOLabelSetLabelText( wPattern, wLabel, sizeof(wLabel), 
					     rank, worldnum );
			dlen = strlen(wLabel);
			if (dlen < lenleft) {
			    MPIU_Strncpy( pout, wLabel, lenleft );
			    pout    += dlen;
			    lenleft -= dlen;
			}
			else {
			    *pout++ = '*';
			    lenleft--;
			}
		    }
		}
		break;
	    default:
		/* Ignore the control */
		*pout++ = '%'; lenleft--; 
		if (lenleft--) *pout++ = *pin;
	    }
	    pin++;
	}
	else {
	    *pout++ = *pin++;
	    lenleft--;
	}
    }
    *pout = 0;

    return 0;
}

/* Check the environment for label control
   If either prefix is set, the labels are enabled.  If only one prefix
   is set, the default prefix is used for the other label */
int IOLabelCheckEnv( void )
{
    char *envval;
    envval = getenv( "MPIEXEC_PREFIX_STDOUT" );
    if (envval) {
	if (strlen(envval) < MAX_LABEL) {
	    MPIU_Strncpy( outLabelPattern, envval, MAX_LABEL );
	    useLabels = 1;
	}
	else {
	    MPIU_Error_printf( "Pattern for stdout label specified by MPIEXEC_PREFIX_STROUT is too long" );
	}
    }
    envval = getenv( "MPIEXEC_PREFIX_STDERR" );
    if (envval) {
	if (strlen(envval) < MAX_LABEL) {
	    MPIU_Strncpy( errLabelPattern, envval, MAX_LABEL );
	    useLabels = 1;
	}
	else {
	    MPIU_Error_printf( "Pattern for stderr label specified by MPIEXEC_PREFIX_STRERR is too long" );
	}
    }

    return 0;
}
