/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_INLINE_H_INCLUDED
#define NETMOD_INLINE_H_INCLUDED

#include "ofi_am.h"
#include "ofi_events.h"
#include "ofi_proc.h"
#include "ofi_progress.h"
#include "ofi_unimpl.h"

#ifdef MPIDI_ENABLE_AM_ONLY
#include "netmod_am_fallback.h"
#else
#include "ofi_coll.h"
#include "ofi_probe.h"
#include "ofi_recv.h"
#include "ofi_send.h"
#include "ofi_win.h"
#include "ofi_rma.h"
#include "ofi_part.h"
#endif

/* Not-inlined OFI netmod functions */
#include "ofi_noinline.h"

#endif /* NETMOD_INLINE_H_INCLUDED */
