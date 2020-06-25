/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifdef POSIX_EAGER_INLINE
/* this file is empty */
#else

#define POSIX_EAGER_DISABLE_INLINES

#include <mpidimpl.h>
#include <posix_eager.h>
#include "posix_eager_inline.h"

MPIDI_POSIX_eager_funcs_t MPIDI_POSIX_eager_stub_funcs = {
    MPIDI_POSIX_stub_init,
    MPIDI_POSIX_stub_finalize,

    MPIDI_POSIX_eager_send,

    MPIDI_POSIX_eager_recv_begin,
    MPIDI_POSIX_eager_recv_memcpy,
    MPIDI_POSIX_eager_recv_commit,

    MPIDI_POSIX_eager_recv_posted_hook,
    MPIDI_POSIX_eager_recv_completed_hook,

    MPIDI_POSIX_eager_payload_limit,
    MPIDI_POSIX_eager_buf_limit
};

#endif /* POSIX_EAGER_INLINE */
