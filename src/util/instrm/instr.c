/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef USE_MPIU_INSTR

static int MPIU_INSTR_Printf( FILE *fp );
static int MPIU_INSTR_Finalize( void *p );

/* */
/*
 * Basic but general support for instrumentation hooks in MPICH
 *
 * Most actions are handled by MPIU_INSTR_xxx macros (to permit both lowest
 * overhead and to allow instrumentation to be selected at compile time.
 */
static struct MPIU_INSTR_Generic_t *instrHead = 0, *instrTail = 0;

int MPIU_INSTR_AddHandle( void *handlePtr )
{
    struct MPIU_INSTR_Generic_t *gPtr = 
	(struct MPIU_INSTR_Generic_t *)handlePtr;

    /* Note that Addhandle must be within a thread-safe initialization */
    if (!instrHead) {
	/* Make sure that this call back occurs early (before MPID_Finalize) */
	MPIR_Add_finalize( MPIU_INSTR_Finalize, stdout,
			   MPIR_FINALIZE_CALLBACK_PRIO + 2 );
    }

    if (instrHead) {
	instrTail->next = gPtr;
    }
    else {
	instrHead       = gPtr;
    }
    instrTail       = gPtr;
    return 0;
}

#define MAX_INSTR_BUF 1024
static int MPIU_INSTR_Printf( FILE *fp )
{
    struct MPIU_INSTR_Generic_t *gPtr = instrHead;
    char instrBuf[MAX_INSTR_BUF];
    
    while (gPtr) {
	/* We only output information on events that occured */
	if (gPtr->count) {
	    if (gPtr->toStr) {
		(*gPtr->toStr)( instrBuf, sizeof(instrBuf), gPtr );
	    }
	    else {
		if (gPtr->desc) {
		    MPIU_Strncpy( instrBuf, gPtr->desc, sizeof(instrBuf) );
		}
		else {
		    /* This should not happen */
		    MPIU_Strncpy( instrBuf, "", sizeof(instrBuf) );
		}
	    }
	    fputs( instrBuf, fp );
	    fputc( '\n', fp );
	}
	gPtr = gPtr->next;
    }
    fflush( fp );
    return 0;
}

static int MPIU_INSTR_Finalize( void *p )
{
    int rc;
    struct MPIU_INSTR_Generic_t *gPtr = instrHead;
    /* FIXME: This should at least issue the writes in process order */
    /* Allow whether output is generated to be controlled */
    if (!MPL_env2bool( "MPICH_INSTR_AT_FINALIZE", &rc )) 
	rc = 0;

    if (rc) {
	MPIU_INSTR_Printf( stdout );
    }

    /* Free any memory allocated for the descriptions */
    while (gPtr) {
	if (gPtr->desc) {
	    MPIU_Free( (char *)gPtr->desc );
	    gPtr->desc = 0;
	}
	gPtr = gPtr->next;
    }
    
    return 0;
}

/*
 * Standard print routines for the instrumentation objects
 */

/* 
 * Print a duration, which may have extra integer fields.  Those fields
 * are printed as integers, in order, separate by tabs
 */
int MPIU_INSTR_ToStr_Duration_Count( char *buf, size_t maxBuf, void *ptr )
{
    double ttime;
    struct MPIU_INSTR_Duration_count_t *dPtr = 
	(struct MPIU_INSTR_Duration_count_t *)ptr;
    MPID_Wtime_todouble( &dPtr->ttime, &ttime );
    snprintf( buf, maxBuf, "%-40s:\t%d\t%e", dPtr->desc, dPtr->count, ttime );
    if (dPtr->nitems) {
	char *p;
	size_t  len = strlen(buf);
	int  i;
	/* Add each integer value, separated by a tab. */
	maxBuf -= len;
	p       = buf + len;
	for (i=0; i<dPtr->nitems; i++) {
	    snprintf( p, maxBuf, "\t%d", dPtr->data[i] );
	    len     = strlen(p);
	    maxBuf -= len;
	    p      += len;
	}
    }
    return 0;
}

/* Print the max counter value and the total counter value. */
int MPIU_INSTR_ToStr_Counter( char * buf, size_t maxBuf, void *ptr )
{
    struct MPIU_INSTR_Counter_t *dPtr = 
	(struct MPIU_INSTR_Counter_t *)ptr;
    snprintf( buf, maxBuf, "%-40s:\t%d\t%d", 
	      dPtr->desc, dPtr->maxcount, dPtr->totalcount );
    return 0;
}

#else
/* No routines required if instrumentation is not selected */
#endif
