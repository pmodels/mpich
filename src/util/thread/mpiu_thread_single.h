/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_THREAD_SINGLE_H_INCLUDED)
#define MPIU_THREAD_SINGLE_H_INCLUDED

/* If single threaded, make this point at a pre-allocated segment.
   This structure is allocated in src/mpi/init/initthread.c */
extern MPIUI_Per_thread_t MPIUI_Thread;

#define MPIU_THREADPRIV_INITKEY
#define MPIU_THREADPRIV_INIT
#define MPIU_THREADPRIV_DECL
#define MPIU_THREADPRIV_GET
#define MPIU_THREADPRIV_FIELD(a_) (MPIUI_Thread.a_)

#endif /* !defined(MPIU_THREAD_SINGLE_H_INCLUDED) */
