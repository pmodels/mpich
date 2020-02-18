/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define MPIR_GLOBAL     /* instantiate MPIR layer globals here */
#include <mpiimpl.h>
#undef MPIR_GLOBAL

MPIR_Process_t MPIR_Process = { MPL_ATOMIC_INT_T_INITIALIZER(MPICH_MPI_STATE__PRE_INIT) };

MPIR_Thread_info_t MPIR_ThreadInfo;

#if defined(MPICH_IS_THREADED) && defined(MPL_TLS)
MPL_TLS MPIR_Thread_tls_obj_t MPIR_Thread_tls_obj;
#else
MPIR_Thread_tls_obj_t MPIR_Thread_tls_obj;
#endif

MPID_Thread_tls_key_t MPII_Thread_tls_key;

/* These are initialized as null (avoids making these into common symbols).
   If the Fortran binding is supported, these can be initialized to
   their Fortran values (MPI only requires that they be valid between
   MPI_Init and MPI_Finalize) */
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUS_IGNORE MPL_USED = 0;
MPIU_DLL_SPEC MPI_Fint *MPI_F_STATUSES_IGNORE MPL_USED = 0;

/* ** HACK **
 * Hack to workaround an Intel compiler bug on macOS. Touching
 * MPIR_Thread_tls_obj in this file forces the compiler to allocate it as TLS.
 * See https://github.com/pmodels/mpich/issues/3437.
 */
void _dummy_touch_tls(void);
void _dummy_touch_tls(void)
{
    MPIR_Thread_tls_obj_t *per_thread = NULL;
    int err;

    MPID_TLS_KEY_RETRIEVE(MPII_Thread_tls_key, MPIR_Thread_tls_obj, per_thread, &err);
    MPIR_Assert(err == 0);
    memset(per_thread, 0, sizeof(MPIR_Thread_tls_obj_t));
}
