/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef NETMOD_PTL_OP_H_INCLUDED
#define NETMOD_PTL_OP_H_INCLUDED

#include "ptl_impl.h"

static inline void MPIDI_NM_op_destroy(MPIR_Op * op_p)
{
    MPIR_Assert(0);
    return;
}

static inline void MPIDI_NM_op_commit(MPIR_Op * op_p)
{
    MPIR_Assert(0);
    return;
}

#endif /* NETMOD_PTL_OP_H_INCLUDED */
