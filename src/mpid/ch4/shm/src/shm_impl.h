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

#ifdef MPIDI_ENABLE_AM_ONLY
#include "shm_am_fallback.h"
#else
#include "shm_coll.h"
#include "shm_p2p.h"
#include "shm_rma.h"
#include "shm_part.h"
#include "shm_hooks.h"
#endif

#include "shm_am.h"
#include "shm_progress.h"
#include "shm_hooks_internal.h"

/* Not-inlined shm functions */
#include "shm_noinline.h"

#endif /* SHM_IMPL_H_INCLUDED */
