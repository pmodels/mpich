/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef POSIX_EAGER_DIRECT
#define POSIX_EAGER_DISABLE_INLINES

#include <mpidimpl.h>
#include <posix_eager.h>
#include "../../posix_types.h"
#include "posix_eager_direct.h"

MPIDI_POSIX_eager_funcs_t MPIDI_POSIX_eager_iqueue_funcs = {
    MPIDI_POSIX_eager_init,
    MPIDI_POSIX_eager_finalize,

    MPIDI_POSIX_eager_threshold,

    MPIDI_POSIX_eager_connect,
    MPIDI_POSIX_eager_listen,
    MPIDI_POSIX_eager_accept,

    MPIDI_POSIX_eager_send,

    MPIDI_POSIX_eager_recv_begin,
    MPIDI_POSIX_eager_recv_memcpy,
    MPIDI_POSIX_eager_recv_commit,

    MPIDI_POSIX_eager_recv_posted_hook,
    MPIDI_POSIX_eager_recv_completed_hook
};

#endif /* POSIX_EAGER_DIRECT */
