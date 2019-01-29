/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#include <mpidimpl.h>
#include "ucx_impl.h"
#include "ucx_types.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_vci_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDI_UCX_vci_alloc(int resource, int type, int properties, int *vci)
{
    if (type & MPIDI_VCI_EXCLUSIVE)
        *vci = -1;
    else {
        *vci = 0;       /* index of the global worker would be 0 */
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_vci_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDI_UCX_vci_free(int vci)
{
    /* The pool has only one shared, generic VNI */
    return;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_vci_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDI_NM_vci_alloc(int resource, int type, int properties, int *vci)
{
    return MPIDI_UCX_vci_alloc(resource, type, properties, vci);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_vci_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDI_NM_vci_free(int vci)
{
    return MPIDI_UCX_vci_free(vci);
}
