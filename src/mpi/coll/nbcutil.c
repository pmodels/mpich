/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Sched_cb_free_buf
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Sched_cb_free_buf(MPID_Comm *comm, int tag, void *state)
{
    MPIU_Free(state);
    return MPI_SUCCESS;
}

