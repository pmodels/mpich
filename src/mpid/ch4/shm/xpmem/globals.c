/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_pre.h"
#include "xpmem_impl.h"

MPIDI_XPMEM_global_t MPIDI_XPMEM_global = { 0 };

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_CH4_SHM_XPMEM_GENERAL;
#endif

/* Preallocated segment objects */
MPIDI_XPMEM_seg_t MPIDI_XPMEM_seg_mem_direct[MPIDI_XPMEM_SEG_PREALLOC] = { {0}
};

/* Preallocated counter objects */
MPIDI_XPMEM_cnt_t MPIDI_XPMEM_cnt_mem_direct[MPIDI_XPMEM_CNT_PREALLOC] = { {0}
};

MPIR_Object_alloc_t MPIDI_XPMEM_seg_mem = { 0, 0, 0, 0, MPIR_INTERNAL,
    sizeof(MPIDI_XPMEM_seg_t), MPIDI_XPMEM_seg_mem_direct,
    MPIDI_XPMEM_SEG_PREALLOC
};

MPIR_Object_alloc_t MPIDI_XPMEM_cnt_mem = { 0, 0, 0, 0, MPIR_INTERNAL,
    sizeof(MPIDI_XPMEM_cnt_t), MPIDI_XPMEM_cnt_mem_direct,
    MPIDI_XPMEM_CNT_PREALLOC
};
