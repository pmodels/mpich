/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This file contains glue code that ROMIO needs to call/use in order to access
 * certain internals of MPICH.  This allows us stop using "mpiimpl.h" (and all
 * the headers it includes) directly inside of ROMIO. */

#include "mpiimpl.h"
#include "mpir_ext.h"

#if defined (MPL_USE_DBG_LOGGING)
static MPL_dbg_class DBG_ROMIO;
#endif /* MPL_USE_DBG_LOGGING */

int MPIR_Ext_dbg_romio_terse_enabled = 0;
int MPIR_Ext_dbg_romio_typical_enabled = 0;
int MPIR_Ext_dbg_romio_verbose_enabled = 0;

/* to be called early by ROMIO's initialization process in order to setup init-time
 * glue code that cannot be initialized statically */
int MPIR_Ext_init(void)
{
    MPIR_Ext_dbg_romio_terse_enabled = 0;
    MPIR_Ext_dbg_romio_typical_enabled = 0;
    MPIR_Ext_dbg_romio_verbose_enabled = 0;

#if defined (MPL_USE_DBG_LOGGING)
    DBG_ROMIO = MPL_dbg_class_alloc("ROMIO", "romio");

    if (MPL_DBG_SELECTED(DBG_ROMIO,TERSE))
        MPIR_Ext_dbg_romio_terse_enabled = 1;
    if (MPL_DBG_SELECTED(DBG_ROMIO,TYPICAL))
        MPIR_Ext_dbg_romio_typical_enabled = 1;
    if (MPL_DBG_SELECTED(DBG_ROMIO,VERBOSE))
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
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

void MPIR_Ext_cs_exit(void)
{
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

/* This routine is for a thread to yield control when the thread is waiting for
 * the completion of communication inside a ROMIO routine but the progress
 * engine is blocked by another thread. */
void MPIR_Ext_cs_yield(void)
{
    /* TODO: check whether the progress engine is blocked */
    MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
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
        MPIR_Datatype *datatype_ptr = NULL;
        MPIR_Datatype_get_ptr(datatype, datatype_ptr);

        MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_fail:
    return mpi_errno;
}

/* ROMIO could parse hostnames but it's easier if we can let it know
 * node ids */
int MPIR_Get_node_id(MPI_Comm comm, int rank, int *id)
{
    MPIR_Comm *comm_ptr;
    MPID_Node_id_t node_id;

    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPID_Get_node_id(comm_ptr, rank, &node_id);
    *id = node_id;

    return MPI_SUCCESS;
}

int MPIR_Abort(MPI_Comm comm, int mpi_errno, int exit_code, const char *error_msg)
{
    MPIR_Comm *comm_ptr;

    MPIR_Comm_get_ptr(comm, comm_ptr);

    return MPID_Abort(comm_ptr, mpi_errno, exit_code, error_msg);
}
