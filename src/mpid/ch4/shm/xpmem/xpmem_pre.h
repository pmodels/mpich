/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef XPMEM_PRE_H_INCLUDED
#define XPMEM_PRE_H_INCLUDED

#include <xpmem.h>

#define MPIDI_XPMEM_PERMIT_VALUE ((void *)0600)

typedef struct {
    xpmem_segid_t remote_segid;
    xpmem_apid_t apid;
} MPIDI_XPMEM_segmap_t;

typedef struct {
    int num_local;
    int local_rank;
    MPIR_Group *node_group_ptr; /* cache node group, used at win_create. */
    xpmem_segid_t segid;        /* my local segid associated with entire address space */
    MPIDI_XPMEM_segmap_t *segmaps;      /* remote seg info for every local processes. */
    size_t sys_page_sz;
} MPIDI_XPMEM_global_t;

extern MPIDI_XPMEM_global_t MPIDI_XPMEM_global;

#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_CH4_SHM_XPMEM_GENERAL;
#endif
#define XPMEM_TRACE(...) \
    MPL_DBG_MSG_FMT(MPIDI_CH4_SHM_XPMEM_GENERAL,VERBOSE,(MPL_DBG_FDEST, "XPMEM "__VA_ARGS__))

#endif /* XPMEM_PRE_H_INCLUDED */
