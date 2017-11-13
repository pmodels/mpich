/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


/* This implementation of MPI_Reduce_scatter_block was obtained by taking
   the implementation of MPI_Reduce_scatter from red_scat.c and replacing 
   recvcnts[i] with recvcount everywhere. */


#include "mpiimpl.h"
#include "../collutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Reduce_scatter_block */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce_scatter_block = PMPI_Reduce_scatter_block
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce_scatter_block  MPI_Reduce_scatter_block
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce_scatter_block as PMPI_Reduce_scatter_block
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
                             __attribute__((weak,alias("PMPI_Reduce_scatter_block")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Reduce_scatter_block
#define MPI_Reduce_scatter_block PMPI_Reduce_scatter_block


/* This is the default implementation of reduce_scatter. The algorithm is:

   Algorithm: MPI_Reduce_scatter

   If the operation is commutative, for short and medium-size
   messages, we use a recursive-halving
   algorithm in which the first p/2 processes send the second n/2 data
   to their counterparts in the other half and receive the first n/2
   data from them. This procedure continues recursively, halving the
   data communicated at each step, for a total of lgp steps. If the
   number of processes is not a power-of-two, we convert it to the
   nearest lower power-of-two by having the first few even-numbered
   processes send their data to the neighboring odd-numbered process
   at (rank+1). Those odd-numbered processes compute the result for
   their left neighbor as well in the recursive halving algorithm, and
   then at  the end send the result back to the processes that didn't
   participate.
   Therefore, if p is a power-of-two,
   Cost = lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma
   If p is not a power-of-two,
   Cost = (floor(lgp)+2).alpha + n.(1+(p-1+n)/p).beta + n.(1+(p-1)/p).gamma
   The above cost in the non power-of-two case is approximate because
   there is some imbalance in the amount of work each process does
   because some processes do the work of their neighbors as well.

   For commutative operations and very long messages we use
   we use a pairwise exchange algorithm similar to
   the one used in MPI_Alltoall. At step i, each process sends n/p
   amount of data to (rank+i) and receives n/p amount of data from
   (rank-i).
   Cost = (p-1).alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma


   If the operation is not commutative, we do the following:

   We use a recursive doubling algorithm, which
   takes lgp steps. At step 1, processes exchange (n-n/p) amount of
   data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
   amount of data, and so forth.

   Cost = lgp.alpha + n.(lgp-(p-1)/p).beta + n.(lgp-(p-1)/p).gamma

   Possible improvements:

   End Algorithm: MPI_Reduce_scatter
*/

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_block_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* not declared static because a machine-specific function may call this one in some cases */
int MPIR_Reduce_scatter_block_intra ( 
    const void *sendbuf, 
    void *recvbuf, 
    int recvcount, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int   comm_size;
    MPI_Aint true_extent, true_lb; 
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int type_size, total_count, nbytes;
    int is_commutative;
    MPIR_Op *op_ptr;

    comm_size = comm_ptr->local_size;

    /* set op_errno to 0. stored in perthread structure */
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        per_thread->op_errno = 0;
    }

    if (recvcount == 0) {
        goto fn_exit;
    }

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    
    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        is_commutative = 1;
    }
    else {
        MPIR_Op_get_ptr(op, op_ptr);
        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            is_commutative = 0;
        else
            is_commutative = 1;
    }


    total_count = comm_size*recvcount;
    
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    if ((is_commutative) && (nbytes < MPIR_CVAR_REDSCAT_COMMUTATIVE_LONG_MSG_SIZE)) {
        /* commutative and short. use recursive halving algorithm */
        mpi_errno = MPIR_Reduce_scatter_block_recursive_halving(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
    if (is_commutative && (nbytes >= MPIR_CVAR_REDSCAT_COMMUTATIVE_LONG_MSG_SIZE)) {

        /* commutative and long message, or noncommutative and long message.
           use (p-1) pairwise exchanges */ 
        mpi_errno = MPIR_Reduce_scatter_block_pairwise(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        
    }
    
    if (!is_commutative) {

        /* power of two check */
        if (!(comm_size & (comm_size - 1))) {
            /* noncommutative, pof2 size */
            mpi_errno = MPIR_Reduce_scatter_block_noncomm(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        else {
            /* noncommutative and non-pof2, use recursive doubling. */
            mpi_errno = MPIR_Reduce_scatter_block_recursive_doubling(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, errflag);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:

    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno)
            mpi_errno = per_thread->op_errno;
    }

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_block_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_scatter_block_inter ( 
    const void *sendbuf, 
    void *recvbuf, 
    int recvcount, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag )
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_block_generic_inter(sendbuf, recvbuf,
            recvcount, datatype, op, comm_ptr, errflag);

    return mpi_errno;
}


/* MPIR_Reduce_scatter_block performs a red_scat_block using
   point-to-point messages.  This is intended to be used by
   device-specific implementations of red_scat_block. */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_scatter_block(const void *sendbuf, void *recvbuf, 
                              int recvcount, MPI_Datatype datatype,
                              MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Reduce_scatter_block_intra(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno = MPIR_Reduce_scatter_block_inter(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Reduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Reduce_scatter_block - Combines values and scatters the results

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. recvcount - element count per block (non-negative integer)
. datatype - data type of elements of input buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Parameters:
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
int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, 
                             int recvcount, 
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_REDUCE_SCATTER_BLOCK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_REDUCE_SCATTER_BLOCK);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_Datatype *datatype_ptr = NULL;
            MPIR_Op *op_ptr = NULL;
	    
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            MPIR_ERRTEST_COUNT(recvcount,mpi_errno);

	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, recvcount, mpi_errno);
            } else if (sendbuf != MPI_IN_PLACE && recvcount != 0)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno)

            MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,datatype,mpi_errno);
            MPIR_ERRTEST_USERBUFFER(sendbuf,recvcount,datatype,mpi_errno); 

	    MPIR_ERRTEST_OP(op, mpi_errno);

            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr( op_ptr, mpi_errno );
            }
            if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = 
                    ( * MPIR_OP_HDL_TO_DTYPE_FN(op) )(datatype); 
            }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Reduce_scatter_block(sendbuf, recvbuf, recvcount,
                                          datatype, op, comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_REDUCE_SCATTER_BLOCK);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_reduce_scatter_block",
	    "**mpi_reduce_scatter_block %p %p %d %D %O %C", sendbuf, recvbuf, recvcount, datatype, op, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
