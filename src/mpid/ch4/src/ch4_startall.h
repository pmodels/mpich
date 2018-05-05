/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_STARTALL_H_INCLUDED
#define CH4_STARTALL_H_INCLUDED

#include "ch4_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Startall(int count, MPIR_Request * requests[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_STARTALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_STARTALL);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_startall(count, requests);
#else
    int i;
    for (i = 0; i < count; i++) {
        MPIR_Request *req = requests[i];
        /* This is sub-optimal, can we do better? */
        if (MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req)) {
            mpi_errno = MPIDI_SHM_mpi_startall(1, &req);
            if (mpi_errno == MPI_SUCCESS) {
                mpi_errno = MPIDI_NM_mpi_startall(1, &MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req));
                MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req->u.persist.real_request) =
                    MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req)->u.persist.real_request;
                MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER
                                                     (req)->u.persist.real_request) =
                    req->u.persist.real_request;
            }
        } else if (MPIDI_CH4I_REQUEST(req, is_local))
            mpi_errno = MPIDI_SHM_mpi_startall(1, &req);
        else
            mpi_errno = MPIDI_NM_mpi_startall(1, &req);
    }
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_STARTALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_STARTALL_H_INCLUDED */
