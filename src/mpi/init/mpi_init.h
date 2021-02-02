/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPI_INIT_H_INCLUDED
#define MPI_INIT_H_INCLUDED

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : DEVELOPER
      description : useful for developers working on MPICH itself

cvars:
    - name        : MPIR_CVAR_MEMDUMP
      category    : DEVELOPER
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_MPIDEV_DETAIL
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, list any memory that was allocated by MPICH and that
        remains allocated when MPI_Finalize completes.

    - name        : MPIR_CVAR_MEM_CATEGORY_INFORMATION
      category    : DEVELOPER
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_MPIDEV_DETAIL
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, print a summary of memory allocation by category. The category
        definitions are found in mpl_trmem.h.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Definitions local to src/mpi/init only */
int MPIR_Init_thread(int *, char ***, int, int *);

void MPII_thread_mutex_create(void);
void MPII_thread_mutex_destroy(void);

int MPII_init_local_proc_attrs(int *p_thread_required);
int MPII_finalize_local_proc_attrs(void);
int MPII_init_tag_ub(void);

void MPII_init_windows(void);
void MPII_init_binding_cxx(void);
void MPII_init_binding_f08(void);
void MPII_pre_init_dbg_logging(int *argc, char ***argv);
void MPII_init_dbg_logging(void);

int MPII_init_async(void);
int MPII_finalize_async(void);

static inline void MPII_pre_init_memory_tracing(void)
{
#ifdef USE_MEMORY_TRACING
    MPL_trinit();
#endif
}

static inline void MPII_post_init_memory_tracing(void)
{
#ifdef USE_MEMORY_TRACING
    MPL_trconfig(MPIR_Process.comm_world->rank,
                 MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE);
#endif
}

static inline void MPII_finalize_memory_tracing(void)
{
#ifdef USE_MEMORY_TRACING
    /* FIXME: We'd like to arrange for the mem dump output to
     * go to separate files or to be sorted by rank (note that
     * the rank is at the head of the line) */
    {
        if (MPIR_CVAR_MEMDUMP) {
            /* The second argument is the min id to print; memory allocated
             * after MPI_Init is given an id of one.  This allows us to
             * ignore, if desired, memory leaks in the MPID_Init call */
            MPL_trdump((void *) 0, -1);
        }
        if (MPIR_CVAR_MEM_CATEGORY_INFORMATION)
            MPL_trcategorydump(stderr);
    }
#endif
}

static inline void MPII_debugger_hold(void)
{
    volatile int hold = 1;
    while (hold) {
#ifdef HAVE_USLEEP
        usleep(100);
#endif
    }
}

static inline void MPII_final_coverage_delay(int rank)
{
#if defined(HAVE_USLEEP) && defined(USE_COVERAGE)
    /*
     * On some systems, a 0.1 second delay appears to be too short for
     * the file system.  This code allows the use of the environment
     * variable MPICH_FINALDELAY, which is the delay in milliseconds.
     * It must be an integer value.
     */
    {
        int microseconds = 100000;
        char *delayStr = getenv("MPICH_FINALDELAY");
        if (delayStr) {
            /* Because this is a maintainer item, we won't check for
             * errors in the delayStr */
            microseconds = 1000 * atoi(delayStr);
        }
        usleep(rank * microseconds);
    }
#endif
}

#endif /* MPI_INIT_H_INCLUDED */
