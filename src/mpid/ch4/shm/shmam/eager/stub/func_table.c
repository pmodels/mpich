/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef SHMAM_EAGER_DIRECT
#define SHMAM_EAGER_DISABLE_INLINES

#include <mpidimpl.h>
#include <shmam_eager.h>
#include "shmam_eager_direct.h"

MPIDI_SHMAM_eager_funcs_t MPIDI_SHMAM_eager_stub_funcs = {
    MPIDI_SHMAM_eager_init,
    MPIDI_SHMAM_eager_finalize,

    MPIDI_SHMAM_eager_threshold,

    MPIDI_SHMAM_eager_connect,
    MPIDI_SHMAM_eager_listen,
    MPIDI_SHMAM_eager_accept,

    MPIDI_SHMAM_eager_send,

    MPIDI_SHMAM_eager_recv_begin,
    MPIDI_SHMAM_eager_recv_memcpy,
    MPIDI_SHMAM_eager_recv_commit,

    MPIDI_SHMAM_eager_recv_posted_hook,
    MPIDI_SHMAM_eager_recv_completed_hook,
    MPIDI_SHMAM_eager_anysource_posted_hook,
    MPIDI_SHMAM_eager_anysource_completed_hook
};

#endif /* SHMAM_EAGER_DIRECT */
