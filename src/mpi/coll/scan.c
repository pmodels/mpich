/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

/* begin:nested */
/* not declared static because a machine-specific function may call this one in some cases */
int MPIR_Scan ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
    static const char FCNAME[] = "MPIR_Scan";
    MPI_Status status;
    int        rank, comm_size;
    int        mpi_errno = MPI_SUCCESS;
    int mask, dst, is_commutative; 
    MPI_Aint true_extent, true_lb, extent;
    void *partial_scan, *tmp_buf;
    MPI_User_function *uop;
    MPID_Op *op_ptr;
    MPI_Comm comm;
    MPIU_THREADPRIV_DECL;
#ifdef HAVE_CXX_BINDING
    int is_cxx_uop = 0;
#endif
    
    if (count == 0) return MPI_SUCCESS;

    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIU_THREADPRIV_GET;
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
    mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb,
                                          &true_extent);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    MPID_Datatype_get_extent_macro(datatype, extent);
    partial_scan = MPIU_Malloc(count*(MPIR_MAX(extent,true_extent)));

    /* This eventually gets malloc()ed as a temp buffer, not added to
     * any user buffers */
    MPID_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));

    /* --BEGIN ERROR HANDLING-- */
    if (!partial_scan) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *)((char*)partial_scan - true_lb);
    
    /* need to allocate temporary buffer to store incoming data*/
    tmp_buf = MPIU_Malloc(count*(MPIR_MAX(extent,true_extent)));
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_buf) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);
    
    /* Since this is an inclusive scan, copy local contribution into
       recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   recvbuf, count, datatype);
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
    }
    
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   partial_scan, count, datatype);
    else 
        mpi_errno = MPIR_Localcopy(recvbuf, count, datatype,
                                   partial_scan, count, datatype);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIC_Sendrecv(partial_scan, count, datatype,
                                      dst, MPIR_SCAN_TAG, tmp_buf,
                                      count, datatype, dst,
                                      MPIR_SCAN_TAG, comm,
                                      &status);
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
            
            if (rank > dst) {
#ifdef HAVE_CXX_BINDING
		if (is_cxx_uop) {
		    (*MPIR_Process.cxx_call_op_fn)( tmp_buf, partial_scan, 
				     count, datatype, uop );
		    (*MPIR_Process.cxx_call_op_fn)( tmp_buf, recvbuf, 
				     count, datatype, uop );
		}
		else 
#endif
                {		    
		    (*uop)(tmp_buf, partial_scan, &count, &datatype);
		    (*uop)(tmp_buf, recvbuf, &count, &datatype);
		}
            }
            else {
                if (is_commutative) {
#ifdef HAVE_CXX_BINDING
		    if (is_cxx_uop) {
			(*MPIR_Process.cxx_call_op_fn)( tmp_buf, partial_scan, 
					 count, datatype, uop );
		    }
		    else 
#endif
                    (*uop)(tmp_buf, partial_scan, &count, &datatype);
		}
                else {
#ifdef HAVE_CXX_BINDING
		    if (is_cxx_uop) {
			(*MPIR_Process.cxx_call_op_fn)( partial_scan, tmp_buf,
					 count, datatype, uop );
		    }
		    else 
#endif
                    (*uop)(partial_scan, tmp_buf, &count, &datatype);
                    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                               partial_scan,
                                               count, datatype);
		    /* --BEGIN ERROR HANDLING-- */
                    if (mpi_errno)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			return mpi_errno;
		    }
		    /* --END ERROR HANDLING-- */
                }
            }
        }
        mask <<= 1;
    }
    
    MPIU_Free((char *)partial_scan+true_lb); 
    MPIU_Free((char *)tmp_buf+true_lb); 
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    
    if (MPIU_THREADPRIV_FIELD(op_errno)) 
	mpi_errno = MPIU_THREADPRIV_FIELD(op_errno);

    return (mpi_errno);
}
/* end:nested */
#endif

/* A simple utility function to that calls the comm_ptr->coll_fns->Scan
override if it exists or else it calls MPIR_Scan with the same arguments. */
#undef FUNCNAME
#define FUNCNAME MPIR_Scan_or_coll_fn
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIR_Scan_or_coll_fn(
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Scan != NULL)
    {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->coll_fns->Scan(sendbuf, recvbuf, count,
                                             datatype, op, comm_ptr);
        /* --END USEREXTENSION-- */
    }
    else {
        mpi_errno = MPIR_Scan(sendbuf, recvbuf, count, 
                              datatype, op, comm_ptr);
    }

    return mpi_errno;
}

/* Sub function to perform shmcoll scan operation. The "op" could be either 
   commutative or non-commutative. 
   Restriction: we require a communicator, in which all the nodes contain 
   processes with consecutive ranks. */
#undef FUNCNAME
#define FUNCNAME MPIR_Scan_sub_shmcoll
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIR_SMP_Scan(
    void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    MPID_Comm *comm_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(3);

    int rank = comm_ptr->rank;
    MPI_Status status;
    void *tempbuf = NULL, *localfulldata = NULL, *prefulldata = NULL;
    MPI_Aint  true_lb, true_extent, extent; 
    MPI_User_function *uop;
    MPID_Op *op_ptr;
    int noneed = 1; /* noneed=1 means no need to bcast tempbuf and 
                       reduce tempbuf & recvbuf */

    mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb, &true_extent);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno); 

    MPID_Datatype_get_extent_macro(datatype, extent);

    MPID_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));

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
        mpi_errno = MPIR_Scan_or_coll_fn(sendbuf, recvbuf, count, datatype, 
                                         op, comm_ptr->node_comm);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else if (sendbuf != MPI_IN_PLACE)
    {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   recvbuf, count, datatype);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* get result from local node's last processor which 
       contains the reduce result of the whole node. Name it as
       localfulldata. For example, localfulldata from node 1 contains
       reduced data of rank 1,2,3. */
    if (comm_ptr->node_roots_comm != NULL && comm_ptr->node_comm != NULL)
    {
        mpi_errno = MPIC_Recv(localfulldata, count, datatype, 
                              comm_ptr->node_comm->local_size - 1, MPIR_SCAN_TAG, 
                              comm_ptr->node_comm->handle, &status);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else if (comm_ptr->node_roots_comm == NULL && 
             comm_ptr->node_comm != NULL && 
             MPIU_Get_intranode_rank(comm_ptr, rank) == comm_ptr->node_comm->local_size - 1)
    {
        mpi_errno = MPIC_Send(recvbuf, count, datatype,
                              0, MPIR_SCAN_TAG, comm_ptr->node_comm->handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
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
        mpi_errno = MPIR_Scan_or_coll_fn(localfulldata, prefulldata, count, datatype,
                                         op, comm_ptr->node_roots_comm);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (MPIU_Get_internode_rank(comm_ptr, rank) != 
            comm_ptr->node_roots_comm->local_size-1)
        {
            mpi_errno = MPIC_Send(prefulldata, count, datatype,
                                  MPIU_Get_internode_rank(comm_ptr, rank) + 1,
                                  MPIR_SCAN_TAG, comm_ptr->node_roots_comm->handle);
            if(mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        if (MPIU_Get_internode_rank(comm_ptr, rank) != 0)
        {
            mpi_errno = MPIC_Recv(tempbuf, count, datatype,
                                  MPIU_Get_internode_rank(comm_ptr, rank) - 1, 
                                  MPIR_SCAN_TAG, comm_ptr->node_roots_comm->handle, 
                                  &status);
            noneed = 0;
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }

    /* now tempbuf contains all the data needed to get the correct 
       scan result. for example, to node 3, it will have reduce result 
       of rank 1,2,3,4,5,6 in tempbuf. 
       then we should broadcast this result in the local node, and
       reduce it with recvbuf to get final result if nessesary. */

    if (comm_ptr->node_comm != NULL) {
        mpi_errno = MPIR_Bcast_or_coll_fn(&noneed, 1, MPI_INT, 0, comm_ptr->node_comm);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    if (noneed == 0) {
#ifdef HAVE_CXX_BINDING
        int is_cxx_uop = 0;
#endif
        if (comm_ptr->node_comm != NULL) {
            mpi_errno = MPIR_Bcast_or_coll_fn(tempbuf, count, datatype, 0, comm_ptr->node_comm);
            if(mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

        /* do reduce on tempbuf and recvbuf, finish scan. */
        if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
            /* get the function by indexing into the op table */
            uop = MPIR_Op_table[op%16 - 1];
        }
        else {
            MPID_Op_get_ptr(op, op_ptr);

#ifdef HAVE_CXX_BINDING
            if (op_ptr->language == MPID_LANG_CXX) {
                 uop = (MPI_User_function *) op_ptr->function.c_function;
                 is_cxx_uop = 1;
            }
            else
#endif
            {
                if ((op_ptr->language == MPID_LANG_C))
                    uop = (MPI_User_function *) op_ptr->function.c_function;
                else
                    uop = (MPI_User_function *) op_ptr->function.f77_function;
            }
        }

#ifdef HAVE_CXX_BINDING
        if (is_cxx_uop) {
            (*MPIR_Process.cxx_call_op_fn)( tempbuf, recvbuf, count, 
                                            datatype, uop );
        }
        else
#endif
            (*uop)(tempbuf, recvbuf, &count, &datatype);
    }

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPI_Scan
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@

MPI_Scan - Computes the scan (partial reductions) of data on a collection of
           processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in input buffer (integer) 
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
.N MPI_ERR_BUFFER_ALIAS
@*/
int MPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
	     MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SCAN);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_SCAN);

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

            /* in_place option allowed. no error check */
            MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(recvbuf,count,datatype,mpi_errno);

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

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Scan != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Scan(sendbuf, recvbuf, count,
                                             datatype, op, comm_ptr);
    }
    else
    {
	MPIU_THREADPRIV_GET;
	MPIR_Nest_incr();
#if defined(USE_SMP_COLLECTIVES)

        /* The current algorithm assume the ranks of processes in the 
           same node are consecutive. for example, node 1 contains rank
           1, 2, 3; while node 2 has 4, 5, 6 and node 3 with 7, 8, 9 */
        if (MPIR_Comm_is_node_consecutive(comm_ptr)) {
            mpi_errno = MPIR_SMP_Scan(sendbuf, recvbuf, count,
                                      datatype, op, comm_ptr);
        }
        else {
            mpi_errno = MPIR_Scan(sendbuf, recvbuf, count, datatype, 
                                  op, comm_ptr);
        }
#else
        mpi_errno = MPIR_Scan(sendbuf, recvbuf, count, datatype,
                              op, comm_ptr); 
#endif
	MPIR_Nest_decr();
    }
    
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCAN);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
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
