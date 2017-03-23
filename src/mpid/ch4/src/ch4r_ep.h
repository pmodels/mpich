/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef CH4R_EP_H_INCLUDED
#define  CH4R_EP_H_INCLUDED

#include "ch4_types.h"

#undef FUNCNAME
#define FUNCNAME MPIDIU_endpoint_get_next
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_endpoint_get_next()
{
    int ret = MPIDI_CH4_Global.next_ep_idx;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ENDPOINT_GET_NEXT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ENDPOINT_GET_NEXT);
    MPIDI_CH4_Global.next_ep_idx += 1;
    if (MPIDI_CH4_Global.next_ep_idx == MPIDI_CH4_Global.n_netmod_eps) {
        MPIDI_CH4_Global.next_ep_idx = 0;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ENDPOINT_GET_NEXT);
    return ret;
}

#endif /* end of include guard:  CH4R_EP_H_INCLUDED */
