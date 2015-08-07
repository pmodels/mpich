/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

#include "muteximpl.h"


/* -- Begin Profiling Symbol Block for routine MPIX_Mutex_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Mutex_free = PMPIX_Mutex_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Mutex_free  MPIX_Mutex_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Mutex_free as PMPIX_Mutex_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Mutex_free(MPIX_Mutex * hdl_ptr) __attribute__((weak,alias("PMPIX_Mutex_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Mutex_free
#define MPIX_Mutex_free PMPIX_Mutex_free
#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Mutex_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/** Free a group of MPI mutexes.  Collective on communicator used at the
  * time of creation.
  *
  * @param[in] hdl Handle to the group that will be freed
  * @return        MPI status
  */
int MPIX_Mutex_free(MPIX_Mutex * hdl_ptr)
{
    MPIX_Mutex hdl = *hdl_ptr;
    int i;

    for (i = 0; i < hdl->max_count; i++) {
        MPI_Win_free(&hdl->windows[i]);
    }

    if (hdl->bases != NULL) {
        for (i = 0; i < hdl->my_count; i++)
            MPI_Free_mem(hdl->bases[i]);

        free(hdl->bases);
    }

    MPI_Comm_free(&hdl->comm);
    free(hdl);
    hdl_ptr = NULL;

    return MPI_SUCCESS;
}
