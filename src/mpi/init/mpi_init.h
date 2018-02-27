/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPI_INIT_H_INCLUDED
#define MPI_INIT_H_INCLUDED

/* Definitions local to src/mpi/init only */
int MPIR_Init_thread(int *, char ***, int, int *);
int MPIR_Thread_CS_Finalize(void);

extern int MPIR_async_thread_initialized;

#endif /* MPI_INIT_H_INCLUDED */
