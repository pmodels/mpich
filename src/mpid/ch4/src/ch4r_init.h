/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4R_INIT_H_INCLUDED
#define CH4R_INIT_H_INCLUDED

int MPIDIG_init_comm(MPIR_Comm * comm);
int MPIDIG_destroy_comm(MPIR_Comm * comm);
void *MPIDIG_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr);
int MPIDIG_mpi_free_mem(void *ptr);

#endif /* CH4R_INIT_H_INCLUDED */
