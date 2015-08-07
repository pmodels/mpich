/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <strings.h>

#include <mpi.h>
#include "muteximpl.h"


/* -- Begin Profiling Symbol Block for routine MPIX_Mutex_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Mutex_create = PMPIX_Mutex_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Mutex_create  MPIX_Mutex_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Mutex_create as PMPIX_Mutex_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Mutex_create(int my_count, MPI_Comm comm, MPIX_Mutex * hdl_out) __attribute__((weak,alias("PMPIX_Mutex_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Mutex_create
#define MPIX_Mutex_create PMPIX_Mutex_create
#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Mutex_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/** Create a group of MPI mutexes.  Collective on the given communicator.
  *
  * @param[in]  count   Number of mutexes on the local process.
  * @param[in]  comm    MPI communicator on which to create mutexes
  * @param[out] hdl_out Handle to the mutex group
  * @return             MPI status
  */
int MPIX_Mutex_create(int my_count, MPI_Comm comm, MPIX_Mutex * hdl_out)
{
    int rank, nproc, max_count, i;
    MPIX_Mutex hdl;

    hdl = malloc(sizeof(struct mpixi_mutex_s));
    assert(hdl != NULL);

    MPI_Comm_dup(comm, &hdl->comm);

    MPI_Comm_rank(hdl->comm, &rank);
    MPI_Comm_size(hdl->comm, &nproc);

    hdl->my_count = my_count;

    /* Find the max. count to determine how many windows we need. */
    MPI_Allreduce(&my_count, &max_count, 1, MPI_INT, MPI_MAX, hdl->comm);
    assert(max_count > 0);

    hdl->max_count = max_count;
    hdl->windows = malloc(sizeof(MPI_Win) * max_count);
    assert(hdl->windows != NULL);

    if (my_count > 0) {
        hdl->bases = malloc(sizeof(uint8_t *) * my_count);
        assert(hdl->bases != NULL);
    }
    else {
        hdl->bases = NULL;
    }

    /* We need multiple windows here: one for each mutex.  Otherwise
     * performance will suffer due to exclusive access epochs. */
    for (i = 0; i < max_count; i++) {
        int size = 0;
        void *base = NULL;

        if (i < my_count) {
            MPI_Alloc_mem(nproc, MPI_INFO_NULL, &hdl->bases[i]);
            assert(hdl->bases[i] != NULL);
            memset(hdl->bases[i], 0, nproc);

            base = hdl->bases[i];
            size = nproc;
        }

        MPI_Win_create(base, size, sizeof(uint8_t), MPI_INFO_NULL, hdl->comm,
                       &hdl->windows[i]);
    }

    *hdl_out = hdl;
    return MPI_SUCCESS;
}
