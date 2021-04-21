/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_info.h"
/*#include <stdio.h>*/

/* This is the utility file for info that contains the basic info items
   and storage management */
#ifndef MPIR_INFO_PREALLOC
#define MPIR_INFO_PREALLOC 8
#endif

/* Preallocated info objects */
MPIR_Info MPIR_Info_builtin[MPIR_INFO_N_BUILTIN];
MPIR_Info MPIR_Info_direct[MPIR_INFO_PREALLOC];

MPIR_Object_alloc_t MPIR_Info_mem = { 0, 0, 0, 0, 0, 0, MPIR_INFO,
    sizeof(MPIR_Info), MPIR_Info_direct,
    MPIR_INFO_PREALLOC,
    NULL, {0}
};

/* Free an info structure.  In the multithreaded case, this routine
   relies on the SINGLE_CS in the info routines (particularly MPI_Info_free) */
int MPIR_Info_free_impl(MPIR_Info * info_ptr)
{
    MPIR_Info *curr_ptr, *last_ptr;

    curr_ptr = info_ptr->next;
    last_ptr = NULL;

    MPIR_Info_handle_obj_free(&MPIR_Info_mem, info_ptr);

    /* printf("Returning info %x\n", info_ptr->id); */
    /* First, free the string storage */
    while (curr_ptr) {
        /* MPI_Info objects are allocated by normal MPL_direct_xxx() functions, so
         * they need to be freed by MPL_direct_free(), not MPL_free(). */
        MPL_direct_free(curr_ptr->key);
        MPL_direct_free(curr_ptr->value);
        last_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
        MPIR_Info_handle_obj_free(&MPIR_Info_mem, last_ptr);
    }
    return MPI_SUCCESS;
}

/* Allocate and initialize an MPIR_Info object.
 *
 * Returns MPICH error codes */
int MPIR_Info_alloc(MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    *info_p_p = (MPIR_Info *) MPIR_Info_handle_obj_alloc(&MPIR_Info_mem);
    MPIR_ERR_CHKANDJUMP1(!*info_p_p, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPI_Info");

    MPIR_Object_set_ref(*info_p_p, 0);
    (*info_p_p)->next = NULL;
    (*info_p_p)->key = NULL;
    (*info_p_p)->value = NULL;

  fn_fail:
    return mpi_errno;
}

/* Set up MPI_INFO_ENV.  This may be called before initialization and after
 * finalization by MPI_Info_create_env().  This routine must be thread-safe. */
void MPIR_Info_setup_env(MPIR_Info * info_ptr)
{
    /* FIXME: Currently this info object is left empty, we need to add data to
     * this as defined by the standard. */
    (void) info_ptr;
}
