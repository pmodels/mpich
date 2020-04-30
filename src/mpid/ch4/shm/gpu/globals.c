/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "gpu_pre.h"
#include "gpu_impl.h"

MPIDI_GPU_global_t MPIDI_GPU_global = { 0 };

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_CH4_SHM_GPU_IPC_GENERAL;
#endif
