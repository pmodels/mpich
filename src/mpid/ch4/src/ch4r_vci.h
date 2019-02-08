/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef CH4R_VCI_H_INCLUDED
#define CH4R_VCI_H_INCLUDED

#include "ch4r_vci_hash_types.h"

#if MPIDI_CH4_VCI_HASH == MPIDI_CH4_VCI_HASH_COMM_ONLY
static int vci_hash_type = MPIDI_CH4_VCI_HASH_COMM_ONLY;
#else
#error "unknown VCI hash"
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_vci_nm_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_nm_get(MPIR_Comm * comm, int dest, int tag)
{
    int vci;

    /* this switch statement would be discarded by a good compiler
     * when vci_hash_type is mapped to a constant. */
    switch (vci_hash_type) {
        case MPIDI_CH4_VCI_HASH_COMM_ONLY:
            /* use a single vci for now */
            vci = MPIDI_COMM(comm, hash).u.single.nm_vci;
            break;
        default:
            MPIR_Assert(0);
            break;
    }

    return vci;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_vci_shm_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_shm_get(MPIR_Comm * comm, int dest, int tag)
{
    int vci;

    /* this switch statement would be discarded by a good compiler
     * when vci_hash_type is mapped to a constant. */
    switch (vci_hash_type) {
        case MPIDI_CH4_VCI_HASH_COMM_ONLY:
            /* use a single vci for now */
            vci = MPIDI_COMM(comm, hash).u.single.shm_vci;
            break;
        default:
            MPIR_Assert(0);
            break;
    }

    return vci;
}

#endif /* CH4R_VCI_H_INCLUDED */
