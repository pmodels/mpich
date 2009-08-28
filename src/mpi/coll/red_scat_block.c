/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Reduce_scatter_block */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce_scatter_block = PMPI_Reduce_scatter_block
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce_scatter_block  MPI_Reduce_scatter_block
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce_scatter_block as PMPI_Reduce_scatter_block
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Reduce_scatter_block
#define MPI_Reduce_scatter_block PMPI_Reduce_scatter_block
/* any utility functions should go here, usually prefixed with PMPI_LOCAL to
 * correctly handle weak symbols and the profiling interface */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Reduce_scatter_block
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@

MPI_Reduce_scatter_block - Combines values and scatters the results

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. recvcount - element count per block (non-negative integer)
. datatype - data type of elements of input buffer (handle)
. op - operation (handle)
- comm - communicator (handle)

Output Parameter:
. recvbuf - starting address of receive buffer (choice)

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_OP
.N MPI_ERR_BUFFER_ALIAS
@*/
int MPI_Reduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *scatter_comm_ptr = NULL;
    void *tmp_buf;
    MPI_Aint extent, true_extent, true_lb;
    MPIU_THREADPRIV_DECL;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_REDUCE_SCATTER_BLOCK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_REDUCE_SCATTER_BLOCK);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;
            MPID_Op *op_ptr = NULL;
            int size;

            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            size = comm_ptr->local_size;

            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
            if (comm_ptr->comm_kind == MPID_INTERCOMM)
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, recvcount, mpi_errno);

            MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,datatype,mpi_errno);
            MPIR_ERRTEST_USERBUFFER(sendbuf,recvcount*size,datatype,mpi_errno);

            MPIR_ERRTEST_OP(op, mpi_errno);

            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr(op_ptr, mpi_errno);
            }
            if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = (*MPIR_Op_check_dtype_table[op%16 - 1])(datatype);
            }
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* Use a naive implementation for now (reduce followed by scatter).
     *
     * FIXME We should adapt one or more of the existing MPI_Reduce_scatter
     * algorithms to work here as well. */
    MPIU_THREADPRIV_GET;
    MPIR_Nest_incr();

    MPID_Datatype_get_extent_macro(datatype, extent);
    mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb, &true_extent);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_CHKLMEM_MALLOC(tmp_buf, void *, true_extent * recvcount * comm_ptr->local_size, mpi_errno, "tmp_buf");
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    mpi_errno = NMPI_Reduce(sendbuf, tmp_buf,
                            (recvcount * comm_ptr->local_size), datatype, op,
                            0/*root*/, comm);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    scatter_comm_ptr = comm_ptr;
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPIR_Setup_intercomm_localcomm(comm_ptr);
        scatter_comm_ptr = comm_ptr->local_comm;
    }
    mpi_errno = NMPI_Scatter(tmp_buf, recvcount, datatype,
                             recvbuf, recvcount, datatype,
                             0/*root*/, scatter_comm_ptr->handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */
  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIR_Nest_decr();
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE_SCATTER_BLOCK);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                         MPI_ERR_OTHER, "**mpi_reduce_scatter_block",
                                         "**mpi_reduce_scatter_block %p %p %d %D %O %C",
                                         sendbuf, recvbuf, recvcount, datatype, op, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

