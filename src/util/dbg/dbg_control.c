/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpichconf.h"
#include "mpimem.h"
#include <string.h>

/* FIXME: Delete this file (functionality switched to MPIU_DBG_MSG interface */

#define MAX_DEBUG_NAME 256
/* 
   This routine checks the argument against the value of the environment
   variable MPICH_DEBUG_ITEM, and if they are the same, returns 1.
   Otherwise, returns 0.
*/
int MPIR_IDebug( const char *str )
{
    static int needEnvValue = 1;
    const char *d;
    static char debugName[MAX_DEBUG_NAME];

    if (needEnvValue) {
	d = getenv( "MPICH_DEBUG_ITEM" );
	if (d) {
	    /* Make a copy */
	    MPIU_Strncpy( debugName, d, MAX_DEBUG_NAME );
	}
	else {
	    debugName[0] = 0;
	}
	needEnvValue = 0;
    }
    return strcmp( debugName, str ) == 0;
}

