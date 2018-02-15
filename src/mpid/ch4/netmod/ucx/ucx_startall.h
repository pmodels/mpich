/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */

#ifndef UCX_STARTALL_H_INCLUDED
#define UCX_STARTALL_H_INCLUDED
#include <ucp/api/ucp.h>
#include "ucx_impl.h"
#include "ucx_types.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_startall(int count, MPIR_Request * requests[])
{
    return MPIDIG_mpi_startall(count, requests);
}

#endif /* UCX_STARTALL_H_INCLUDED */
