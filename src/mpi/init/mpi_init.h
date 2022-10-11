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

    - name        : MPIR_CVAR_DEBUG_SUMMARY
      category    : DEVELOPER
      alt-env     : MPIR_CVAR_MEM_CATEGORY_INFORMATION, MPIR_CVAR_CH4_OFI_CAPABILITY_SETS_DEBUG, MPIR_CVAR_CH4_UCX_CAPABILITY_DEBUG
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_MPIDEV_DETAIL
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, print internal summary of various debug information, such as memory allocation by category.
        Each layer may print their own summary information. For example, ch4-ofi may print its provider
        capability settings.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Definitions local to src/mpi/init only */
int MPII_Init_thread(int *argc, char ***argv, int user_required, int *provided,
                     MPIR_Session ** p_session_ptr);
int MPII_Finalize(MPIR_Session * session_ptr);

void MPII_thread_mutex_create(void);
void MPII_thread_mutex_destroy(void);

int MPII_init_local_proc_attrs(int *p_thread_required);
int MPII_init_tag_ub(void);
int MPII_init_builtin_infos(void);
int MPII_finalize_builtin_infos(void);

void MPII_init_windows(void);
void MPII_init_binding_cxx(void);
void MPII_pre_init_dbg_logging(int *argc, char ***argv);
void MPII_init_dbg_logging(void);

int MPII_init_async(void);
int MPII_finalize_async(void);

void MPII_Call_finalize_callbacks(int min_prio, int max_prio);
void MPII_dump_debug_summary(void);

/* MPI_Init[_thread]/MPI_Finalize only can be used in "world" model where it only
 * can be initialized and finalized once, while we can have multiple sessions.
 * Following inline functions are used to track the world model state in functions
 * MPI_Initialized, MPI_Finalized, MPI_Init, MPI_Init_thread, and MPI_Finalize
 */

/* Possible values for process world_model_state */
#define MPICH_WORLD_MODEL_UNINITIALIZED 0
#define MPICH_WORLD_MODEL_INITIALIZED   1
#define MPICH_WORLD_MODEL_FINALIZED     2

extern MPL_atomic_int_t MPIR_world_model_state;

static inline void MPII_world_set_initilized(void)
{
    MPL_atomic_store_int(&MPIR_world_model_state, MPICH_WORLD_MODEL_INITIALIZED);
}

static inline void MPII_world_set_finalized(void)
{
    MPL_atomic_store_int(&MPIR_world_model_state, MPICH_WORLD_MODEL_FINALIZED);
}

static inline bool MPII_world_is_initialized(void)
{
    /* Note: the standards says that whether MPI_FINALIZE has been called does
     * not affect the behavior of MPI_INITIALIZED. */
    return (MPL_atomic_load_int(&MPIR_world_model_state) != MPICH_WORLD_MODEL_UNINITIALIZED);
}

static inline bool MPII_world_is_finalized(void)
{
    return (MPL_atomic_load_int(&MPIR_world_model_state) == MPICH_WORLD_MODEL_FINALIZED);
}

/* defined here instead of mpir_err.h to keep the symbols within this header */
#define MPIR_ERRTEST_INITTWICE() \
    if (MPL_atomic_load_int(&MPIR_world_model_state) != MPICH_WORLD_MODEL_UNINITIALIZED) { \
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, \
                                         __func__, __LINE__, MPI_ERR_OTHER, "**inittwice", 0); \
        goto fn_fail; \
    }

/* Other inline routines used in Init/Finalize */

static inline void MPII_pre_init_memory_tracing(void)
{
#ifdef USE_MEMORY_TRACING
    MPL_trinit();
#endif
}

static inline void MPII_post_init_memory_tracing(void)
{
#ifdef USE_MEMORY_TRACING
    MPL_trconfig(MPIR_Process.rank, MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE);
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
        if (MPIR_CVAR_DEBUG_SUMMARY)
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
