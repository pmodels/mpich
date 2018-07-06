/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef NETMOD_INLINE_H_INCLUDED
#define NETMOD_INLINE_H_INCLUDED

#include "ucx_progress.h"
#include "ucx_request.h"
#include "ucx_init.h"
#ifdef MPICH_UCX_AM_ONLY
#include "ucx_am_send.h"
#include "ucx_am_startall.h"
#include "ucx_am_recv.h"
#include "ucx_am_probe.h"
#else
#include "ucx_send.h"
#include "ucx_startall.h"
#include "ucx_recv.h"
#include "ucx_probe.h"
#endif
#include "ucx_win.h"
#include "ucx_rma.h"
#include "ucx_am.h"
#include "ucx_spawn.h"
#include "ucx_comm.h"
#include "ucx_datatype.h"
#include "ucx_op.h"
#include "ucx_proc.h"
#include "ucx_coll.h"
#endif /* NETMOD_INLINE_H_INCLUDED */
