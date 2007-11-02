/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: infoutil.c,v 1.10 2006/04/21 18:50:19 gropp Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"
/*#include <stdio.h>*/

/* This is the utility file for info that contains the basic info items
   and storage management */
#ifndef MPID_INFO_PREALLOC 
#define MPID_INFO_PREALLOC 8
#endif

/* Preallocated info objects */
MPID_Info MPID_Info_direct[MPID_INFO_PREALLOC] = { { 0 } };
MPIU_Object_alloc_t MPID_Info_mem = { 0, 0, 0, 0, MPID_INFO, 
				      sizeof(MPID_Info), MPID_Info_direct,
                                      MPID_INFO_PREALLOC, };

/* Free an info structure.  In the multithreaded case, this routine
   relies on the SINGLE_CS in the info routines (particularly MPI_Info_free) */
void MPIU_Info_free( MPID_Info *info_ptr )
{
    MPID_Info *curr_ptr, *last_ptr;

    curr_ptr = info_ptr->next;
    last_ptr = info_ptr;

    /* printf( "Returning info %x\n", info_ptr->id ); */
    /* First, free the string storage */
    while (curr_ptr) {
	MPIU_Free(curr_ptr->key);
	MPIU_Free(curr_ptr->value);
	last_ptr = curr_ptr;
	curr_ptr = curr_ptr->next;
    }

    /* Return info to the avail list */
    last_ptr->next	= (MPID_Info *)MPID_Info_mem.avail;
    MPID_Info_mem.avail	= (void *)info_ptr;
}
