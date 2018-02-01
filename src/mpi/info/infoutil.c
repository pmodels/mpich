/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
MPIR_Info MPIR_Info_builtin[MPIR_INFO_N_BUILTIN] = { {0}
};
MPIR_Info MPIR_Info_direct[MPIR_INFO_PREALLOC] = { {0}
};

MPIR_Object_alloc_t MPIR_Info_mem = { 0, 0, 0, 0, MPIR_INFO,
    sizeof(MPIR_Info), MPIR_Info_direct,
    MPIR_INFO_PREALLOC,
};

/* Free an info structure.  In the multithreaded case, this routine
   relies on the SINGLE_CS in the info routines (particularly MPI_Info_free) */
#undef FUNCNAME
#define FUNCNAME MPIR_Info_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Info_free(MPIR_Info * info_ptr)
{
    MPIR_Info *curr_ptr, *last_ptr;

    curr_ptr = info_ptr->next;
    last_ptr = NULL;

    MPIR_Handle_obj_free(&MPIR_Info_mem, info_ptr);

    /* printf("Returning info %x\n", info_ptr->id); */
    /* First, free the string storage */
    while (curr_ptr) {
        MPL_free(curr_ptr->key);
        MPL_free(curr_ptr->value);
        last_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
        MPIR_Handle_obj_free(&MPIR_Info_mem, last_ptr);
    }
}

/* Allocate and initialize an MPIR_Info object.
 *
 * Returns MPICH error codes */
#undef FUNCNAME
#define FUNCNAME MPIR_Info_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Info_alloc(MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    *info_p_p = (MPIR_Info *) MPIR_Handle_obj_alloc(&MPIR_Info_mem);
    MPIR_ERR_CHKANDJUMP1(!*info_p_p, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPI_Info");

    MPIR_Object_set_ref(*info_p_p, 0);
    (*info_p_p)->next = NULL;
    (*info_p_p)->key = NULL;
    (*info_p_p)->value = NULL;

  fn_fail:
    return mpi_errno;
}
