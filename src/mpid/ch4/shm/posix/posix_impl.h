/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_IMPL_H_INCLUDED
#define POSIX_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "mpidch4r.h"

#include "posix_types.h"
#include "posix_eager.h"
#include "posix_eager_impl.h"
#include "posix_progress.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_get_vsi(int flag, MPIR_Comm * comm_ptr,
                                                 int src_rank, int dst_rank, int tag)
{
    return MPIDI_get_vci(flag, comm_ptr, src_rank, dst_rank, tag) % MPIDI_POSIX_global.num_vsis;
}

#endif /* POSIX_IMPL_H_INCLUDED */
