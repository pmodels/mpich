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
#ifndef OFI_STARTALL_H_INCLUDED
#define OFI_STARTALL_H_INCLUDED

#include "ofi_impl.h"
#include <../mpi/pt2pt/bsendutil.h>

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_startall(int count, MPIR_Request * requests[])
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_STARTALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_STARTALL);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_startall(count, requests);
        goto fn_exit;
    }

    for (i = 0; i < count; i++) {
        MPIR_Request *const preq = requests[i];

        switch (MPIDI_OFI_REQUEST(preq, util.persist.type)) {
#ifdef MPIDI_BUILD_CH4_SHM
        case MPIDI_PTYPE_RECV:
            mpi_errno = MPIDI_NM_mpi_irecv(MPIDI_OFI_REQUEST(preq,util.persist.buf),
                                           MPIDI_OFI_REQUEST(preq,util.persist.count),
                                           MPIDI_OFI_REQUEST(preq,datatype),
                                           MPIDI_OFI_REQUEST(preq,util.persist.rank),
                                           MPIDI_OFI_REQUEST(preq,util.persist.tag),
                                           preq->comm,
                                           MPIDI_OFI_REQUEST(preq,util_id) - preq->comm->recvcontext_id,
                                           MPIDIU_comm_rank_to_av(preq->comm,
                                                                  MPIDI_OFI_REQUEST(preq,util.persist.rank)),
                                           &preq->u.persist.real_request);
            break;
#else
        case MPIDI_PTYPE_RECV:
            mpi_errno = MPID_Irecv(MPIDI_OFI_REQUEST(preq,util.persist.buf),
                                   MPIDI_OFI_REQUEST(preq,util.persist.count),
                                   MPIDI_OFI_REQUEST(preq,datatype),
                                   MPIDI_OFI_REQUEST(preq,util.persist.rank),
                                   MPIDI_OFI_REQUEST(preq,util.persist.tag),
                                   preq->comm,
                                   MPIDI_OFI_REQUEST(preq,util_id) - preq->comm->recvcontext_id,
                                   &preq->u.persist.real_request);
            break;
#endif

#ifdef MPIDI_BUILD_CH4_SHM
        case MPIDI_PTYPE_SEND:
            mpi_errno = MPIDI_NM_mpi_isend(MPIDI_OFI_REQUEST(preq,util.persist.buf),
                                           MPIDI_OFI_REQUEST(preq,util.persist.count),
                                           MPIDI_OFI_REQUEST(preq,datatype),
                                           MPIDI_OFI_REQUEST(preq,util.persist.rank),
                                           MPIDI_OFI_REQUEST(preq,util.persist.tag),
                                           preq->comm,
                                           MPIDI_OFI_REQUEST(preq,util_id) - preq->comm->context_id,
                                           MPIDIU_comm_rank_to_av(preq->comm,
                                                                  MPIDI_OFI_REQUEST(preq,util.persist.rank)),
                                           &preq->u.persist.real_request);
            break;
#else
        case MPIDI_PTYPE_SEND:
            mpi_errno = MPID_Isend(MPIDI_OFI_REQUEST(preq,util.persist.buf),
                                   MPIDI_OFI_REQUEST(preq,util.persist.count),
                                   MPIDI_OFI_REQUEST(preq,datatype),
                                   MPIDI_OFI_REQUEST(preq,util.persist.rank),
                                   MPIDI_OFI_REQUEST(preq,util.persist.tag),
                                   preq->comm,
                                   MPIDI_OFI_REQUEST(preq,util_id) - preq->comm->context_id,
                                   &preq->u.persist.real_request);
            break;
#endif

        case MPIDI_PTYPE_SSEND:
            mpi_errno = MPID_Issend(MPIDI_OFI_REQUEST(preq,util.persist.buf),
                                    MPIDI_OFI_REQUEST(preq,util.persist.count),
                                    MPIDI_OFI_REQUEST(preq,datatype),
                                    MPIDI_OFI_REQUEST(preq,util.persist.rank),
                                    MPIDI_OFI_REQUEST(preq,util.persist.tag),
                                    preq->comm,
                                    MPIDI_OFI_REQUEST(preq,util_id) - preq->comm->context_id,
                                    &preq->u.persist.real_request);
            break;

        case MPIDI_PTYPE_BSEND:{
                MPI_Request sreq_handle;
                mpi_errno = MPIR_Ibsend_impl(MPIDI_OFI_REQUEST(preq, util.persist.buf),
                                      MPIDI_OFI_REQUEST(preq, util.persist.count),
                                      MPIDI_OFI_REQUEST(preq, datatype),
                                      MPIDI_OFI_REQUEST(preq, util.persist.rank),
                                      MPIDI_OFI_REQUEST(preq, util.persist.tag),
                                      preq->comm, &sreq_handle);

                if (mpi_errno == MPI_SUCCESS)
                    MPIR_Request_get_ptr(sreq_handle, preq->u.persist.real_request);

                break;
            }

        default:
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __FUNCTION__,
                                      __LINE__, MPI_ERR_INTERN, "**ch3|badreqtype",
                                      "**ch3|badreqtype %d", MPIDI_OFI_REQUEST(preq,
                                                                               util.persist.type));
        }

        if (mpi_errno == MPI_SUCCESS) {
            preq->status.MPI_ERROR = MPI_SUCCESS;

            if (MPIDI_OFI_REQUEST(preq, util.persist.type) == MPIDI_PTYPE_BSEND) {
                preq->cc_ptr = &preq->cc;
                MPIR_cc_set(&preq->cc, 0);
            }
            else
                preq->cc_ptr = &preq->u.persist.real_request->cc;
        }
        else {
            preq->u.persist.real_request = NULL;
            preq->status.MPI_ERROR = mpi_errno;
            preq->cc_ptr = &preq->cc;
            MPIR_cc_set(&preq->cc, 0);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_STARTALL);
    return mpi_errno;
}

#endif /* OFI_STARTALL_H_INCLUDED */
