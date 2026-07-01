/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpidimpl.h"
#include "xpmem_pre.h"
#include "xpmem_types.h"

MPIDI_XPMEMI_global_t MPIDI_XPMEMI_global;

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_XPMEMI_DBG_GENERAL;
#endif
