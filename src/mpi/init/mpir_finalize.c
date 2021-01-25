/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpi_init.h"

int MPII_Finalize(MPIR_Session * session_ptr)
{
    return MPI_SUCCESS;
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
    MPII_Call_finalize_callbacks(MPIR_FINALIZE_CALLBACK_PRIO + 1, MPIR_FINALIZE_CALLBACK_MAX_PRIO);

    /* Signal the debugger that we are about to exit. */
    MPIR_Debugger_set_aborting(NULL);

    mpi_errno = MPID_Finalize();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPII_Coll_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    /* Call the low-priority (post Finalize) callbacks */
    MPII_Call_finalize_callbacks(0, MPIR_FINALIZE_CALLBACK_PRIO - 1);

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
