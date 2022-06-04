/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_REQUEST_H_INCLUDED
#define MPIDIG_REQUEST_H_INCLUDED

#include "ch4_types.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX int MPIDIG_request_init_internal(MPIR_Request * req,
                                                          int local_vci, int remote_vci)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    req->dev.type = MPIDI_REQ_TYPE_AM;
    MPIDI_NM_am_request_init(req);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_SHM_am_request_init(req);
#endif

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDIG_req_ext_t) <= MPIDIU_REQUEST_POOL_CELL_SIZE);
    MPIDU_genq_private_pool_alloc_cell(MPIDI_global.per_vci[local_vci].request_pool,
                                       (void **) &MPIDIG_REQUEST(req, req));
    MPIR_Assert(MPIDIG_REQUEST(req, req));
    MPIDIG_REQUEST(req, req->local_vci) = local_vci;
    MPIDIG_REQUEST(req, req->remote_vci) = remote_vci;
    MPIDIG_REQUEST(req, req->status) = 0;
    MPIDIG_REQUEST(req, req->recv_async).data_copy_cb = NULL;
    MPIDIG_REQUEST(req, req->recv_async).recv_type = MPIDIG_RECV_NONE;

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_request_create(MPIR_Request_kind_t kind,
                                                             int ref_count,
                                                             int local_vci, int remote_vci)
{
    MPIR_Request *req;

    MPIR_FUNC_ENTER;

    MPIDI_CH4_REQUEST_CREATE(req, kind, local_vci, ref_count);
    if (req == NULL)
        goto fn_fail;

    MPIDIG_request_init_internal(req, local_vci, remote_vci);

  fn_exit:
    MPIR_FUNC_EXIT;
    return req;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_request_init(MPIR_Request * req, int local_vci, int remote_vci)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Assert(req != NULL);
    /* Increment the refcount by one to account for the MPIDIG layer */
    MPIR_Request_add_ref(req);

    mpi_errno = MPIDIG_request_init_internal(req, local_vci, remote_vci);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#ifndef MPIDI_CH4_DIRECT_NETMOD
/* mpi-layer request handle is connected to one of the request (the one with shm),
 * so we can't simply free one at matching time. Both need be completed and freed
 * at the same time.
 *
 * At matching time, we cancel one of them (but not freed).
 * At completion time, we complete and free both.
 */
/* Thread-safety: MPIDI_anysrc_try_cancel_partner need be called inside the correct
 * critical section, as both MPIDI_NM_mpi_cancel_recv and MPIDI_SHM_mpi_cancel_recv
 * are unsafe. This assumes netmod, shmmod and am share the same vci. If that changes,
 * here we need be called outside lock and shift the lock responsibility into netmod,
 * shmmod, and am */
MPL_STATIC_INLINE_PREFIX int MPIDI_anysrc_try_cancel_partner(MPIR_Request * rreq, int *is_cancelled)
{
    int mpi_errno = MPI_SUCCESS;

    *is_cancelled = 1;  /* overwrite if cancel fails */

    MPIR_Request *anysrc_partner = rreq->dev.anysrc_partner;
    if (unlikely(anysrc_partner)) {
        if (!MPIR_STATUS_GET_CANCEL_BIT(anysrc_partner->status)) {
            if (MPIDI_REQUEST(rreq, is_local)) {
                /* SHM, cancel NM partner */
                /* canceling a netmod recv request in libfabric ultimately
                 * has one of the two results: 1. COMPLETION event 2. or,
                 * FI_ECANCELED.
                 * MPIDI_NM_mpi_cancel_recv() may not wait for COMPLETION
                 * event when fi_cancel returns FI_NOENT. In other cases,
                 * progress call is invoked and recv_event may happen
                 * for recv completion. In this case, it frees up
                 * anysrc_partner in recv_event(). Therefore, increase
                 * ref count here to prevent free since here we will check
                 * the request status */
                MPIR_Request_add_ref(anysrc_partner);
                mpi_errno = MPIDI_NM_mpi_cancel_recv(anysrc_partner, true);     /* blocking */
                MPIR_ERR_CHECK(mpi_errno);
                if (!MPIR_STATUS_GET_CANCEL_BIT(anysrc_partner->status)) {
                    /* either complete or failed, cancel SHM rreq instead
                     * NOTE: comm and datatype will be defreferenced at caller site
                     */
                    MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
                    *is_cancelled = 0;
                } else {
                    /* NM partner is not user-visible, free it now, which means
                     * explicit SHM code can skip MPIDI_anysrc_free_partner
                     */
                    rreq->dev.anysrc_partner = NULL;
                    anysrc_partner->dev.anysrc_partner = NULL;
                    /* cancel freed it once, freed once more on behalf of mpi-layer */
                    MPIDI_CH4_REQUEST_FREE(anysrc_partner);
                }
                MPIDI_CH4_REQUEST_FREE(anysrc_partner);
            } else {
                /* NM, cancel SHM partner */
                /* prevent free, we'll complete it separately */
                MPIR_cc_inc(anysrc_partner->cc_ptr);
                mpi_errno = MPIDI_SHM_mpi_cancel_recv(anysrc_partner);
                MPIR_ERR_CHECK(mpi_errno);

                /* NOTE: We assume the cancel is always successful.
                 *       This is because SHM won't match unless NM cancels.
                 *       We only reach here when NM is still active.
                 */
            }
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_anysrc_free_partner(MPIR_Request * rreq)
{
    MPIR_Request *anysrc_partner = rreq->dev.anysrc_partner;
    if (unlikely(anysrc_partner)) {
        if (!MPIDI_REQUEST(rreq, is_local)) {
            /* NM, complete and free SHM partner */
            rreq->dev.anysrc_partner = NULL;
            anysrc_partner->dev.anysrc_partner = NULL;
            /* copy status to SHM partner */
            anysrc_partner->status = rreq->status;
            MPID_Request_complete(anysrc_partner);
            /* free NM request on behalf of mpi-layer
             * final free for NM request happen at call-site MPID_Request_complete
             * final free for SHM partner happen at mpi-layer
             */
            MPIDI_CH4_REQUEST_FREE(rreq);
        } else {
            /* SHM, NM partner should already been freed (this branch can't happen) */
            MPIR_Assert(0);
        }
    }
}
#endif /* MPIDI_CH4_DIRECT_NETMOD */

#endif /* MPIDIG_REQUEST_H_INCLUDED */
