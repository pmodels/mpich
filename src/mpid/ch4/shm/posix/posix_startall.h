/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_STARTALL_H_INCLUDED
#define POSIX_STARTALL_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_STARTALL)
static inline int MPIDI_POSIX_mpi_startall(int count, MPIR_Request * requests[])
{
    return MPIDIG_mpi_startall(count, requests);
}

#endif /* POSIX_STARTALL_H_INCLUDED */
