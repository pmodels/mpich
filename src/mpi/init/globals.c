/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>

MPIR_Process_t MPIR_Process = { OPA_INT_T_INITIALIZER(MPICH_MPI_STATE__PRE_INIT) };

MPIR_Thread_info_t MPIR_ThreadInfo;

#if defined(MPICH_IS_THREADED) && defined(MPL_TLS)
MPL_TLS MPIR_Per_thread_t MPIR_Per_thread;
#else
MPIR_Per_thread_t MPIR_Per_thread;
#endif

MPID_Thread_tls_t MPIR_Per_thread_key;

#ifdef MPICH_THREAD_USE_MDTA
/* This counts how many threads allowed to stay in the progress engine. */
OPA_int_t num_server_thread;

/* Other threads will wait in a sync object, and are recorded here. */
MPIR_Thread_sync_list_t sync_wait_list;
#endif

/* These are initialized as null (avoids making these into common symbols).
   If the Fortran binding is supported, these can be initialized to
   their Fortran values (MPI only requires that they be valid between
   MPI_Init and MPI_Finalize) */
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUS_IGNORE MPL_USED = 0;
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUSES_IGNORE MPL_USED = 0;

/* ** HACK **
 * Hack to workaround an Intel compiler bug on macOS. Touching
 * MPIR_Per_thread in this file forces the compiler to allocate it as TLS.
 * See https://github.com/pmodels/mpich/issues/3437.
 */
void _dummy_touch_tls(void);
void _dummy_touch_tls(void)
{
    MPIR_Per_thread_t *per_thread = NULL;
    int err;

    MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                 MPIR_Per_thread, per_thread, &err);
    MPIR_Assert(err == 0);
    memset(per_thread, 0, sizeof(MPIR_Per_thread_t));
}
