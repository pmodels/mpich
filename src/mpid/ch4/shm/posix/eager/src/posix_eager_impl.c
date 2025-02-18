/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_INLINE
#ifndef POSIX_EAGER_DISABLE_INLINES

#include "mpidimpl.h"
#include "posix_eager.h"

int MPIDI_POSIX_eager_init(int rank, int size)
{
    return MPIDI_POSIX_eager_func->init(rank, size);
}

int MPIDI_POSIX_eager_post_init(void)
{
    return MPIDI_POSIX_eager_func->post_init();
}

int MPIDI_POSIX_eager_set_vcis(MPIR_Comm * comm, int num_vcis)
{
    return MPIDI_POSIX_eager_func->set_vcis(comm, num_vcis);
}

int MPIDI_POSIX_eager_finalize(void)
{
    return MPIDI_POSIX_eager_func->finalize();
}

#endif
#endif
