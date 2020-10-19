/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_INLINE_H_INCLUDED
#define NETMOD_INLINE_H_INCLUDED

#include "ofi_am.h"
#include "ofi_events.h"
#include "ofi_proc.h"
#include "ofi_unimpl.h"

#if defined(MPIDI_ENABLE_AM_ONLY)
#include "netmod_am_fallback_coll.h"
#else
#include "ofi_coll.h"
#endif

#if defined(MPIDI_ENABLE_AM_ONLY_PT2PT) || defined(MPIDI_ENABLE_AM_ONLY)
#include "netmod_am_fallback_send.h"
#include "netmod_am_fallback_recv.h"
#include "netmod_am_fallback_probe.h"
#else
#include "ofi_probe.h"
#include "ofi_recv.h"
#include "ofi_send.h"
#endif

#if defined(MPIDI_ENABLE_AM_ONLY_RMA) || defined(MPIDI_ENABLE_AM_ONLY)
#include "netmod_am_fallback_rma.h"
#else
#include "ofi_win.h"
#include "ofi_rma.h"
#endif

/* Not-inlined OFI netmod functions */
#include "ofi_noinline.h"

#endif /* NETMOD_INLINE_H_INCLUDED */
