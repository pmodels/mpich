/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This file contains glue code that ROMIO needs to call/use in order to access
 * certain internals of MPICH.  This allows us stop using "mpiimpl.h" (and all
 * the headers it includes) directly inside of ROMIO. */

#include "mpiimpl.h"
#include "glue_romio.h"

int MPIR_Ext_dbg_romio_terse_enabled = 0;
int MPIR_Ext_dbg_romio_typical_enabled = 0;
int MPIR_Ext_dbg_romio_verbose_enabled = 0;

/* to be called early by ROMIO's initialization process in order to setup init-time
 * glue code that cannot be initialized statically */
int MPIR_Ext_init(void)
{
    if (MPIU_DBG_SELECTED(ROMIO,TERSE))
        MPIR_Ext_dbg_romio_terse_enabled = 1;
    if (MPIU_DBG_SELECTED(ROMIO,TYPICAL))
        MPIR_Ext_dbg_romio_typical_enabled = 1;
    if (MPIU_DBG_SELECTED(ROMIO,VERBOSE))
        MPIR_Ext_dbg_romio_verbose_enabled = 1;

    return MPI_SUCCESS;
}


int MPIR_Ext_assert_fail(const char *cond, const char *file_name, int line_num)
{
    return MPIR_Assert_fail(cond, file_name, line_num);
}

void MPIR_Ext_thread_mutex_create(void **mutex_p_p)
{
    int err;
    MPID_Thread_mutex_t *m;

    m = (MPID_Thread_mutex_t *) MPIU_Malloc(sizeof(MPID_Thread_mutex_t));
    MPID_Thread_mutex_create(m, &err);
    MPIU_Assert(err == 0);

    *mutex_p_p = (void *) m;
}

void MPIR_Ext_thread_mutex_destroy(void *mutex_p)
{
    int err;
    MPID_Thread_mutex_t *m = (MPID_Thread_mutex_t *) mutex_p;

    MPID_Thread_mutex_destroy(m, &err);
    MPIU_Assert(err == 0);

    MPIU_Free(mutex_p);
}

/* These two routines export the GLOBAL CS_ENTER/EXIT macros as functions so
 * that ROMIO can use them.  These routines only support the GLOBAL granularity
 * of MPICH threading; other accommodations must be made for finer-grained
 * threading strategies. */
void MPIR_Ext_cs_enter(void *mutex_p)
{
    MPID_THREAD_CS_ENTER(GLOBAL, *((MPID_Thread_mutex_t *) mutex_p));
}

void MPIR_Ext_cs_exit(void *mutex_p)
{
    MPID_THREAD_CS_EXIT(GLOBAL, *((MPID_Thread_mutex_t *) mutex_p));
}

/* This routine is for a thread to yield control when the thread is waiting for
 * the completion of communication inside a ROMIO routine but the progress
 * engine is blocked by another thread. */
void MPIR_Ext_cs_yield(void *mutex_p)
{
    /* TODO: check whether the progress engine is blocked */
    MPID_THREAD_CS_YIELD(GLOBAL, *((MPID_Thread_mutex_t *) mutex_p));
}

/* will consider MPI_DATATYPE_NULL to be an error */
#undef FUNCNAME
#define FUNCNAME MPIR_Ext_datatype_iscommitted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ext_datatype_iscommitted(MPI_Datatype datatype)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
        MPID_Datatype *datatype_ptr = NULL;
        MPID_Datatype_get_ptr(datatype, datatype_ptr);

        MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_fail:
    return mpi_errno;
}

