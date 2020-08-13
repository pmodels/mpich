/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4R_REQUEST_H_INCLUDED
#define CH4R_REQUEST_H_INCLUDED

#include "ch4_types.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_request_create(MPIR_Request_kind_t kind,
                                                             int ref_count)
{
    MPIR_Request *req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_REQUEST_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_REQUEST_CREATE);

    req = MPIR_Request_create_from_pool(kind, 0, ref_count);
    if (req == NULL)
        goto fn_fail;

    MPIDI_NM_am_request_init(req);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_SHM_am_request_init(req);
#endif

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDIG_req_ext_t) <= MPIDIU_REQUEST_POOL_CELL_SIZE);
    MPIDU_genq_private_pool_alloc_cell(MPIDI_global.request_pool,
                                       (void **) &MPIDIG_REQUEST(req, req));
    MPIR_Assert(MPIDIG_REQUEST(req, req));
    MPIDIG_REQUEST(req, req->status) = 0;
    /* init the request as ready for data as this is the common case, CH4 will should set them to
     * false when needed */
    MPIDIG_REQUEST(req, recv_ready) = true;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_REQUEST_CREATE);
    return req;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_request_init(MPIR_Request * req,
                                                           MPIR_Request_kind_t kind)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_REQUEST_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_REQUEST_INIT);

    MPIR_Assert(req != NULL);
    /* Increment the refcount by one to account for the MPIDIG layer */
    MPIR_Request_add_ref(req);

    MPIDI_NM_am_request_init(req);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_SHM_am_request_init(req);
#endif

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDIG_req_ext_t) <= MPIDIU_REQUEST_POOL_CELL_SIZE);
    MPIDU_genq_private_pool_alloc_cell(MPIDI_global.request_pool,
                                       (void **) &MPIDIG_REQUEST(req, req));
    MPIR_Assert(MPIDIG_REQUEST(req, req));
    MPIDIG_REQUEST(req, req->status) = 0;
    /* init the request as ready for data as this is the common case, CH4 will should set them to
     * false when needed */
    MPIDIG_REQUEST(req, recv_ready) = true;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_REQUEST_INIT);
    return req;
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

    MPIR_Request *anysrc_partner = MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq);
    if (unlikely(anysrc_partner)) {
        if (!MPIR_STATUS_GET_CANCEL_BIT(anysrc_partner->status)) {
            if (MPIDI_REQUEST(rreq, is_local)) {
                /* SHM, cancel NM partner */
                mpi_errno = MPIDI_NM_mpi_cancel_recv(anysrc_partner);
                MPIR_ERR_CHECK(mpi_errno);
                if (!MPIR_STATUS_GET_CANCEL_BIT(anysrc_partner->status)) {
                    /* failed, cancel SHM rreq instead
                     * NOTE: comm and datatype will be defreferenced at caller site
                     */
                    MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
                    *is_cancelled = 0;
                } else {
                    /* NM partner is not user-visible, free it now, which means
                     * explicit SHM code can skip MPIDI_anysrc_free_partner
                     */
                    MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq) = NULL;
                    MPIDI_REQUEST_ANYSOURCE_PARTNER(anysrc_partner) = NULL;
                    /* cancel freed it once, freed once more on behalf of mpi-layer */
                    MPIR_Request_free_unsafe(anysrc_partner);
                }
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
    MPIR_Request *anysrc_partner = MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq);
    if (unlikely(anysrc_partner)) {
        if (!MPIDI_REQUEST(rreq, is_local)) {
            /* NM, complete and free SHM partner */
            MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq) = NULL;
            MPIDI_REQUEST_ANYSOURCE_PARTNER(anysrc_partner) = NULL;
            /* copy status to SHM partner */
            anysrc_partner->status = rreq->status;
            MPID_Request_complete(anysrc_partner);
            /* free NM request on behalf of mpi-layer
             * final free for NM request happen at call-site MPID_Request_complete
             * final free for SHM partner happen at mpi-layer
             */
            MPIR_Request_free_unsafe(rreq);
        } else {
            /* SHM, NM partner should already been freed (this branch can't happen) */
            MPIR_Assert(0);
        }
    }
}
#endif /* MPIDI_CH4_DIRECT_NETMOD */

#endif /* CH4R_REQUEST_H_INCLUDED */
