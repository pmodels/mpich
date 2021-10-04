/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4R_UTIL_H_INCLUDED
#define CH4R_UTIL_H_INCLUDED

/* Checks to make sure that the specified request is the next one expected to finish. If it isn't
 * supposed to finish next, it is appended to a list of requests to be retrieved later. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_check_cmpl_order(MPIR_Request * req, int vci)
{
    int ret = 0;

    MPIR_FUNC_ENTER;

    if (MPIDIG_REQUEST(req, req->seq_no) ==
        MPL_atomic_load_uint64(&MPIDI_global.per_vci[vci].exp_seq_no)) {
        MPL_atomic_fetch_add_uint64(&MPIDI_global.per_vci[vci].exp_seq_no, 1);
        ret = 1;
        goto fn_exit;
    }

    MPIDIG_REQUEST(req, req->request) = req;
    DL_APPEND(MPIDI_global.per_vci[vci].cmpl_list, req->dev.ch4.am.req);

  fn_exit:
    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_progress_compl_list(int vci)
{
    MPIR_Request *req;
    MPIDIG_req_ext_t *curr, *tmp;

    MPIR_FUNC_ENTER;

  do_check_again:
    DL_FOREACH_SAFE(MPIDI_global.per_vci[vci].cmpl_list, curr, tmp) {
        if (curr->seq_no == MPL_atomic_load_uint64(&MPIDI_global.per_vci[vci].exp_seq_no)) {
            DL_DELETE(MPIDI_global.per_vci[vci].cmpl_list, curr);
            req = curr->request;
            MPIDIG_REQUEST(req, req->target_cmpl_cb) (req);
            goto do_check_again;
        }
    }
    MPIR_FUNC_EXIT;
}

#endif /* CH4R_UTIL_H_INCLUDED */
