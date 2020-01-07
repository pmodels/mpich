/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ch4_impl.h"

int MPIDIG_get_context_index(uint64_t context_id)
{
    int raw_prefix, idx, bitpos, gen_id;




    raw_prefix = MPIR_CONTEXT_READ_FIELD(PREFIX, context_id);
    idx = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;
    gen_id = (idx * MPIR_CONTEXT_INT_BITS) + (31 - bitpos);


    return gen_id;
}

uint64_t MPIDIG_generate_win_id(MPIR_Comm * comm_ptr)
{
    uint64_t ret;




    /* context id lower bits, window instance upper bits */
    ret = 1 + (((uint64_t) comm_ptr->context_id) |
               ((uint64_t) ((MPIDIG_COMM(comm_ptr, window_instance))++) << 32));


    return ret;
}
