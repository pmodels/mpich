/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_REQUEST_H_INCLUDED
#define CH4_REQUEST_H_INCLUDED

#include "ch4_impl.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX int MPID_Request_is_anysource(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPID_Request_is_pending_failure(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_set_completed(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    MPIR_cc_set(&req->cc, 0);

    MPIR_FUNC_EXIT;
    return;
}

/* These request functions should be called by the MPI layer only
   since they only do base initialization of the request object.
   A few notes:

   It is each layer's responsibility to initialize a request
   properly.

   The CH4I_request functions are even more bare bones.
   They create request objects that are not usable by the
   lower layers until further initialization takes place.

   MPIDIG_request_xxx functions can be used to create and destroy
   request objects at any CH4 layer, including shmmod and netmod.
   These functions create and initialize a base request with
   the appropriate "above device" fields initialized, and any
   required CH4 layer fields initialized.

   The net/shm mods can upcall to MPIDIG to create a request, or
   they can initialize their own requests internally, but note
   that all the fields from the upper layers must be initialized
   properly.

   Note that the request_release function is used by the MPI
   layer to release the ref on a request object.  It is important
   for the netmods to release any memory pointed to by the request
   when the internal completion counters hits zero, NOT when the
   ref hits zero or there will be a memory leak. The generic
   release function will not release any memory pointed to by
   the request because it does not know about the internals of
   the mpidig/netmod/shmmod fields of the request.
*/
MPL_STATIC_INLINE_PREFIX int MPID_Request_complete(MPIR_Request * req)
{
    int incomplete;

    MPIR_FUNC_ENTER;

    MPIR_cc_decr(req->cc_ptr, &incomplete);

    /* if we hit a zero completion count, free up AM-related
     * objects */
    if (!incomplete) {
        if (req->dev.completion_notification) {
            MPIR_cc_dec(req->dev.completion_notification);
        }

        if (req->dev.type == MPIDI_REQ_TYPE_AM) {
            /* FIXME: refactor mpidig code into ch4r_request.h */
            int vci = MPIDI_Request_get_vci(req);
            MPIDU_genq_private_pool_free_cell(MPIDI_global.per_vci[vci].request_pool,
                                              MPIDIG_REQUEST(req, req));
            MPIDIG_REQUEST(req, req) = NULL;
            MPIDI_NM_am_request_finalize(req);
#ifndef MPIDI_CH4_DIRECT_NETMOD
            MPIDI_SHM_am_request_finalize(req);
#endif
        }
        MPIDI_CH4_REQUEST_FREE(req);
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_Request_complete_fast(MPIR_Request * req)
{
    int incomplete;
    MPIR_cc_decr(req->cc_ptr, &incomplete);
    if (!incomplete) {
        if (req->dev.completion_notification) {
            MPIR_cc_dec(req->dev.completion_notification);
        }
        MPIDI_CH4_REQUEST_FREE(req);
    }
}

MPL_STATIC_INLINE_PREFIX void MPID_Prequest_free_hook(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    /* If a user passed a derived datatype for this persistent communication,
     * free it.
     * We could have done this cleanup in more general request cleanup functions,
     * like MPID_Request_destroy_hook. However, that would always add a few
     * instructions for any kind of request object, even if it's no a request
     * from persistent communications. */
    MPIR_Datatype_release_if_not_builtin(MPIDI_PREQUEST(req, datatype));

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX void MPID_Part_send_request_free_hook(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    MPIR_Datatype_release_if_not_builtin(MPIDI_PART_REQUEST(req, datatype));

    /* TG: FIXME this function is in MPIDI and not MPIDIG */
    MPIR_Assert(req->kind == MPIR_REQUEST_KIND__PART_SEND);
    MPIR_cc_t *cc_part = MPIDIG_PART_SREQUEST(req, cc_part);
    if (cc_part != NULL) {
        MPL_free(cc_part);
    }

    /* release the counter per msg */
    MPIR_cc_t *cc_msg = MPIDIG_PART_SREQUEST(req, cc_msg);
    if (cc_msg) {
        MPL_free(cc_msg);
    }

    /* if we do tag, release the request array and decrease the cc value */
    const bool do_tag = MPIDIG_PART_DO_TAG(req);
    if (do_tag) {
        MPIR_Request **tag_req_ptr = MPIDIG_PART_SREQUEST(req, tag_req_ptr);
        MPIR_Assert(tag_req_ptr != NULL);
        const int msg_part = MPIDIG_PART_REQUEST(req, msg_part);
        for (int i = 0; i < msg_part; ++i) {
            if (tag_req_ptr[i]) {
                MPIR_Request_free(tag_req_ptr[i]);
            }
        }
        MPL_free(tag_req_ptr);
        MPIR_cc_dec(&req->comm->part_context_cc);
    }

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX void MPID_Part_recv_request_free_hook(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    MPIR_Datatype_release_if_not_builtin(MPIDI_PART_REQUEST(req, datatype));

    /* TG: FIXME this function is in MPIDI and not MPIDIG */
    MPIR_Assert(req->kind == MPIR_REQUEST_KIND__PART_RECV);
    const bool do_tag = MPIDIG_PART_DO_TAG(req);
    if (do_tag) {
        MPIR_Request **tag_req_ptr = MPIDIG_PART_RREQUEST(req, tag_req_ptr);
        const int msg_part = MPIDIG_PART_REQUEST(req, msg_part);
        for (int i = 0; i < msg_part; ++i) {
            if (tag_req_ptr[i]) {
                MPIR_Request_free(tag_req_ptr[i]);
            }
        }
        MPL_free(tag_req_ptr);
    } else {
        MPIR_cc_t *cc_part = MPIDIG_PART_RREQUEST(req, cc_part);
        MPL_free(cc_part);
    }

    MPIR_FUNC_EXIT;
}

#endif /* CH4_REQUEST_H_INCLUDED */
