
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_COLL_SELECT_UTILS_H_INCLUDED
#define SHM_COLL_SELECT_UTILS_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

/* Pick the default container for a particular type of collective. */
MPL_STATIC_INLINE_PREFIX const void *MPIDI_SHM_get_default_container(MPIDU_SELECTION_coll_id_t
                                                                     coll_id)
{
    const void *container;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_BARRIER);

    container = MPIDI_POSIX_get_default_container(coll_id);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_BARRIER);
    return container;
}

#endif /* SHM_COLL_SELECT_UTILS_H_INCLUDED */
