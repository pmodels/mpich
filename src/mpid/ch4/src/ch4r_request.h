/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4R_REQUEST_H_INCLUDED
#define CH4R_REQUEST_H_INCLUDED

#include "ch4_types.h"
#include "ch4r_buf.h"

static inline MPIR_Request *MPIDIG_request_create(MPIR_Request_kind_t kind, int ref_count)
{
    MPIR_Request *req;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_REQUEST_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_REQUEST_CREATE);

    req = MPIR_Request_create(kind, 0);
    if (req == NULL)
        goto fn_fail;

    /* as long as ref_count is a constant, any compiler should be able
     * to unroll the below loop.  when threading is not enabled, the
     * compiler should be able to combine the below individual
     * increments to a single increment of "ref_count - 1".
     *
     * FIXME: when threading is enabled, the ref_count increase is an
     * atomic operation, so it might be more inefficient.  we should
     * use a new API to increase the ref_count value instead of the
     * for loop. */
    for (i = 0; i < ref_count - 1; i++)
        MPIR_Request_add_ref(req);

    MPIDI_NM_am_request_init(req);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_SHM_am_request_init(req);
#endif

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDIG_req_ext_t) <= MPIDIU_BUF_POOL_SZ);
    MPIDIG_REQUEST(req, req) = (MPIDIG_req_ext_t *) MPIDIU_get_buf(MPIDI_global.buf_pool);
    MPIR_Assert(MPIDIG_REQUEST(req, req));
    MPIDIG_REQUEST(req, req->status) = 0;

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

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDIG_req_ext_t) <= MPIDIU_BUF_POOL_SZ);
    MPIDIG_REQUEST(req, req) = (MPIDIG_req_ext_t *) MPIDIU_get_buf(MPIDI_global.buf_pool);
    MPIR_Assert(MPIDIG_REQUEST(req, req));
    MPIDIG_REQUEST(req, req->status) = 0;

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
static inline int MPIDI_anysrc_try_cancel_partner(MPIR_Request * rreq, int *is_cancelled)
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
                    MPIR_Request_free(anysrc_partner);
                }
            } else {
                /* NM, cancel SHM partner */
                int c;
                /* prevent free, we'll complete it separately */
                MPIR_cc_incr(anysrc_partner->cc_ptr, &c);
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

static inline void MPIDI_anysrc_free_partner(MPIR_Request * rreq)
{
    MPIR_Request *anysrc_partner = MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq);
    if (unlikely(anysrc_partner)) {
        if (!MPIDI_REQUEST(rreq, is_local)) {
            /* NM, complete and free SHM partner */
            MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq) = NULL;
            MPIDI_REQUEST_ANYSOURCE_PARTNER(anysrc_partner) = NULL;
            MPID_Request_complete(anysrc_partner);
            /* copy status to SHM partner */
            anysrc_partner->status = rreq->status;
            /* free NM request on behalf of mpi-layer
             * final free for NM request happen at call-site MPID_Request_complete
             * final free for SHM partner happen at mpi-layer
             */
            MPIR_Request_free(rreq);
        } else {
            /* SHM, NM partner should already been freed (this branch can't happen) */
            MPIR_Assert(0);
        }
    }
}
#endif /* MPIDI_CH4_DIRECT_NETMOD */

#endif /* CH4R_REQUEST_H_INCLUDED */
