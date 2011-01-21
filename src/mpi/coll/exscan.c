/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Exscan */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Exscan = PMPI_Exscan
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Exscan  MPI_Exscan
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Exscan as PMPI_Exscan
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Exscan
#define MPI_Exscan PMPI_Exscan

/* NOTE: copied from red_scat.c, if we use this one more time we need to
 * refactor it into a common location */
#ifdef HAVE_CXX_BINDING
/* NOTE: assumes 'uop' is the operator function pointer and
   that 'is_cxx_uop' is is a boolean indicating the obvious */
#define call_uop(in_, inout_, count_, datatype_)                                     \
do {                                                                                 \
    if (is_cxx_uop) {                                                                \
        (*MPIR_Process.cxx_call_op_fn)((in_), (inout_), (count_), (datatype_), uop); \
    }                                                                                \
    else {                                                                           \
        (*uop)((in_), (inout_), &(count_), &(datatype_));                            \
    }                                                                                \
} while (0)

#else
#define call_uop(in_, inout_, count_, datatype_)      \
    (*uop)((in_), (inout_), &(count_), &(datatype_))
#endif

/* This is the default implementation of exscan. The algorithm is:
   
   Algorithm: MPI_Exscan

   We use a lgp recursive doubling algorithm. The basic algorithm is
   given below. (You can replace "+" with any other scan operator.)
   The result is stored in recvbuf.

 .vb
   partial_scan = sendbuf;
   mask = 0x1;
   flag = 0;
   while (mask < size) {
      dst = rank^mask;
      if (dst < size) {
         send partial_scan to dst;
         recv from dst into tmp_buf;
         if (rank > dst) {
            partial_scan = tmp_buf + partial_scan;
            if (rank != 0) {
               if (flag == 0) {
                   recv_buf = tmp_buf;
                   flag = 1;
               }
               else 
                   recv_buf = tmp_buf + recvbuf;
            }
         }
         else {
            if (op is commutative)
               partial_scan = tmp_buf + partial_scan;
            else {
               tmp_buf = partial_scan + tmp_buf;
               partial_scan = tmp_buf;
            }
         }
      }
      mask <<= 1;
   }  
.ve

   End Algorithm: MPI_Exscan
*/


/* not declared static because a machine-specific function may call this one in some cases */
/* MPIR_Exscan performs an exscan using point-to-point messages.  This
   is intended to be used by device-specific implementations of
   exscan.  In all other cases MPIR_Exscan_impl should be used. */
#undef FUNCNAME
#define FUNCNAME MPIR_Exscan
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Exscan ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr,
    int *errflag )
{
    MPI_Status status;
    int        rank, comm_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int mask, dst, is_commutative, flag; 
    MPI_Aint true_extent, true_lb, extent;
    void *partial_scan, *tmp_buf;
    MPI_User_function *uop;
    MPID_Op *op_ptr;
    MPI_Comm comm;
    MPIU_CHKLMEM_DECL(2);
    MPIU_THREADPRIV_DECL;
#ifdef HAVE_CXX_BINDING
    int is_cxx_uop = 0;
#endif
    
    if (count == 0) return MPI_SUCCESS;

    MPIU_THREADPRIV_GET;
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    /* set op_errno to 0. stored in perthread structure */
    MPIU_THREADPRIV_FIELD(op_errno) = 0;

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        is_commutative = 1;
        /* get the function by indexing into the op table */
        uop = MPIR_Op_table[op%16 - 1];
    }
    else {
        MPID_Op_get_ptr(op, op_ptr);
        if (op_ptr->kind == MPID_OP_USER_NONCOMMUTE)
            is_commutative = 0;
        else
            is_commutative = 1;
        
#ifdef HAVE_CXX_BINDING            
            if (op_ptr->language == MPID_LANG_CXX) {
                uop = (MPI_User_function *) op_ptr->function.c_function;
		is_cxx_uop = 1;
	    }
	    else
#endif
	if ((op_ptr->language == MPID_LANG_C))
            uop = (MPI_User_function *) op_ptr->function.c_function;
        else
            uop = (MPI_User_function *) op_ptr->function.f77_function;
    }
    
    /* need to allocate temporary buffer to store partial scan*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPID_Datatype_get_extent_macro( datatype, extent );

    MPIU_CHKLMEM_MALLOC(partial_scan, void *, (count*(MPIR_MAX(true_extent,extent))), mpi_errno, "partial_scan");
    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *)((char*)partial_scan - true_lb);

    /* need to allocate temporary buffer to store incoming data*/
    MPIU_CHKLMEM_MALLOC(tmp_buf, void *, (count*(MPIR_MAX(true_extent,extent))), mpi_errno, "tmp_buf");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    mpi_errno = MPIR_Localcopy((sendbuf == MPI_IN_PLACE ? recvbuf : sendbuf), count, datatype,
                               partial_scan, count, datatype);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    flag = 0;
    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIC_Sendrecv_ft(partial_scan, count, datatype,
                                         dst, MPIR_EXSCAN_TAG, tmp_buf,
                                         count, datatype, dst,
                                         MPIR_EXSCAN_TAG, comm,
                                         &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = TRUE;
                MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            if (rank > dst) {
                call_uop(tmp_buf, partial_scan, count, datatype);

                /* On rank 0, recvbuf is not defined.  For sendbuf==MPI_IN_PLACE
                   recvbuf must not change (per MPI-2.2).
                   On rank 1, recvbuf is to be set equal to the value
                   in sendbuf on rank 0.
                   On others, recvbuf is the scan of values in the
                   sendbufs on lower ranks. */ 
                if (rank != 0) {
                    if (flag == 0) {
                        /* simply copy data recd from rank 0 into recvbuf */
                        mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                                   recvbuf, count, datatype);
                        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                        flag = 1;
                    }
                    else {
                        call_uop(tmp_buf, recvbuf, count, datatype);
                    }
                }
            }
            else {
                if (is_commutative) {
                    call_uop(tmp_buf, partial_scan, count, datatype);
		}
                else {
                    call_uop(partial_scan, tmp_buf, count, datatype);

                    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                               partial_scan,
                                               count, datatype);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                }
            }
        }
        mask <<= 1;
    }

    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );

    if (MPIU_THREADPRIV_FIELD(op_errno)) 
	mpi_errno = MPIU_THREADPRIV_FIELD(op_errno);

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag)
        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


/* MPIR_Exscan_impl should be called by any internal component that
   would otherwise call MPI_Exscan.  This differs from MPIR_Exscan in
   that this will call the coll_fns version if it exists.  This
   function replaces NMPI_Exscan. */
#undef FUNCNAME
#define FUNCNAME MPIR_Exscan_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Exscan_impl(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Exscan != NULL) {
	mpi_errno = comm_ptr->coll_fns->Exscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
	mpi_errno = MPIR_Exscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

        
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Exscan
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@

MPI_Exscan - Computes the exclusive scan (partial reductions) of data on a 
           collection of processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in input buffer (integer) 
. datatype - data type of elements of input buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - starting address of receive buffer (choice) 

Notes:
  'MPI_Exscan' is like 'MPI_Scan', except that the contribution from the
   calling process is not included in the result at the calling process
   (it is contributed to the subsequent processes, of course).

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_BUFFER_ALIAS
@*/
int MPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
               MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int errflag = FALSE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_EXSCAN);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_EXSCAN);

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
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *datatype_ptr = NULL;
            MPID_Op *op_ptr = NULL;
            int rank;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    MPIR_ERRTEST_OP(op, mpi_errno);
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
            }

            rank = comm_ptr->rank;

            MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);

            if (rank != 0) {
                MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
                MPIR_ERRTEST_USERBUFFER(recvbuf,count,datatype,mpi_errno);
            }

            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr( op_ptr, mpi_errno );
            }
            if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = 
                    ( * MPIR_Op_check_dtype_table[op%16 - 1] )(datatype); 
            }
            
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:    
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_EXSCAN);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_exscan",
	    "**mpi_exscan %p %p %d %D %O %C", sendbuf, recvbuf, count, datatype, op, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
