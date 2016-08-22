/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef NETMOD_UCX_OP_H_INCLUDED
#define NETMOD_UCX_OP_H_INCLUDED

#include "ucx_impl.h"

static inline void MPIDI_NM_op_destroy(MPIR_Op * op_p)
{
    return;
}

static inline void MPIDI_NM_op_commit(MPIR_Op * op_p)
{
    return;
}

#endif /* NETMOD_UCX_OP_H_INCLUDED */
