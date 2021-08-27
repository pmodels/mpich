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

/* for RMA, vsi need be persistent with window */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_get_win_vsi(MPIR_Win * win)
{
    int win_idx = 0;
    return MPIDI_get_vci(SRC_VCI_FROM_SENDER, win->comm_ptr, 0, 0, win_idx) %
        MPIDI_POSIX_global.num_vsis;
}

#endif /* POSIX_IMPL_H_INCLUDED */
