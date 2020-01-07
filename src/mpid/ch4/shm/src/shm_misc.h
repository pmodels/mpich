/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_MISC_H_INCLUDED
#define SHM_MISC_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                     int *lpid_ptr, bool is_remote)
{
    int ret;

    ret = MPIDI_POSIX_comm_get_lpid(comm_ptr, idx, lpid_ptr, is_remote);

    return ret;
}

#endif /* SHM_MISC_H_INCLUDED */
