/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_STUB_IMPL_H_INCLUDED
#define POSIX_EAGER_STUB_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include <posix_eager_transaction.h>

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_payload_limit(void)
{
    MPIR_Assert(0);
    return 0;
}

#endif /* POSIX_EAGER_STUB_IMPL_H_INCLUDED */
