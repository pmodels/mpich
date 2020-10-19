/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_INLINE_H_INCLUDED
#define NETMOD_INLINE_H_INCLUDED

#if defined(MPIDI_ENABLE_AM_ONLY)
#include "netmod_am_fallback_coll.h"
#else
#include "ucx_coll.h"
#endif

#if defined(MPIDI_ENABLE_AM_ONLY_PT2PT) || defined(MPIDI_ENABLE_AM_ONLY)
#include "netmod_am_fallback_send.h"
#include "netmod_am_fallback_recv.h"
#include "netmod_am_fallback_probe.h"
#else
#include "ucx_send.h"
#include "ucx_recv.h"
#include "ucx_probe.h"
#endif

#if defined(MPIDI_ENABLE_AM_ONLY_RMA) || defined(MPIDI_ENABLE_AM_ONLY)
#include "netmod_am_fallback_rma.h"
#else
#include "ucx_win.h"
#include "ucx_rma.h"
#endif

#include "ucx_request.h"
#include "ucx_am.h"
#include "ucx_proc.h"

/* Not-inlined UCX netmod functions */
#include "ucx_noinline.h"

#endif /* NETMOD_INLINE_H_INCLUDED */
