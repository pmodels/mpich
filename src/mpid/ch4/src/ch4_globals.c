/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* All global ADI data structures need to go in this file */
/* reference them with externs from other files           */

#include <mpidimpl.h>
#include "ch4_impl.h"

MPIDI_CH4_Global_t MPIDI_CH4_Global;
MPIDI_av_table_t **MPIDI_av_table;
MPIDI_av_table_t *MPIDI_av_table0;

MPIDI_NM_funcs_t *MPIDI_NM_func;
MPIDI_NM_native_funcs_t *MPIDI_NM_native_func;

#ifdef MPIDI_BUILD_CH4_SHM
MPIDI_SHM_funcs_t *MPIDI_SHM_func;
MPIDI_SHM_native_funcs_t *MPIDI_SHM_native_func;
#endif

#ifdef MPID_DEVICE_DEFINES_THREAD_CS
pthread_mutex_t MPIDI_Mutex_lock[MPIDI_NUM_LOCKS];
#endif

/* Another weird ADI that doesn't follow convention */
static void init_comm() __attribute__ ((constructor));
static void init_comm()
{
    MPIR_Comm_fns = &MPIDI_CH4_Global.MPIR_Comm_fns_store;
    MPIR_Comm_fns->split_type = MPIDI_Comm_split_type;
}

MPL_dbg_class MPIDI_CH4_DBG_GENERAL;

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
MPL_dbg_class MPIDI_CH4_DBG_MAP;
MPL_dbg_class MPIDI_CH4_DBG_COMM;
MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif
