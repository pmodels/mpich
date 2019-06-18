/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_COLL_SELECT_UTILS_H_INCLUDED
#define OFI_COLL_SELECT_UTILS_H_INCLUDED

/* Get the default container for a particular collective signature. */
MPL_STATIC_INLINE_PREFIX const void *MPIDI_NM_get_default_container(MPIDU_SELECTION_coll_id_t
                                                                    coll_id)
{
    return MPIDI_OFI_coll_default_container[coll_id];
}

#endif /* OFI_COLL_SELECT_UTILS_H_INCLUDED */
