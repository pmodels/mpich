/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_INLINE_H_INCLUDED
#define NETMOD_INLINE_H_INCLUDED

/* The sock netmod only implements the active message API. All native API
 * functions fall back to the MPIDIG implementation. */
#include "netmod_am_fallback.h"

#include "sock_am.h"
#include "sock_progress.h"
#include "sock_request.h"

/* Not-inlined SOCK netmod functions */
#include "sock_noinline.h"

#endif /* NETMOD_INLINE_H_INCLUDED */
