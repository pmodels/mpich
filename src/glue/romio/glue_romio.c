/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This file contains glue code that ROMIO needs to call/use in order to access
 * certain internals of MPICH.  This allows us stop using "mpiimpl.h" (and all
 * the headers it includes) directly inside of ROMIO. */

#include "mpiimpl.h"

#ifndef BUILD_MPI_ABI
#include "mpir_ext.h"
#else
#include "mpi_abi_util.h"
int MPIR_Ext_datatype_iscommitted(ABI_Datatype datatype);
int MPIR_Ext_datatype_iscontig(ABI_Datatype datatype, int *flag);
int MPIR_Get_node_id(ABI_Comm comm, int rank, int *id);
int MPIR_Abort(ABI_Comm comm, int mpi_errno, int exit_code, const char *error_msg);
#endif

#if defined (MPL_USE_DBG_LOGGING)
static MPL_dbg_class DBG_ROMIO;
#endif /* MPL_USE_DBG_LOGGING */

int MPIR_Ext_dbg_romio_terse_enabled = 0;
int MPIR_Ext_dbg_romio_typical_enabled = 0;
int MPIR_Ext_dbg_romio_verbose_enabled = 0;

/* mutex states */
#define ROMIO_MUTEX_UNINIT 0
#define ROMIO_MUTEX_INITIALIZING 1
#define ROMIO_MUTEX_READY 2

/* NOTE: we'll lazily initialize this mutex */
#if defined(MPICH_IS_THREADED)
static MPL_thread_mutex_t romio_mutex;
static MPL_atomic_int_t romio_mutex_initialized = MPL_ATOMIC_INT_T_INITIALIZER(ROMIO_MUTEX_UNINIT);
#endif

void MPIR_Ext_mutex_init(void)
{
#if defined(MPICH_IS_THREADED)
    while (MPL_atomic_load_int(&romio_mutex_initialized) != ROMIO_MUTEX_READY) {
        /* only one thread will change the state from UNINIT->INITIALIZING,
         * others will wait for READY */
        int oldval = MPL_atomic_cas_int(&romio_mutex_initialized, ROMIO_MUTEX_UNINIT,
                                        ROMIO_MUTEX_INITIALIZING);
        if (oldval == ROMIO_MUTEX_UNINIT) {
            int err;
            MPL_thread_mutex_create(&romio_mutex, &err);
            MPIR_Assert(err == 0);

            MPL_atomic_store_int(&romio_mutex_initialized, ROMIO_MUTEX_READY);
        }
    }
#endif
}

void MPIR_Ext_mutex_finalize(void)
{
#if defined(MPICH_IS_THREADED)
    if (MPL_atomic_load_int(&romio_mutex_initialized) == ROMIO_MUTEX_READY) {
        int err;
        MPL_thread_mutex_destroy(&romio_mutex, &err);
        MPIR_Assert(err == 0);

        MPL_atomic_store_int(&romio_mutex_initialized, ROMIO_MUTEX_UNINIT);
    }
#endif
}

/* to be called early by ROMIO's initialization process in order to setup init-time
 * glue code that cannot be initialized statically */
int MPIR_Ext_init(void)
{
    MPIR_Ext_dbg_romio_terse_enabled = 0;
    MPIR_Ext_dbg_romio_typical_enabled = 0;
    MPIR_Ext_dbg_romio_verbose_enabled = 0;

#if defined (MPL_USE_DBG_LOGGING)
    DBG_ROMIO = MPL_dbg_class_alloc("ROMIO", "romio");

    if (MPL_DBG_SELECTED(DBG_ROMIO, TERSE))
        MPIR_Ext_dbg_romio_terse_enabled = 1;
    if (MPL_DBG_SELECTED(DBG_ROMIO, TYPICAL))
        MPIR_Ext_dbg_romio_typical_enabled = 1;
    if (MPL_DBG_SELECTED(DBG_ROMIO, VERBOSE))
        MPIR_Ext_dbg_romio_verbose_enabled = 1;
#endif /* MPL_USE_DBG_LOGGING */

    return MPI_SUCCESS;
}


int MPIR_Ext_assert_fail(const char *cond, const char *file_name, int line_num)
{
    return MPIR_Assert_fail(cond, file_name, line_num);
}

/* These two routines export the GLOBAL CS_ENTER/EXIT macros as functions so
 * that ROMIO can use them.  These routines only support the GLOBAL granularity
 * of MPICH threading; other accommodations must be made for finer-grained
 * threading strategies. */
void MPIR_Ext_cs_enter(void)
{
#if defined(MPICH_IS_THREADED)
    if (MPIR_ThreadInfo.isThreaded) {
        /* lazily initialize the mutex */
        MPIR_Ext_mutex_init();

        int err;
        MPL_thread_mutex_lock(&romio_mutex, &err, MPL_THREAD_PRIO_HIGH);
        MPIR_Assert(err == 0);
    }
#endif
}

void MPIR_Ext_cs_exit(void)
{
#if defined(MPICH_IS_THREADED)
    if (MPIR_ThreadInfo.isThreaded) {
        int err;
        MPL_thread_mutex_unlock(&romio_mutex, &err);
        MPIR_Assert(err == 0);
    }
#endif
}

/* This routine is for a thread to yield control when the thread is waiting for
 * the completion of communication inside a ROMIO routine but the progress
 * engine is blocked by another thread. */
void MPIR_Ext_cs_yield(void)
{
    /* TODO: check whether the progress engine is blocked */
    MPIR_Ext_cs_exit();
    MPL_thread_yield();
    MPIR_Ext_cs_enter();
}

void *MPIR_Ext_gpu_host_alloc(const void *buf, MPI_Aint count, MPI_Datatype datatype)
{
    return MPIR_gpu_host_alloc(buf, count, datatype);
}

void MPIR_Ext_gpu_host_free(void *host_buf, MPI_Aint count, MPI_Datatype datatype)
{
    MPIR_gpu_host_free(host_buf, count, datatype);
}

void *MPIR_Ext_gpu_host_swap(const void *buf, MPI_Aint count, MPI_Datatype datatype)
{
    return MPIR_gpu_host_swap(buf, count, datatype);
}

void MPIR_Ext_gpu_swap_back(void *host_buf, void *gpu_buf, MPI_Aint count, MPI_Datatype datatype)
{
    MPIR_gpu_swap_back(host_buf, gpu_buf, count, datatype);
}

/* will consider MPI_DATATYPE_NULL to be an error */
static int ext_datatype_iscommitted(MPI_Datatype datatype)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

    if (!HANDLE_IS_BUILTIN(datatype)) {
        MPIR_Datatype *datatype_ptr = NULL;
        MPIR_Datatype_get_ptr(datatype, datatype_ptr);

        MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_fail:
    return mpi_errno;
}

/* This will eventually be removed once ROMIO knows more about MPICH */
static int ext_datatype_iscontig(MPI_Datatype datatype, int *flag)
{
    if (HANDLE_IS_BUILTIN(datatype))
        *flag = 1;
    else {
        MPIR_Datatype *dt_ptr;
        MPIR_Datatype_get_ptr(datatype, dt_ptr);
        if (!dt_ptr->is_committed) {
            MPIR_Type_commit_impl(&datatype);
        }
        MPIR_Datatype_is_contig(datatype, flag);
    }
    return MPI_SUCCESS;
}

/* ROMIO could parse hostnames but it's easier if we can let it know
 * node ids */
static int ext_get_node_id(MPI_Comm comm, int rank, int *id)
{
    MPIR_Comm *comm_ptr;

    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPID_Get_node_id(comm_ptr, rank, id);

    return MPI_SUCCESS;
}

static int ext_abort(MPI_Comm comm, int mpi_errno, int exit_code, const char *error_msg)
{
    MPIR_Comm *comm_ptr;

    MPIR_Comm_get_ptr(comm, comm_ptr);

    return MPID_Abort(comm_ptr, mpi_errno, exit_code, error_msg);
}

#ifndef BUILD_MPI_ABI
int MPIR_Ext_datatype_iscommitted(MPI_Datatype datatype)
{
    return ext_datatype_iscommitted(datatype);
}

int MPIR_Ext_datatype_iscontig(MPI_Datatype datatype, int *flag)
{
    return ext_datatype_iscontig(datatype, flag);
}

int MPIR_Get_node_id(MPI_Comm comm, int rank, int *id)
{
    return ext_get_node_id(comm, rank, id);
}

int MPIR_Abort(MPI_Comm comm, int mpi_errno, int exit_code, const char *error_msg)
{
    return ext_abort(comm, mpi_errno, exit_code, error_msg);
}

#else
int MPIR_Ext_datatype_iscommitted(ABI_Datatype datatype)
{
    return ext_datatype_iscommitted(ABI_Datatype_to_mpi(datatype));
}

int MPIR_Ext_datatype_iscontig(ABI_Datatype datatype, int *flag)
{
    return ext_datatype_iscontig(ABI_Datatype_to_mpi(datatype), flag);
}

int MPIR_Get_node_id(ABI_Comm comm, int rank, int *id)
{
    return ext_get_node_id(ABI_Comm_to_mpi(comm), rank, id);
}

int MPIR_Abort(ABI_Comm comm, int mpi_errno, int exit_code, const char *error_msg)
{
    return ext_abort(ABI_Comm_to_mpi(comm), mpi_errno, exit_code, error_msg);
}
#endif
