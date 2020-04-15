/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_MISC_H_INCLUDED
#define SHM_MISC_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                     int *lpid_ptr, bool is_remote)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_COMM_GET_LPID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_COMM_GET_LPID);

    ret = MPIDI_POSIX_comm_get_lpid(comm_ptr, idx, lpid_ptr, is_remote);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_COMM_GET_LPID);
    return ret;
}

#endif /* SHM_MISC_H_INCLUDED */
