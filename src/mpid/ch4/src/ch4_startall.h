/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_STARTALL_H_INCLUDED
#define CH4_STARTALL_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPID_Startall(int count, MPIR_Request * requests[])
{
    int mpi_errno = MPI_SUCCESS, i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_STARTALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_STARTALL);

    for (i = 0; i < count; i++) {
        MPIR_Request *const preq = requests[i];
        /* continue if the source/dest is MPI_PROC_NULL */
        if (MPIDI_PREQUEST(preq, rank) == MPI_PROC_NULL)
            continue;

        switch (MPIDI_PREQUEST(preq, p_type)) {

            case MPIDI_PTYPE_RECV:
                mpi_errno = MPID_Irecv(MPIDI_PREQUEST(preq, buffer), MPIDI_PREQUEST(preq, count),
                                       MPIDI_PREQUEST(preq, datatype), MPIDI_PREQUEST(preq, rank),
                                       MPIDI_PREQUEST(preq, tag), preq->comm,
                                       MPIDI_prequest_get_context_offset(preq),
                                       &preq->u.persist.real_request);
                break;

            case MPIDI_PTYPE_SEND:
                mpi_errno = MPID_Isend(MPIDI_PREQUEST(preq, buffer), MPIDI_PREQUEST(preq, count),
                                       MPIDI_PREQUEST(preq, datatype), MPIDI_PREQUEST(preq, rank),
                                       MPIDI_PREQUEST(preq, tag), preq->comm,
                                       MPIDI_prequest_get_context_offset(preq),
                                       &preq->u.persist.real_request);
                break;

            case MPIDI_PTYPE_SSEND:
                mpi_errno = MPID_Issend(MPIDI_PREQUEST(preq, buffer), MPIDI_PREQUEST(preq, count),
                                        MPIDI_PREQUEST(preq, datatype), MPIDI_PREQUEST(preq, rank),
                                        MPIDI_PREQUEST(preq, tag), preq->comm,
                                        MPIDI_prequest_get_context_offset(preq),
                                        &preq->u.persist.real_request);
                break;

            case MPIDI_PTYPE_BSEND:
                mpi_errno =
                    MPIR_Bsend_isend(MPIDI_PREQUEST(preq, buffer), MPIDI_PREQUEST(preq, count),
                                     MPIDI_PREQUEST(preq, datatype), MPIDI_PREQUEST(preq, rank),
                                     MPIDI_PREQUEST(preq, tag), preq->comm,
                                     &preq->u.persist.real_request);
                if (mpi_errno == MPI_SUCCESS) {
                    preq->status.MPI_ERROR = MPI_SUCCESS;
                    preq->cc_ptr = &preq->cc;
                    /* bsend is local-complete */
                    MPIR_cc_set(preq->cc_ptr, 0);
                    goto fn_exit;
                }
                break;

            default:
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __FUNCTION__,
                                                 __LINE__, MPI_ERR_INTERN, "**ch4|badreqtype",
                                                 "**ch4|badreqtype %d", MPIDI_PREQUEST(preq,
                                                                                       p_type));
        }

        if (mpi_errno == MPI_SUCCESS) {
            preq->status.MPI_ERROR = MPI_SUCCESS;
            preq->cc_ptr = &preq->u.persist.real_request->cc;
        } else {
            preq->u.persist.real_request = NULL;
            preq->status.MPI_ERROR = mpi_errno;
            preq->cc_ptr = &preq->cc;
            MPID_Request_set_completed(preq);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_STARTALL);
    return mpi_errno;
}

#endif /* CH4_STARTALL_H_INCLUDED */
