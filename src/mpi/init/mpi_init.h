/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPI_INIT_H_INCLUDED
#define MPI_INIT_H_INCLUDED

/* Definitions local to src/mpi/init only */
int MPIR_Init_thread(int *, char ***, int, int *);

void init_thread_and_enter_cs(int thread_required);
void init_thread_and_exit_cs(int thread_provided);
void init_thread_failed_exit_cs(void);
void finalize_thread_cs(void);
void MPIR_Thread_CS_Init(void);
void MPIR_Thread_CS_Finalize(void);

void init_windows(void);
void init_binding_fortran(void);
void init_binding_cxx(void);
void init_binding_f08(void);

extern int MPIR_async_thread_initialized;

#endif /* MPI_INIT_H_INCLUDED */
