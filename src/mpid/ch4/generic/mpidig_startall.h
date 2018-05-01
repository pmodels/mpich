/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef MPIDIG_STARTALL_H_INCLUDED
#define MPIDIG_STARTALL_H_INCLUDED

#include "ch4_impl.h"
#include <../mpi/pt2pt/bsendutil.h>

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_startall(int count, MPIR_Request * requests[])
{
    int mpi_errno = MPI_SUCCESS, i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_STARTALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_STARTALL);

    for (i = 0; i < count; i++) {
        MPIR_Request *const preq = requests[i];

        switch (MPIDI_CH4U_REQUEST(preq, p_type)) {

            case MPIDI_PTYPE_RECV:
                mpi_errno = MPID_Irecv(MPIDI_CH4U_REQUEST(preq, buffer),
                                       MPIDI_CH4U_REQUEST(preq, count),
                                       MPIDI_CH4U_REQUEST(preq, datatype),
                                       MPIDI_CH4U_REQUEST(preq, rank),
                                       MPIDI_CH4U_REQUEST(preq, tag),
                                       preq->comm,
                                       MPIDI_CH4U_request_get_context_offset(preq),
                                       &preq->u.persist.real_request);
                break;

            case MPIDI_PTYPE_SEND:
                mpi_errno = MPID_Isend(MPIDI_CH4U_REQUEST(preq, buffer),
                                       MPIDI_CH4U_REQUEST(preq, count),
                                       MPIDI_CH4U_REQUEST(preq, datatype),
                                       MPIDI_CH4U_REQUEST(preq, rank),
                                       MPIDI_CH4U_REQUEST(preq, tag),
                                       preq->comm,
                                       MPIDI_CH4U_request_get_context_offset(preq),
                                       &preq->u.persist.real_request);
                break;

            case MPIDI_PTYPE_SSEND:
                mpi_errno = MPID_Issend(MPIDI_CH4U_REQUEST(preq, buffer),
                                        MPIDI_CH4U_REQUEST(preq, count),
                                        MPIDI_CH4U_REQUEST(preq, datatype),
                                        MPIDI_CH4U_REQUEST(preq, rank),
                                        MPIDI_CH4U_REQUEST(preq, tag),
                                        preq->comm,
                                        MPIDI_CH4U_request_get_context_offset(preq),
                                        &preq->u.persist.real_request);
                break;

            case MPIDI_PTYPE_BSEND:{
                    MPI_Request sreq_handle;
                    mpi_errno = MPIR_Ibsend_impl(MPIDI_CH4U_REQUEST(preq, buffer),
                                                 MPIDI_CH4U_REQUEST(preq, count),
                                                 MPIDI_CH4U_REQUEST(preq, datatype),
                                                 MPIDI_CH4U_REQUEST(preq, rank),
                                                 MPIDI_CH4U_REQUEST(preq, tag),
                                                 preq->comm, &sreq_handle);
                    if (mpi_errno == MPI_SUCCESS)
                        MPIR_Request_get_ptr(sreq_handle, preq->u.persist.real_request);

                    break;
                }

            default:
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __FUNCTION__,
                                                 __LINE__, MPI_ERR_INTERN, "**ch3|badreqtype",
                                                 "**ch3|badreqtype %d", MPIDI_CH4U_REQUEST(preq,
                                                                                           p_type));
        }

        if (mpi_errno == MPI_SUCCESS) {
            preq->status.MPI_ERROR = MPI_SUCCESS;

            if (MPIDI_CH4U_REQUEST(preq, p_type) == MPIDI_PTYPE_BSEND) {
                preq->cc_ptr = &preq->cc;
                MPID_Request_set_completed(preq);
            } else
                preq->cc_ptr = &preq->u.persist.real_request->cc;
        } else {
            preq->u.persist.real_request = NULL;
            preq->status.MPI_ERROR = mpi_errno;
            preq->cc_ptr = &preq->cc;
            MPID_Request_set_completed(preq);
        }
        MPIR_Datatype_release_if_not_builtin(MPIDI_CH4U_REQUEST(preq, datatype));
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_STARTALL);
    return mpi_errno;
}

#endif /* MPIDIG_STARTALL_H_INCLUDED */
