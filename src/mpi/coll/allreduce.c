/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Allreduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allreduce = PMPI_Allreduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allreduce  MPI_Allreduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allreduce as PMPI_Allreduce
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Allreduce
#define MPI_Allreduce PMPI_Allreduce

MPI_User_function *MPIR_Op_table[] = { MPIR_MAXF, MPIR_MINF, MPIR_SUM,
                                       MPIR_PROD, MPIR_LAND,
                                       MPIR_BAND, MPIR_LOR, MPIR_BOR,
                                       MPIR_LXOR, MPIR_BXOR,
                                       MPIR_MINLOC, MPIR_MAXLOC, };

MPIR_Op_check_dtype_fn *MPIR_Op_check_dtype_table[] = {
    MPIR_MAXF_check_dtype, MPIR_MINF_check_dtype,
    MPIR_SUM_check_dtype,
    MPIR_PROD_check_dtype, MPIR_LAND_check_dtype,
    MPIR_BAND_check_dtype, MPIR_LOR_check_dtype, MPIR_BOR_check_dtype,
    MPIR_LXOR_check_dtype, MPIR_BXOR_check_dtype,
    MPIR_MINLOC_check_dtype, MPIR_MAXLOC_check_dtype, }; 


/* This is the default implementation of allreduce. The algorithm is:
   
   Algorithm: MPI_Allreduce

   For the heterogeneous case, we call MPI_Reduce followed by MPI_Bcast
   in order to meet the requirement that all processes must have the
   same result. For the homogeneous case, we use the following algorithms.


   For long messages and for builtin ops and if count >= pof2 (where
   pof2 is the nearest power-of-two less than or equal to the number
   of processes), we use Rabenseifner's algorithm (see 
   http://www.hlrs.de/organization/par/services/models/mpi/myreduce.html ).
   This algorithm implements the allreduce in two steps: first a
   reduce-scatter, followed by an allgather. A recursive-halving
   algorithm (beginning with processes that are distance 1 apart) is
   used for the reduce-scatter, and a recursive doubling 
   algorithm is used for the allgather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few even-numbered processes send their data to their right neighbors
   (rank+1), and the reduce-scatter and allgather happen among the remaining
   power-of-two processes. At the end, the first few even-numbered
   processes get the result from their right neighbors.

   For the power-of-two case, the cost for the reduce-scatter is 
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   allgather lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case, 
   Cost = (2.floor(lgp)+2).alpha + (2.((p-1)/p) + 2).n.beta + n.(1+(p-1)/p).gamma

   
   For short messages, for user-defined ops, and for count < pof2 
   we use a recursive doubling algorithm (similar to the one in
   MPI_Allgather). We use this algorithm in the case of user-defined ops
   because in this case derived datatypes are allowed, and the user
   could pass basic datatypes on one process and derived on another as
   long as the type maps are the same. Breaking up derived datatypes
   to do the reduce-scatter is tricky. 

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma

   Possible improvements: 

   End Algorithm: MPI_Allreduce
*/


/* not declared static because a machine-specific function may call this one 
   in some cases */
#undef FCNAME 
#define FCNAME "MPIR_Allreduce"

int MPIR_Allreduce ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
    int is_homogeneous;
#ifdef MPID_HAS_HETERO
    int rc;
#endif
    int        comm_size, rank, type_size;
    int        mpi_errno = MPI_SUCCESS;
    int mask, dst, is_commutative, pof2, newrank, rem, newdst, i,
        send_idx, recv_idx, last_idx, send_cnt, recv_cnt, *cnts, *disps; 
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf;
    MPI_User_function *uop;
    MPID_Op *op_ptr;
    MPI_Comm comm;
    MPIU_THREADPRIV_DECL;
#ifdef HAVE_CXX_BINDING
    int is_cxx_uop = 0;
#endif
    MPIU_CHKLMEM_DECL(3);
    
    if (count == 0) return MPI_SUCCESS;
    comm = comm_ptr->handle;

    MPIU_THREADPRIV_GET;
    MPIR_Nest_incr();
    
    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    
#ifdef MPID_HAS_HETERO
    if (!is_homogeneous) {
        /* heterogeneous. To get the same result on all processes, we
           do a reduce to 0 and then broadcast. */
        mpi_errno = NMPI_Reduce ( sendbuf, recvbuf, count, datatype,
                                  op, 0, comm );
	/* FIXME: mpi_errno is error CODE, not necessarily the error
	   class MPI_ERR_OP.  In MPICH2, we can get the error class 
	   with 
	       errorclass = mpi_errno & ERROR_CLASS_MASK;
	*/
        if (mpi_errno == MPI_ERR_OP || mpi_errno == MPI_SUCCESS) {
	    /* Allow MPI_ERR_OP since we can continue from this error */
            rc = NMPI_Bcast  ( recvbuf, count, datatype, 0, comm );
            if (rc) mpi_errno = rc;
        }
    }
    else 
#endif /* MPID_HAS_HETERO */
    {
        /* homogeneous */
        
        /* set op_errno to 0. stored in perthread structure */
        MPIU_THREADPRIV_FIELD(op_errno) = 0;

        comm_size = comm_ptr->local_size;
        rank = comm_ptr->rank;
        
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
        
        /* need to allocate temporary buffer to store incoming data*/
        mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb,
                                              &true_extent);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
        MPID_Datatype_get_extent_macro(datatype, extent);

        MPID_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));
        MPIU_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPIR_MAX(extent,true_extent)), mpi_errno, "temporary buffer");
	
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
        
        /* copy local data into recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                       count, datatype);
            MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
        }

        MPID_Datatype_get_size_macro(datatype, type_size);

        /* find nearest power-of-two less than or equal to comm_size */
        pof2 = 1;
        while (pof2 <= comm_size) pof2 <<= 1;
        pof2 >>=1;

        rem = comm_size - pof2;

        /* In the non-power-of-two case, all even-numbered
           processes of rank < 2*rem send their data to
           (rank+1). These even-numbered processes no longer
           participate in the algorithm until the very end. The
           remaining processes form a nice power-of-two. */
        
        /* check if multiple threads are calling this collective function */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

        if (rank < 2*rem) {
            if (rank % 2 == 0) { /* even */
                mpi_errno = MPIC_Send(recvbuf, count, 
                                      datatype, rank+1,
                                      MPIR_ALLREDUCE_TAG, comm);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
                
                /* temporarily set the rank to -1 so that this
                   process does not pariticipate in recursive
                   doubling */
                newrank = -1; 
            }
            else { /* odd */
                mpi_errno = MPIC_Recv(tmp_buf, count, 
                                      datatype, rank-1,
                                      MPIR_ALLREDUCE_TAG, comm,
                                      MPI_STATUS_IGNORE);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
                
                /* do the reduction on received data. since the
                   ordering is right, it doesn't matter whether
                   the operation is commutative or not. */
#ifdef HAVE_CXX_BINDING
                if (is_cxx_uop) {
                    (*MPIR_Process.cxx_call_op_fn)( tmp_buf, recvbuf, 
                                                    count,
                                                    datatype,
                                                    uop ); 
                }
                else 
#endif
                    (*uop)(tmp_buf, recvbuf, &count, &datatype);
                
                /* change the rank */
                newrank = rank / 2;
            }
        }
        else  /* rank >= 2*rem */
            newrank = rank - rem;
        
        /* If op is user-defined or count is less than pof2, use
           recursive doubling algorithm. Otherwise do a reduce-scatter
           followed by allgather. (If op is user-defined,
           derived datatypes are allowed and the user could pass basic
           datatypes on one process and derived on another as long as
           the type maps are the same. Breaking up derived
           datatypes to do the reduce-scatter is tricky, therefore
           using recursive doubling in that case.) */

        if (newrank != -1) {
            if ((count*type_size <= MPIR_ALLREDUCE_SHORT_MSG) ||
                (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) ||  
                (count < pof2)) { /* use recursive doubling */
                mask = 0x1;
                while (mask < pof2) {
                    newdst = newrank ^ mask;
                    /* find real rank of dest */
                    dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

                    /* Send the most current data, which is in recvbuf. Recv
                       into tmp_buf */ 
                    mpi_errno = MPIC_Sendrecv(recvbuf, count, datatype, 
                                              dst, MPIR_ALLREDUCE_TAG, tmp_buf,
                                              count, datatype, dst,
                                              MPIR_ALLREDUCE_TAG, comm,
                                              MPI_STATUS_IGNORE);
		    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
                    
                    /* tmp_buf contains data received in this step.
                       recvbuf contains data accumulated so far */
                    
                    if (is_commutative  || (dst < rank)) {
                        /* op is commutative OR the order is already right */
#ifdef HAVE_CXX_BINDING
                        if (is_cxx_uop) {
                            (*MPIR_Process.cxx_call_op_fn)( tmp_buf, recvbuf, 
                                                            count,
                                                            datatype,
                                                            uop ); 
                        }
                        else 
#endif
                            (*uop)(tmp_buf, recvbuf, &count, &datatype);
                    }
                    else {
                        /* op is noncommutative and the order is not right */
#ifdef HAVE_CXX_BINDING
                        if (is_cxx_uop) {
                            (*MPIR_Process.cxx_call_op_fn)( recvbuf, tmp_buf, 
                                                            count,
                                                            datatype,
                                                            uop ); 
                        }
                        else 
#endif
                            (*uop)(recvbuf, tmp_buf, &count, &datatype);
                        
                        /* copy result back into recvbuf */
                        mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                                   recvbuf, count, datatype);
			MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
                    }
                    mask <<= 1;
                }
            }

            else {

                /* do a reduce-scatter followed by allgather */

                /* for the reduce-scatter, calculate the count that
                   each process receives and the displacement within
                   the buffer */

		MPIU_CHKLMEM_MALLOC(cnts, int *, pof2*sizeof(int), mpi_errno, "counts");
		MPIU_CHKLMEM_MALLOC(disps, int *, pof2*sizeof(int), mpi_errno, "displacements");

                for (i=0; i<(pof2-1); i++) 
                    cnts[i] = count/pof2;
                cnts[pof2-1] = count - (count/pof2)*(pof2-1);

                disps[0] = 0;
                for (i=1; i<pof2; i++)
                    disps[i] = disps[i-1] + cnts[i-1];

                mask = 0x1;
                send_idx = recv_idx = 0;
                last_idx = pof2;
                while (mask < pof2) {
                    newdst = newrank ^ mask;
                    /* find real rank of dest */
                    dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

                    send_cnt = recv_cnt = 0;
                    if (newrank < newdst) {
                        send_idx = recv_idx + pof2/(mask*2);
                        for (i=send_idx; i<last_idx; i++)
                            send_cnt += cnts[i];
                        for (i=recv_idx; i<send_idx; i++)
                            recv_cnt += cnts[i];
                    }
                    else {
                        recv_idx = send_idx + pof2/(mask*2);
                        for (i=send_idx; i<recv_idx; i++)
                            send_cnt += cnts[i];
                        for (i=recv_idx; i<last_idx; i++)
                            recv_cnt += cnts[i];
                    }

/*                    printf("Rank %d, send_idx %d, recv_idx %d, send_cnt %d, recv_cnt %d, last_idx %d\n", newrank, send_idx, recv_idx,
                           send_cnt, recv_cnt, last_idx);
                           */
                    /* Send data from recvbuf. Recv into tmp_buf */ 
                    mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                              disps[send_idx]*extent,
                                              send_cnt, datatype,  
                                              dst, MPIR_ALLREDUCE_TAG, 
                                              (char *) tmp_buf +
                                              disps[recv_idx]*extent,
                                              recv_cnt, datatype, dst,
                                              MPIR_ALLREDUCE_TAG, comm,
                                              MPI_STATUS_IGNORE);
		    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
                    
                    /* tmp_buf contains data received in this step.
                       recvbuf contains data accumulated so far */
                    
                    /* This algorithm is used only for predefined ops
                       and predefined ops are always commutative. */

		    (*uop)((char *) tmp_buf + disps[recv_idx]*extent,
			   (char *) recvbuf + disps[recv_idx]*extent, 
			   &recv_cnt, &datatype);
                    
                    /* update send_idx for next iteration */
                    send_idx = recv_idx;
                    mask <<= 1;

                    /* update last_idx, but not in last iteration
                       because the value is needed in the allgather
                       step below. */
                    if (mask < pof2)
                        last_idx = recv_idx + pof2/mask;
                }

                /* now do the allgather */

                mask >>= 1;
                while (mask > 0) {
                    newdst = newrank ^ mask;
                    /* find real rank of dest */
                    dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

                    send_cnt = recv_cnt = 0;
                    if (newrank < newdst) {
                        /* update last_idx except on first iteration */
                        if (mask != pof2/2)
                            last_idx = last_idx + pof2/(mask*2);

                        recv_idx = send_idx + pof2/(mask*2);
                        for (i=send_idx; i<recv_idx; i++)
                            send_cnt += cnts[i];
                        for (i=recv_idx; i<last_idx; i++)
                            recv_cnt += cnts[i];
                    }
                    else {
                        recv_idx = send_idx - pof2/(mask*2);
                        for (i=send_idx; i<last_idx; i++)
                            send_cnt += cnts[i];
                        for (i=recv_idx; i<send_idx; i++)
                            recv_cnt += cnts[i];
                    }

                    mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                              disps[send_idx]*extent,
                                              send_cnt, datatype,  
                                              dst, MPIR_ALLREDUCE_TAG, 
                                              (char *) recvbuf +
                                              disps[recv_idx]*extent,
                                              recv_cnt, datatype, dst,
                                              MPIR_ALLREDUCE_TAG, comm,
                                              MPI_STATUS_IGNORE);
		    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");

                    if (newrank > newdst) send_idx = recv_idx;

                    mask >>= 1;
                }
            }
        }

        /* In the non-power-of-two case, all odd-numbered
           processes of rank < 2*rem send the result to
           (rank-1), the ranks who didn't participate above. */
        if (rank < 2*rem) {
            if (rank % 2)  /* odd */
                mpi_errno = MPIC_Send(recvbuf, count, 
                                      datatype, rank-1,
                                      MPIR_ALLREDUCE_TAG, comm);
            else  /* even */
                mpi_errno = MPIC_Recv(recvbuf, count,
                                      datatype, rank+1,
                                      MPIR_ALLREDUCE_TAG, comm,
                                      MPI_STATUS_IGNORE); 
            MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
        }

        /* check if multiple threads are calling this collective function */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );

        if (MPIU_THREADPRIV_FIELD(op_errno)) 
	    mpi_errno = MPIU_THREADPRIV_FIELD(op_errno);
    }

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIR_Nest_decr();
    return (mpi_errno);

  fn_fail:
    goto fn_exit;
}


/* not declared static because a machine-specific function may call this one 
   in some cases */
#undef FCNAME
#define FCNAME "MPIR_Allreduce_inter"
int MPIR_Allreduce_inter ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
/* Intercommunicator Allreduce.
   We first do an intercommunicator reduce to rank 0 on left group,
   then an intercommunicator reduce to rank 0 on right group, followed
   by local intracommunicator broadcasts in each group.

   We don't do local reduces first and then intercommunicator
   broadcasts because it would require allocation of a temporary buffer. 
*/
    int rank, mpi_errno, root;
    MPID_Comm *newcomm_ptr = NULL;
    MPIU_THREADPRIV_DECL;

    MPIU_THREADPRIV_GET;

    MPIR_Nest_incr();
    
    rank = comm_ptr->rank;

    /* first do a reduce from right group to rank 0 in left group,
       then from left group to rank 0 in right group*/
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce_inter(sendbuf, recvbuf, count, datatype, op,
				      root, comm_ptr);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Reduce_inter(sendbuf, recvbuf, count, datatype, op,
				      root, comm_ptr);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
    }
    else {
        /* reduce to rank 0 of left group */
        root = 0;
        mpi_errno = MPIR_Reduce_inter(sendbuf, recvbuf, count, datatype, op,
				      root, comm_ptr);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce_inter(sendbuf, recvbuf, count, datatype, op,
				      root, comm_ptr);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
	MPIR_Setup_intercomm_localcomm( comm_ptr );

    newcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPIR_Bcast(recvbuf, count, datatype, 0, newcomm_ptr);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");

  fn_exit:
    MPIR_Nest_decr();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Allreduce
#undef FCNAME

/*@
MPI_Allreduce - Combines values from all processes and distributes the result
                back to all processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - data type of elements of send buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - starting address of receive buffer (choice) 

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_ERR_BUFFER
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_OP
.N MPI_ERR_COMM
@*/
int MPI_Allreduce ( void *sendbuf, void *recvbuf, int count, 
		    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm )
{
    static const char FCNAME[] = "MPI_Allreduce";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLREDUCE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_ALLREDUCE);

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
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    MPIR_ERRTEST_OP(op, mpi_errno);
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
            }

	    if (comm_ptr->comm_kind == MPID_INTERCOMM)
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);
            
            if (sendbuf != MPI_IN_PLACE) 
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

	MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
	if (mpi_errno != MPI_SUCCESS) goto fn_fail;

        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Allreduce != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Allreduce(sendbuf, recvbuf, count,
                                              datatype, op, comm_ptr);
    }
    else
    {
        if (comm_ptr->comm_kind == MPID_INTRACOMM) {
            /* intracommunicator */
#if USE_SMP_COLLECTIVES
	    MPID_Op *op_ptr;
	    int is_commutative; 

	    /* is the op commutative? We do SMP optimizations only if it is. */ 
	    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN)
		is_commutative = 1;
	    else {
		MPID_Op_get_ptr(op, op_ptr);
		is_commutative = (op_ptr->kind == MPID_OP_USER_NONCOMMUTE) ? 0 : 1;
	    }

            if (MPIR_Comm_is_node_aware(comm_ptr) && is_commutative) {
		/* on each node, do a reduce to the local root */ 
                if (comm_ptr->node_comm != NULL)
                {
		    /* take care of the MPI_IN_PLACE case. For reduce, 
		        MPI_IN_PLACE is specified only on the root; 
                        for allreduce it is specified on all processes. */

		    if ((sendbuf == MPI_IN_PLACE) && (comm_ptr->node_comm->rank != 0)) {
			/* IN_PLACE and not root of reduce. Data supplied to this
			allreduce is in recvbuf. Pass that as the sendbuf to reduce. */
			
			mpi_errno = MPIR_Reduce_or_coll_fn(recvbuf, NULL, count, datatype,
						op, 0, comm_ptr->node_comm);
		    }
		    else {
			mpi_errno = MPIR_Reduce_or_coll_fn(sendbuf, recvbuf, count, datatype,
						op, 0, comm_ptr->node_comm);
		    }
		    if (mpi_errno) goto fn_fail;
                }
		else { 
		    /* only one process on the node. copy sendbuf to recvbuf */
		    if (sendbuf != MPI_IN_PLACE) {
			mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, 
						   recvbuf, count, datatype);
			if (mpi_errno) goto fn_fail;
		    }
		}

                /* now do an IN_PLACE allreduce among the local roots of all nodes */
                if (comm_ptr->node_roots_comm != NULL) {
                    mpi_errno = MPIR_Allreduce(MPI_IN_PLACE, recvbuf, count, datatype,
					       op, comm_ptr->node_roots_comm);
                    if (mpi_errno) goto fn_fail;
                }

		/* now broadcast the result among local processes */
                if (comm_ptr->node_comm != NULL) {
		    MPIU_THREADPRIV_DECL;
		    MPIU_THREADPRIV_GET;
		    
		    MPIR_Nest_incr();
                    mpi_errno = MPIR_Bcast_or_coll_fn(recvbuf, count, datatype, 
						      0, comm_ptr->node_comm);
		    MPIR_Nest_decr();
		}
            }
            else {
                mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype,
                                       op, comm_ptr);
            }
#else
            mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype,
                                       op, comm_ptr); 
#endif
	}
        else {
            /* intercommunicator */
            mpi_errno = MPIR_Allreduce_inter(sendbuf, recvbuf, count,
					     datatype, op, comm_ptr);       
        }
    }

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_allreduce",
	    "**mpi_allreduce %p %p %d %D %O %C", sendbuf, recvbuf, count, datatype, op, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
