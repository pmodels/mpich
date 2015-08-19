/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Scan */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Scan = PMPI_Scan
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Scan  MPI_Scan
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Scan as PMPI_Scan
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
             MPI_Comm comm)
             __attribute__((weak,alias("PMPI_Scan")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Scan
#define MPI_Scan PMPI_Scan

/* This is the default implementation of scan. The algorithm is:
   
   Algorithm: MPI_Scan

   We use a lgp recursive doubling algorithm. The basic algorithm is
   given below. (You can replace "+" with any other scan operator.)
   The result is stored in recvbuf.

 .vb
   recvbuf = sendbuf;
   partial_scan = sendbuf;
   mask = 0x1;
   while (mask < size) {
      dst = rank^mask;
      if (dst < size) {
         send partial_scan to dst;
         recv from dst into tmp_buf;
         if (rank > dst) {
            partial_scan = tmp_buf + partial_scan;
            recvbuf = tmp_buf + recvbuf;
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

   End Algorithm: MPI_Scan
*/


#undef FUNCNAME
#define FUNCNAME MPIR_Scan_generic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_Scan_generic ( 
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    MPID_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    MPI_Status status;
    int        rank, comm_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int mask, dst, is_commutative; 
    MPI_Aint true_extent, true_lb, extent;
    void *partial_scan, *tmp_buf;
    MPID_Op *op_ptr;
    MPID_THREADPRIV_DECL;
    MPIU_CHKLMEM_DECL(2);
    
    if (count == 0) return MPI_SUCCESS;

    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPID_THREADPRIV_GET;
    /* set op_errno to 0. stored in perthread structure */
    MPID_THREADPRIV_FIELD(op_errno) = 0;

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        is_commutative = 1;
    }
    else {
        MPID_Op_get_ptr(op, op_ptr);
        if (op_ptr->kind == MPID_OP_USER_NONCOMMUTE)
            is_commutative = 0;
        else
            is_commutative = 1;
    }
    
    /* need to allocate temporary buffer to store partial scan*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPID_Datatype_get_extent_macro(datatype, extent);
    MPIU_CHKLMEM_MALLOC(partial_scan, void *, count*(MPIR_MAX(extent,true_extent)), mpi_errno, "partial_scan");

    /* This eventually gets malloc()ed as a temp buffer, not added to
     * any user buffers */
    MPIU_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));

    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *)((char*)partial_scan - true_lb);
    
    /* need to allocate temporary buffer to store incoming data*/
    MPIU_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPIR_MAX(extent,true_extent)), mpi_errno, "tmp_buf");
    
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);
    
    /* Since this is an inclusive scan, copy local contribution into
       recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   recvbuf, count, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   partial_scan, count, datatype);
    else 
        mpi_errno = MPIR_Localcopy(recvbuf, count, datatype,
                                   partial_scan, count, datatype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIC_Sendrecv(partial_scan, count, datatype,
                                         dst, MPIR_SCAN_TAG, tmp_buf,
                                         count, datatype, dst,
                                         MPIR_SCAN_TAG, comm_ptr,
                                         &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            
            if (rank > dst) {
		mpi_errno = MPIR_Reduce_local_impl( 
			   tmp_buf, partial_scan, count, datatype, op);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		mpi_errno = MPIR_Reduce_local_impl( 
			   tmp_buf, recvbuf, count, datatype, op);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            else {
                if (is_commutative) {
		    mpi_errno = MPIR_Reduce_local_impl( 
			       tmp_buf, partial_scan, count, datatype, op);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		}
                else {
		    mpi_errno = MPIR_Reduce_local_impl( 
			       partial_scan, tmp_buf, count, datatype, op);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
					       partial_scan,
					       count, datatype);
		    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        mask <<= 1;
    }
    
    if (MPID_THREADPRIV_FIELD(op_errno)) {
	mpi_errno = MPID_THREADPRIV_FIELD(op_errno);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    MPIU_CHKLMEM_FREEALL();
     /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



/* not declared static because a machine-specific function may call this one in some cases */
/* MPIR_Scan performs an scan using point-to-point messages.  This is
   intended to be used by device-specific implementations of scan.  In
   all other cases MPIR_Scan_impl should be used. */
#undef FUNCNAME
#define FUNCNAME MPIR_Scan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Scan(
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    MPID_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(3);
    MPID_THREADPRIV_DECL;
    int rank = comm_ptr->rank;
    MPI_Status status;
    void *tempbuf = NULL, *localfulldata = NULL, *prefulldata = NULL;
    MPI_Aint  true_lb, true_extent, extent; 
    int noneed = 1; /* noneed=1 means no need to bcast tempbuf and 
                       reduce tempbuf & recvbuf */

    /* In order to use the SMP-aware algorithm, the "op" can be
       either commutative or non-commutative, but we require a
       communicator in which all the nodes contain processes with
       consecutive ranks. */

    if (!MPIR_Comm_is_node_consecutive(comm_ptr)) {
        /* We can't use the SMP-aware algorithm, use the generic one */
        return MPIR_Scan_generic(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    }
    
    MPID_THREADPRIV_GET;
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPID_Datatype_get_extent_macro(datatype, extent);

    MPIU_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));

    MPIU_CHKLMEM_MALLOC(tempbuf, void *, count*(MPIR_MAX(extent, true_extent)),
                        mpi_errno, "temporary buffer");
    tempbuf = (void *)((char*)tempbuf - true_lb);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        MPIU_CHKLMEM_MALLOC(prefulldata, void *, count*(MPIR_MAX(extent, true_extent)),
                            mpi_errno, "prefulldata for scan");
        prefulldata = (void *)((char*)prefulldata - true_lb);

        if (comm_ptr->node_comm != NULL) {
            MPIU_CHKLMEM_MALLOC(localfulldata, void *, count*(MPIR_MAX(extent, true_extent)),
                                mpi_errno, "localfulldata for scan");
            localfulldata = (void *)((char*)localfulldata - true_lb);
        }
    }
  
    /* perform intranode scan to get temporary result in recvbuf. if there is only 
       one process, just copy the raw data. */
    if (comm_ptr->node_comm != NULL)
    {
        mpi_errno = MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, 
                                   op, comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else if (sendbuf != MPI_IN_PLACE)
    {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   recvbuf, count, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* get result from local node's last processor which 
       contains the reduce result of the whole node. Name it as
       localfulldata. For example, localfulldata from node 1 contains
       reduced data of rank 1,2,3. */
    if (comm_ptr->node_roots_comm != NULL && comm_ptr->node_comm != NULL)
    {
        mpi_errno = MPIC_Recv(localfulldata, count, datatype,
                                 comm_ptr->node_comm->local_size - 1, MPIR_SCAN_TAG, 
                                 comm_ptr->node_comm, &status, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else if (comm_ptr->node_roots_comm == NULL && 
             comm_ptr->node_comm != NULL && 
             MPIU_Get_intranode_rank(comm_ptr, rank) == comm_ptr->node_comm->local_size - 1)
    {
        mpi_errno = MPIC_Send(recvbuf, count, datatype,
                                 0, MPIR_SCAN_TAG, comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else if (comm_ptr->node_roots_comm != NULL)
    {
        localfulldata = recvbuf;
    }

    /* do scan on localfulldata to prefulldata. for example, 
       prefulldata on rank 4 contains reduce result of ranks 
       1,2,3,4,5,6. it will be sent to rank 7 which is master 
       process of node 3. */
    if (comm_ptr->node_roots_comm != NULL)
    {
        mpi_errno = MPIR_Scan_impl(localfulldata, prefulldata, count, datatype,
                                   op, comm_ptr->node_roots_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        if (MPIU_Get_internode_rank(comm_ptr, rank) != 
            comm_ptr->node_roots_comm->local_size-1)
        {
            mpi_errno = MPIC_Send(prefulldata, count, datatype,
                                     MPIU_Get_internode_rank(comm_ptr, rank) + 1,
                                     MPIR_SCAN_TAG, comm_ptr->node_roots_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        if (MPIU_Get_internode_rank(comm_ptr, rank) != 0)
        {
            mpi_errno = MPIC_Recv(tempbuf, count, datatype,
                                     MPIU_Get_internode_rank(comm_ptr, rank) - 1,
                                     MPIR_SCAN_TAG, comm_ptr->node_roots_comm, &status, errflag);
            noneed = 0;
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

    /* now tempbuf contains all the data needed to get the correct 
       scan result. for example, to node 3, it will have reduce result 
       of rank 1,2,3,4,5,6 in tempbuf. 
       then we should broadcast this result in the local node, and
       reduce it with recvbuf to get final result if nessesary. */

    if (comm_ptr->node_comm != NULL) {
        mpi_errno = MPIR_Bcast_impl(&noneed, 1, MPI_INT, 0, comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    if (noneed == 0) {
        if (comm_ptr->node_comm != NULL) {
            mpi_errno = MPIR_Bcast_impl(tempbuf, count, datatype, 0, 
					comm_ptr->node_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

	mpi_errno = MPIR_Reduce_local_impl( tempbuf, recvbuf, 
					    count, datatype, op );
    }


  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* MPIR_Scan_impl should be called by any internal component that
   would otherwise call MPI_Scan.  This differs from MPIR_Scan in that
   this will call the coll_fns version if it exists.  This function
   replaces NMPI_Scan. */
#undef FUNCNAME
#define FUNCNAME MPIR_Scan_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Scan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Scan != NULL) {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Scan(sendbuf, recvbuf, count,
                                             datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END USEREXTENSION-- */
    } else {
        mpi_errno = MPIR_Scan(sendbuf, recvbuf, count, datatype,
                              op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
        
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Scan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Scan - Computes the scan (partial reductions) of data on a collection of
           processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in input buffer (integer) 
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
.N MPI_ERR_BUFFER_ALIAS
@*/
int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	     MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SCAN);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_SCAN);

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
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *datatype_ptr = NULL;
            MPID_Op *op_ptr = NULL;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    MPIR_ERRTEST_OP(op, mpi_errno);
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            /* in_place option allowed. no error check */
            MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);
            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(recvbuf,count,datatype,mpi_errno);

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr( op_ptr, mpi_errno );
            }
            if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = 
                    ( * MPIR_OP_HDL_TO_DTYPE_FN(op) )(datatype); 
            }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (sendbuf != MPI_IN_PLACE && count != 0)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCAN);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_scan",
	    "**mpi_scan %p %p %d %D %O %C", sendbuf, recvbuf, count, datatype, op, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
