/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Definitions local to src/mpi/init only */
int MPIR_Init_thread(int *, char ***, int, int *);
int MPIR_Init_async_thread(void);
int MPIR_Finalize_async_thread(void);

#if defined USE_ASYNC_PROGRESS
extern int MPIR_async_thread_initialized;
#endif /* USE_ASYNC_PROGRESS */
