/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BCAST_MIN_PROCS
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Let's define short messages as messages with size < MPIR_CVAR_BCAST_SHORT_MSG_SIZE,
        and medium messages as messages with size >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE but
        < MPIR_CVAR_BCAST_LONG_MSG_SIZE, and long messages as messages with size >=
        MPIR_CVAR_BCAST_LONG_MSG_SIZE. The broadcast algorithms selection procedure is
        as follows. For short messages or when the number of processes is <
        MPIR_CVAR_BCAST_MIN_PROCS, we do broadcast using the binomial tree algorithm.
        Otherwise, for medium messages and with a power-of-two number of processes, we do
        broadcast based on a scatter followed by a recursive doubling allgather algorithm.
        Otherwise, for long messages or with non power-of-two number of processes, we do
        broadcast based on a scatter followed by a ring allgather algorithm.
        (See also: MPIR_CVAR_BCAST_SHORT_MSG_SIZE, MPIR_CVAR_BCAST_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_BCAST_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 12288
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Let's define short messages as messages with size < MPIR_CVAR_BCAST_SHORT_MSG_SIZE,
        and medium messages as messages with size >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE but
        < MPIR_CVAR_BCAST_LONG_MSG_SIZE, and long messages as messages with size >=
        MPIR_CVAR_BCAST_LONG_MSG_SIZE. The broadcast algorithms selection procedure is
        as follows. For short messages or when the number of processes is <
        MPIR_CVAR_BCAST_MIN_PROCS, we do broadcast using the binomial tree algorithm.
        Otherwise, for medium messages and with a power-of-two number of processes, we do
        broadcast based on a scatter followed by a recursive doubling allgather algorithm.
        Otherwise, for long messages or with non power-of-two number of processes, we do
        broadcast based on a scatter followed by a ring allgather algorithm.
        (See also: MPIR_CVAR_BCAST_MIN_PROCS, MPIR_CVAR_BCAST_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_BCAST_LONG_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 524288
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Let's define short messages as messages with size < MPIR_CVAR_BCAST_SHORT_MSG_SIZE,
        and medium messages as messages with size >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE but
        < MPIR_CVAR_BCAST_LONG_MSG_SIZE, and long messages as messages with size >=
        MPIR_CVAR_BCAST_LONG_MSG_SIZE. The broadcast algorithms selection procedure is
        as follows. For short messages or when the number of processes is <
        MPIR_CVAR_BCAST_MIN_PROCS, we do broadcast using the binomial tree algorithm.
        Otherwise, for medium messages and with a power-of-two number of processes, we do
        broadcast based on a scatter followed by a recursive doubling allgather algorithm.
        Otherwise, for long messages or with non power-of-two number of processes, we do
        broadcast based on a scatter followed by a ring allgather algorithm.
        (See also: MPIR_CVAR_BCAST_MIN_PROCS, MPIR_CVAR_BCAST_SHORT_MSG_SIZE)

    - name        : MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum message size for which SMP-aware broadcast is used.  A
        value of '0' uses SMP-aware broadcast for all message sizes.

    - name        : MPIR_CVAR_BCAST_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select bcast algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        binomial                                - Force Binomial Tree
        nb                                      - Force nonblocking algorithm
        smp                                     - Force smp algorithm
        scatter_recursive_doubling_allgather    - Force Scatter Recursive-Doubling Allgather
        scatter_ring_allgather                  - Force Scatter Ring

    - name        : MPIR_CVAR_BCAST_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select bcast algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb                      - Force nonblocking algorithm
        remote_send_local_bcast - Force remote-send-local-bcast algorithm

    - name        : MPIR_CVAR_BCAST_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Bcast will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Bcast */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Bcast = PMPI_Bcast
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Bcast  MPI_Bcast
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Bcast as PMPI_Bcast
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Bcast")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Bcast
#define MPI_Bcast PMPI_Bcast


int MPIR_Bcast_allcomm_auto(void *buffer, int count, MPI_Datatype datatype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__BCAST,
        .comm_ptr = comm_ptr,

        .u.bcast.buffer = buffer,
        .u.bcast.count = count,
        .u.bcast.datatype = datatype,
        .u.bcast.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_binomial:
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_recursive_doubling_allgather:
            mpi_errno =
                MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype, root,
                                                                      comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_ring_allgather:
            mpi_errno =
                MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr,
                                                        errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_smp:
            mpi_errno = MPIR_Bcast_intra_smp(buffer, count, datatype, root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_inter_remote_send_local_bcast:
            mpi_errno =
                MPIR_Bcast_inter_remote_send_local_bcast(buffer, count, datatype, root, comm_ptr,
                                                         errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_allcomm_nb:
            mpi_errno = MPIR_Bcast_allcomm_nb(buffer, count, datatype, root, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Bcast_impl(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                    MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_BCAST_INTRA_ALGORITHM) {
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_binomial:
                mpi_errno =
                    MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_scatter_recursive_doubling_allgather:
                mpi_errno =
                    MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype,
                                                                          root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_scatter_ring_allgather:
                mpi_errno =
                    MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr,
                                                            errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Bcast_allcomm_nb(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_smp:
                mpi_errno = MPIR_Bcast_intra_smp(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Bcast_allcomm_auto(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_BCAST_INTER_ALGORITHM) {
            case MPIR_CVAR_BCAST_INTER_ALGORITHM_remote_send_local_bcast:
                mpi_errno =
                    MPIR_Bcast_inter_remote_send_local_bcast(buffer, count, datatype, root,
                                                             comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Bcast_allcomm_nb(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Bcast_allcomm_auto(buffer, count, datatype, root, comm_ptr, errflag);
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

int MPIR_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
               MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BCAST_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm_ptr, errflag);
    }

    return mpi_errno;
}


#endif /* MPICH_MPI_FROM_PMPI */


/*@
MPI_Bcast - Broadcasts a message from the process with rank "root" to
            all other processes of the communicator

Input/Output Parameters:
. buffer - starting address of buffer (choice)

Input Parameters:
+ count - number of entries in buffer (integer)
. datatype - data type of buffer (handle)
. root - rank of broadcast root (integer)
- comm - communicator (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_ROOT
@*/
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_BCAST);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_BCAST);

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

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
            } else {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);
            }

            if (!HANDLE_IS_BUILTIN(datatype)) {
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            MPIR_ERRTEST_BUF_INPLACE(buffer, count, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(buffer, count, datatype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_BCAST);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_bcast", "**mpi_bcast %p %d %D %d %C", buffer, count,
                                 datatype, root, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
