/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>

MPL_atomic_int_t MPIR_init_lock = MPL_ATOMIC_INT_T_INITIALIZER(0);

MPIR_Process_t MPIR_Process = { MPL_ATOMIC_INT_T_INITIALIZER(MPICH_MPI_STATE__PRE_INIT) };

MPIR_Thread_info_t MPIR_ThreadInfo;

/* These are initialized as null (avoids making these into common symbols).
   If the Fortran binding is supported, these can be initialized to
   their Fortran values (MPI only requires that they be valid between
   MPI_Init and MPI_Finalize) */
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUS_IGNORE MPL_USED = 0;
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUSES_IGNORE MPL_USED = 0;
