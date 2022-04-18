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

#define MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci) \
    do { \
        if (!MPIDI_VCI_IS_EXPLICIT(vci)) { \
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock); \
        } \
    } while (0)

#define MPIDI_POSIX_THREAD_CS_EXIT_VCI(vci) \
    do { \
        if (!MPIDI_VCI_IS_EXPLICIT(vci)) { \
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock); \
        } \
    } while (0)

void MPIDI_POSIX_delay_shm_mutex_destroy(int rank, MPL_proc_mutex_t * shm_mutex_ptr);

#endif /* POSIX_IMPL_H_INCLUDED */
