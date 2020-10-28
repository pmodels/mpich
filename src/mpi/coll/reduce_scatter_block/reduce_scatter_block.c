/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */


/* This implementation of MPI_Reduce_scatter_block was obtained by taking
   the implementation of MPI_Reduce_scatter from reduce_scatter.c and replacing
   recvcounts[i] with recvcount everywhere. */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select reduce_scatter_block algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        noncommutative     - Force noncommutative algorithm
        recursive_doubling - Force recursive doubling algorithm
        pairwise           - Force pairwise algorithm
        recursive_halving  - Force recursive halving algorithm
        nb                 - Force nonblocking algorithm

    - name        : MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select reduce_scatter_block algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb                          - Force nonblocking algorithm
        remote_reduce_local_scatter - Force remote-reduce-local-scatter algorithm

    - name        : MPIR_CVAR_REDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Reduce_scatter_block will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

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
    __attribute__ ((weak, alias("PMPI_Reduce_scatter_block")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Reduce_scatter_block
#define MPI_Reduce_scatter_block PMPI_Reduce_scatter_block


/* This is the machine-independent implementation of reduce_scatter. The algorithm is:

   Algorithm: MPI_Reduce_scatter

   Possible improvements:

   End Algorithm: MPI_Reduce_scatter
*/


int MPIR_Reduce_scatter_block_allcomm_auto(const void *sendbuf, void *recvbuf, int recvcount,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK,
        .comm_ptr = comm_ptr,

        .u.reduce_scatter_block.sendbuf = sendbuf,
        .u.reduce_scatter_block.recvbuf = recvbuf,
        .u.reduce_scatter_block.recvcount = recvcount,
        .u.reduce_scatter_block.datatype = datatype,
        .u.reduce_scatter_block.op = op,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_noncommutative:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_noncommutative(sendbuf, recvbuf, recvcount,
                                                               datatype, op, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_pairwise:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_pairwise(sendbuf, recvbuf, recvcount, datatype, op,
                                                         comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_recursive_doubling:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_doubling(sendbuf, recvbuf, recvcount,
                                                                   datatype, op, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_recursive_halving:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_halving(sendbuf, recvbuf, recvcount,
                                                                  datatype, op, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter:
            mpi_errno =
                MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter(sendbuf, recvbuf,
                                                                            recvcount, datatype, op,
                                                                            comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_allcomm_nb:
            mpi_errno =
                MPIR_Reduce_scatter_block_allcomm_nb(sendbuf, recvbuf, recvcount, datatype, op,
                                                     comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Reduce_scatter_block_impl(const void *sendbuf, void *recvbuf,
                                   int recvcount, MPI_Datatype datatype,
                                   MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM) {
                /* intracommunicator */
            case MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM_noncommutative:
                mpi_errno = MPIR_Reduce_scatter_block_intra_noncommutative(sendbuf, recvbuf,
                                                                           recvcount, datatype, op,
                                                                           comm_ptr, errflag);
                break;
            case MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM_pairwise:
                mpi_errno = MPIR_Reduce_scatter_block_intra_pairwise(sendbuf, recvbuf,
                                                                     recvcount, datatype, op,
                                                                     comm_ptr, errflag);
                break;
            case MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM_recursive_halving:
                mpi_errno = MPIR_Reduce_scatter_block_intra_recursive_halving(sendbuf, recvbuf,
                                                                              recvcount, datatype,
                                                                              op, comm_ptr,
                                                                              errflag);
                break;
            case MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM_recursive_doubling:
                mpi_errno = MPIR_Reduce_scatter_block_intra_recursive_doubling(sendbuf, recvbuf,
                                                                               recvcount, datatype,
                                                                               op, comm_ptr,
                                                                               errflag);
                break;
            case MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Reduce_scatter_block_allcomm_nb(sendbuf, recvbuf,
                                                                 recvcount, datatype, op, comm_ptr,
                                                                 errflag);
                break;
            case MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Reduce_scatter_block_allcomm_auto(sendbuf, recvbuf,
                                                                   recvcount, datatype, op,
                                                                   comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTER_ALGORITHM) {
            case MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTER_ALGORITHM_remote_reduce_local_scatter:
                mpi_errno =
                    MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter(sendbuf, recvbuf,
                                                                                recvcount, datatype,
                                                                                op, comm_ptr,
                                                                                errflag);
                break;
            case MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Reduce_scatter_block_allcomm_nb(sendbuf, recvbuf,
                                                                 recvcount, datatype, op, comm_ptr,
                                                                 errflag);
                break;
            case MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Reduce_scatter_block_allcomm_auto(sendbuf, recvbuf, recvcount,
                                                                   datatype, op, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                              int recvcount, MPI_Datatype datatype,
                              MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    void *in_recvbuf = recvbuf;
    void *host_sendbuf;
    void *host_recvbuf;

    MPIR_Coll_host_buffer_alloc(sendbuf, recvbuf, MPIR_Comm_size(comm_ptr) * recvcount, datatype,
                                &host_sendbuf, &host_recvbuf);
    if (host_sendbuf)
        sendbuf = host_sendbuf;
    if (host_recvbuf)
        recvbuf = host_recvbuf;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_REDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Reduce_scatter_block_impl(sendbuf, recvbuf, recvcount, datatype, op,
                                                   comm_ptr, errflag);
    }

    /* Copy out data from host recv buffer to GPU buffer */
    if (host_recvbuf) {
        recvbuf = in_recvbuf;
        MPIR_Localcopy(host_recvbuf, recvcount, datatype, recvbuf, recvcount, datatype);
    }

    MPIR_Coll_host_buffer_free(host_sendbuf, host_recvbuf);

    return mpi_errno;
}

#endif

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
                             int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_REDUCE_SCATTER_BLOCK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_REDUCE_SCATTER_BLOCK);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype *datatype_ptr = NULL;
            MPIR_Op *op_ptr = NULL;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            MPIR_ERRTEST_COUNT(recvcount, mpi_errno);

            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (!HANDLE_IS_BUILTIN(datatype)) {
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, recvcount, mpi_errno);
            } else if (sendbuf != MPI_IN_PLACE && recvcount != 0)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);

            MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, datatype, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(sendbuf, recvcount, datatype, mpi_errno);

            MPIR_ERRTEST_OP(op, mpi_errno);

            if (!HANDLE_IS_BUILTIN(op)) {
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr(op_ptr, mpi_errno);
            } else {
                mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (datatype);
            }
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr,
                                          &errflag);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_REDUCE_SCATTER_BLOCK);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_reduce_scatter_block",
                                 "**mpi_reduce_scatter_block %p %p %d %D %O %C", sendbuf, recvbuf,
                                 recvcount, datatype, op, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
