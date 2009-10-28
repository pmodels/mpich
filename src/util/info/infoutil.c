/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
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
#undef FUNCNAME
#define FUNCNAME MPIU_Info_free
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIU_Info_free( MPID_Info *info_ptr )
{
    MPID_Info *curr_ptr, *last_ptr;

    curr_ptr = info_ptr->next;
    last_ptr = NULL;

    MPIU_Handle_obj_free(&MPID_Info_mem, info_ptr);

    /* printf( "Returning info %x\n", info_ptr->id ); */
    /* First, free the string storage */
    while (curr_ptr) {
	MPIU_Free(curr_ptr->key);
	MPIU_Free(curr_ptr->value);
	last_ptr = curr_ptr;
	curr_ptr = curr_ptr->next;
        MPIU_Handle_obj_free(&MPID_Info_mem, last_ptr);
    }
}

/* Allocate and initialize an MPID_Info object.
 *
 * Returns MPICH2 error codes */
#undef FUNCNAME
#define FUNCNAME MPIU_Info_alloc
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIU_Info_alloc(MPID_Info **info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    *info_p_p = (MPID_Info *)MPIU_Handle_obj_alloc(&MPID_Info_mem);
    MPIU_ERR_CHKANDJUMP1(!*info_p_p, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPI_Info");

    MPIU_Object_set_ref(*info_p_p, 0);
    (*info_p_p)->next  = NULL;
    (*info_p_p)->key   = NULL;
    (*info_p_p)->value = NULL;

fn_fail:
    return mpi_errno;
}

