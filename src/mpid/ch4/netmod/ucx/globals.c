/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#include <mpidimpl.h>
#include "ucx_impl.h"
#include "ucx_types.h"
#include "ucx_datatype.h"

MPIDI_UCX_global_t MPIDI_UCX_global = { 0 };

ucp_generic_dt_ops_t MPIDI_UCX_datatype_ops = {
    .start_pack = MPIDI_UCX_Start_pack,
    .start_unpack = MPIDI_UCX_Start_unpack,
    .packed_size = MPIDI_UCX_Packed_size,
    .pack = MPIDI_UCX_Pack,
    .unpack = MPIDI_UCX_Unpack,
    .finish = MPIDI_UCX_Finish_pack
};
