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

/* These are initialized as null (avoids making these into common symbols).
   If the Fortran binding is supported, these can be initialized to
   their Fortran values (MPI only requires that they be valid between
   MPI_Init and MPI_Finalize) */
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUS_IGNORE MPL_USED = 0;
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUSES_IGNORE MPL_USED = 0;

MPI_F08_status MPIR_F08_MPI_STATUS_IGNORE_OBJ MPICH_API_PUBLIC;
MPI_F08_status MPIR_F08_MPI_STATUSES_IGNORE_OBJ[1] MPICH_API_PUBLIC;
MPIU_DLL_SPEC MPI_F08_status *MPI_F08_STATUS_IGNORE MPL_USED = &MPIR_F08_MPI_STATUS_IGNORE_OBJ;
MPIU_DLL_SPEC MPI_F08_status *MPI_F08_STATUSES_IGNORE MPL_USED =
    &MPIR_F08_MPI_STATUSES_IGNORE_OBJ[0];
