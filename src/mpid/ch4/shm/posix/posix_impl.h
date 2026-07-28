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

/* Active message only need local vci since all messages go to the same per-vci queue */
#define MPIDI_POSIX_RECV_VSI(vci_) \
    do { \
        int vci_src_tmp; \
        MPIDI_EXPLICIT_VCIS(comm, attr, rank, comm->rank, vci_src_tmp, vci_); \
        if (vci_src_tmp == 0 && vci_ == 0) { \
            vci_ = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag); \
        } \
    } while (0)

#define MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci) \
    do { \
        if (!MPIDI_VCI_IS_EXPLICIT(vci)) { \
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(vci)); \
        } \
    } while (0)

#define MPIDI_POSIX_THREAD_CS_EXIT_VCI(vci) \
    do { \
        if (!MPIDI_VCI_IS_EXPLICIT(vci)) { \
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(vci)); \
        } \
    } while (0)

int MPIDI_POSIX_comm_bootstrap(MPIR_Comm * comm);
int MPIDI_POSIX_init_vci(int vci);
void MPIDI_POSIX_delay_shm_mutex_destroy(int rank, MPL_proc_mutex_t * shm_mutex_ptr);

#endif /* POSIX_IMPL_H_INCLUDED */
