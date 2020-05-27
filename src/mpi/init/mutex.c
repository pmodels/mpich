/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpi_init.h"

#if defined(MPICH_IS_THREADED)

/* MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX is always initialized regardless of
 * MPICH_THREAD_GRANULARITY (if MPICH_IS_THREADED).
 */
MPID_Thread_mutex_t MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX;

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
MPID_Thread_mutex_t MPIR_THREAD_POBJ_HANDLE_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_POBJ_MSGQ_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_POBJ_COMPLETION_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_POBJ_CTX_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_POBJ_PMI_MUTEX;

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
MPID_Thread_mutex_t MPIR_THREAD_VCI_HANDLE_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_VCI_CTX_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_VCI_PMI_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_VCI_BSEND_MUTEX;

#endif /* MPICH_THREAD_GRANULARITY */

/* called the first thing in init so it can enter critical section immediately */
void MPII_thread_mutex_create(void)
{
    int err;

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
    MPID_Thread_mutex_create(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_HANDLE_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_MSGQ_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_COMPLETION_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_CTX_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_PMI_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    MPID_Thread_mutex_create(&MPIR_THREAD_VCI_HANDLE_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIR_THREAD_VCI_CTX_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIR_THREAD_VCI_PMI_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIR_THREAD_VCI_BSEND_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__LOCKFREE
    /* Updates to shared data and access to shared services is handled
     * without locks where ever possible. */
#error lock-free not yet implemented

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__SINGLE
    /* No thread support, make all operations a no-op */

#else
#error Unrecognized thread granularity
#endif

    MPL_DBG_MSG(MPIR_DBG_INIT, TYPICAL, "Created mutexes and private storage");
}

/* called only when encounter failure during during init */
void MPII_thread_mutex_destroy(void)
{
    int err;

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_HANDLE_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_MSGQ_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_COMPLETION_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_CTX_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_PMI_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    MPID_Thread_mutex_destroy(&MPIR_THREAD_VCI_HANDLE_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIR_THREAD_VCI_CTX_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIR_THREAD_VCI_PMI_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIR_THREAD_VCI_BSEND_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__LOCKFREE
    /* Updates to shared data and access to shared services is handled
     * without locks where ever possible. */
#error lock-free not yet implemented

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__SINGLE
    /* No thread support, make all operations a no-op */

#else
#error Unrecognized thread granularity
#endif
}

#else
/* not MPICH_IS_THREADED, empty stubs */
void MPII_thread_mutex_create(void)
{
}

void MPII_thread_mutex_destroy(void)
{
}

#endif /* MPICH_IS_THREADED */
