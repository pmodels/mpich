/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "fbox_impl.h"
#include "fbox_types.h"

/* Note: without the following initialiazation, the common symbol may get lost
   during linking. This is manifested by static linking on mac osx.
*/
MPIDI_POSIX_eager_fbox_control_t MPIDI_POSIX_eager_fbox_control_global = { 0 };

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_CH4_SHM_POSIX_FBOX_GENERAL;
#endif
