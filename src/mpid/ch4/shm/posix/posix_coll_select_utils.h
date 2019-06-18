/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_COLL_SELECT_UTILS_H_INCLUDED
#define POSIX_COLL_SELECT_UTILS_H_INCLUDED

/* Pick the default container for a particular type of collective. */
MPL_STATIC_INLINE_PREFIX const void
*MPIDI_POSIX_get_default_container(MPIDU_SELECTION_coll_id_t coll_id)
{
    return MPIDI_POSIX_coll_default_container[coll_id];
}

#endif /* POSIX_COLL_SELECT_UTILS_H_INCLUDED */
