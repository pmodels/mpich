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
#ifndef CH4R_INIT_H_INCLUDED
#define CH4R_INIT_H_INCLUDED

int MPIDIG_init_comm(MPIR_Comm * comm);
int MPIDIG_destroy_comm(MPIR_Comm * comm);
void *MPIDIG_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr);
int MPIDIG_mpi_free_mem(void *ptr);

#endif /* CH4R_INIT_H_INCLUDED */
