/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "mpidig.h"
#include "ch4_impl.h"
#include "mpidch4r.h"

MPIDIG_global_t MPIDIG_global = { {0}
};

#undef FUNCNAME
#define FUNCNAME MPIDIG_comm_abort
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_comm_abort(MPIR_Comm * comm, int exit_code)
{
    int mpi_errno = MPI_SUCCESS;
    int dest;
    int size = 0;
    MPIR_Request *sreq = NULL;
    MPIDI_CH4U_hdr_t am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_COMM_ABORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_COMM_ABORT);

    am_hdr.src_rank = comm->rank;
    am_hdr.tag = exit_code;
    am_hdr.context_id = comm->context_id + MPIR_CONTEXT_INTRA_PT2PT;

    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        size = comm->local_size;
    } else {
        size = comm->remote_size;
    }

    for (dest = 0; dest < size; dest++) {
        if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM && dest == comm->rank)
            continue;

        mpi_errno = MPI_SUCCESS;
        sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__SEND, 2);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        mpi_errno = MPIDI_NM_am_isend(dest, comm, MPIDI_CH4U_COMM_ABORT, &am_hdr,
                                      sizeof(am_hdr), NULL, 0, MPI_INT, sreq);
        if (mpi_errno)
            continue;
        else
            MPIR_Wait_impl(sreq, MPI_STATUSES_IGNORE);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_COMM_ABORT);
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        MPL_exit(exit_code);

    return MPI_SUCCESS;
  fn_fail:
    goto fn_exit;
}
