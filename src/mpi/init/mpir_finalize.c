/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpi_init.h"

/* The following routines provide a callback facility for modules that need
   some code called on exit.  This method allows us to avoid forcing
   MPI_Finalize to know the routine names a priori.  Any module that wants to
   have a callback calls MPIR_Add_finalize(routine, extra, priority).

 */
typedef struct Finalize_func_t {
    int (*f) (void *);          /* The function to call */
    void *extra_data;           /* Data for the function */
    int priority;               /* priority is used to control the order
                                 * in which the callbacks are invoked */
} Finalize_func_t;
/* When full debugging is enabled, each MPI handle type has a finalize handler
   installed to detect unfreed handles.  */
#define MAX_FINALIZE_FUNC 64
static Finalize_func_t fstack[MAX_FINALIZE_FUNC];
static int fstack_sp = 0;
static int fstack_max_priority = 0;

void MPIR_Add_finalize(int (*f) (void *), void *extra_data, int priority)
{
    /* --BEGIN ERROR HANDLING-- */
    if (fstack_sp >= MAX_FINALIZE_FUNC) {
        /* This is a little tricky.  We may want to check the state of
         * MPIR_Process.mpich_state to decide how to signal the error */
        (void) MPL_internal_error_printf("overflow in finalize stack! "
                                         "Is MAX_FINALIZE_FUNC too small?\n");
        if (MPL_atomic_load_int(&MPIR_Process.mpich_state) != MPICH_MPI_STATE__PRE_INIT &&
            MPL_atomic_load_int(&MPIR_Process.mpich_state) != MPICH_MPI_STATE__POST_FINALIZED) {
            MPID_Abort(NULL, MPI_SUCCESS, 13, NULL);
        } else {
            exit(1);
        }
    }
    /* --END ERROR HANDLING-- */
    fstack[fstack_sp].f = f;
    fstack[fstack_sp].priority = priority;
    fstack[fstack_sp++].extra_data = extra_data;

    if (priority > fstack_max_priority)
        fstack_max_priority = priority;
}

/* Invoke the registered callbacks */
static void MPIR_Call_finalize_callbacks(int min_prio, int max_prio)
{
    int i, j;

    if (max_prio > fstack_max_priority)
        max_prio = fstack_max_priority;
    for (j = max_prio; j >= min_prio; j--) {
        for (i = fstack_sp - 1; i >= 0; i--) {
            if (fstack[i].f && fstack[i].priority == j) {
                fstack[i].f(fstack[i].extra_data);
                fstack[i].f = 0;
            }
        }
    }
}

int MPIR_Finalize_impl(void)
{
    int mpi_errno = MPI_SUCCESS;
    int rank = MPIR_Process.comm_world->rank;

    mpi_errno = MPII_finalize_async();
    MPIR_ERR_CHECK(mpi_errno);

    /* Setting isThreaded to 0 to trick any operations used within
     * MPI_Finalize to think that we are running in a single threaded
     * environment. */
#ifdef MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = 0;
#endif

    mpi_errno = MPII_finalize_local_proc_attrs();
    MPIR_ERR_CHECK(mpi_errno);

    MPII_Timer_finalize();

    /* Call the high-priority callbacks */
    MPIR_Call_finalize_callbacks(MPIR_FINALIZE_CALLBACK_PRIO + 1, MPIR_FINALIZE_CALLBACK_MAX_PRIO);

    /* Signal the debugger that we are about to exit. */
    MPIR_Debugger_set_aborting(NULL);

    mpi_errno = MPID_Finalize();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPII_Coll_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    /* Call the low-priority (post Finalize) callbacks */
    MPIR_Call_finalize_callbacks(0, MPIR_FINALIZE_CALLBACK_PRIO - 1);

    MPII_hwtopo_finalize();
    MPII_nettopo_finalize();

    /* Users did not call MPI_T_init_thread(), so we free memories allocated to
     * MPIR_T during MPI_Init here. Otherwise, free them in MPI_T_finalize() */
    if (!MPIR_T_is_initialized())
        MPIR_T_env_finalize();

    /* If performing coverage analysis, make each process sleep for
     * rank * 100 ms, to give time for the coverage tool to write out
     * any files.  It would be better if the coverage tool and runtime
     * was more careful about file updates, though the lack of OS support
     * for atomic file updates makes this harder. */
    MPII_final_coverage_delay(rank);

    /* All memory should be freed at this point */
    MPII_finalize_memory_tracing();

    MPII_thread_mutex_destroy();
    MPIR_Typerep_finalize();
    MPL_atomic_store_int(&MPIR_Process.mpich_state, MPICH_MPI_STATE__POST_FINALIZED);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
