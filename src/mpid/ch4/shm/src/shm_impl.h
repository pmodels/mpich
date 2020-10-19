/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * If inlining is turned off, this file will be used to call into the shared memory module. It will
 * use the function pointer structure to call the appropriate functions rather than directly
 * inlining them.
 */

#ifndef SHM_IMPL_H_INCLUDED
#define SHM_IMPL_H_INCLUDED

#if defined(MPIDI_ENABLE_AM_ONLY)
#include "shm_am_fallback_coll.h"
#else
#include "shm_coll.h"
#endif

#if defined(MPIDI_ENABLE_AM_ONLY_PT2PT) || defined(MPIDI_ENABLE_AM_ONLY)
#include "shm_am_fallback_send.h"
#include "shm_am_fallback_recv.h"
#include "shm_am_fallback_probe.h"
#else
#include "shm_p2p.h"
#endif

#if defined(MPIDI_ENABLE_AM_ONLY_RMA) || defined(MPIDI_ENABLE_AM_ONLY)
#include "shm_am_fallback_rma.h"
#else
#include "shm_rma.h"
#include "shm_hooks.h"
#endif

#include "shm_am.h"
#include "shm_init.h"
#include "shm_misc.h"
#include "shm_types.h"
#include "shm_control.h"
#include "shm_hooks_internal.h"

/* Not-inlined shm functions */
#include "shm_noinline.h"

#endif /* SHM_IMPL_H_INCLUDED */
