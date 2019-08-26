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

int init_global(int *p_thread_required);
int post_init_global(int thread_provided);
int finalize_global(void);

void init_windows(void);
void init_binding_fortran(void);
void init_binding_cxx(void);
void init_binding_f08(void);
void pre_init_dbg_logging(int *argc, char ***argv);
void init_dbg_logging(void);
void init_topo(void);
void finalize_topo(void);

int init_async(int thread_provided);
int finalize_async(void);

static inline void pre_init_memory_tracing(void)
{
#ifdef USE_MEMORY_TRACING
    MPL_trinit();
#endif
}

static inline void post_init_memory_tracing(void)
{
    MPII_Timer_init(MPIR_Process.comm_world->rank, MPIR_Process.comm_world->local_size);
#ifdef USE_MEMORY_TRACING
#ifdef MPICH_IS_THREADED
    MPL_trconfig(MPIR_Process.comm_world->rank, MPIR_ThreadInfo.isThreaded);
#else
    MPL_trconfig(MPIR_Process.comm_world->rank, 0);
#endif
    /* Indicate that we are near the end of the init step; memory
     * allocated already will have an id of zero; this helps
     * separate memory leaks in the initialization code from
     * leaks in the "active" code */
#endif
}

static inline void debugger_hold(void)
{
    volatile int hold = 1;
    while (hold) {
#ifdef HAVE_USLEEP
        usleep(100);
#endif
    }
}

static inline void wait_for_debugger(void)
{
    /* FIXME: Does this need to come before the call to MPID_InitComplete?
     * For some debugger support, MPII_Wait_for_debugger may want to use
     * MPI communication routines to collect information for the debugger */
#ifdef HAVE_DEBUGGER_SUPPORT
    MPII_Wait_for_debugger();
#endif
}

#endif /* MPI_INIT_H_INCLUDED */
