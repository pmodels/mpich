/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_INLINE_H_INCLUDED
#define NETMOD_INLINE_H_INCLUDED

#ifdef MPIDI_ENABLE_AM_ONLY
#include "netmod_am_fallback.h"
#else
#include "ucx_send.h"
#include "ucx_recv.h"
#include "ucx_probe.h"
#include "ucx_win.h"
#include "ucx_rma.h"
#include "ucx_coll.h"
#include "ucx_part.h"
#endif

#include "ucx_progress.h"
#include "ucx_request.h"
#include "ucx_am.h"
#include "ucx_proc.h"

/* Not-inlined UCX netmod functions */
#include "ucx_noinline.h"

#endif /* NETMOD_INLINE_H_INCLUDED */
