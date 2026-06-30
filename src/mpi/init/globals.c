/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>

/* Lock to protect concurrent init/finalize (include session init/finalize)
 * and creation of built-in comms in MPIR_Comm_create_from_group_impl */
MPL_initlock_t MPIR_init_lock = MPL_INITLOCK_INITIALIZER;

MPIR_Process_t MPIR_Process = {
    .mpich_state = MPL_ATOMIC_INT_T_INITIALIZER(MPICH_MPI_STATE__UNINITIALIZED),
    .memory_alloc_kinds = NULL
};

MPIR_Thread_info_t MPIR_ThreadInfo;
