/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BCAST_MIN_PROCS
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : device
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
      class       : device
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
      class       : device
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

    - name        : MPIR_CVAR_ENABLE_SMP_BCAST
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : Enable SMP aware broadcast (See also: MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE)

    - name        : MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum message size for which SMP-aware broadcast is used.  A
        value of '0' uses SMP-aware broadcast for all message sizes.
        (See also: MPIR_CVAR_ENABLE_SMP_BCAST)

    - name        : MPIR_CVAR_BCAST_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select bcast algorithm
        auto                                    - Internal algorithm selection
        binomial                                - Force Binomial Tree
        nb                                      - Force nonblocking algorithm
        scatter_recursive_doubling_allgather    - Force Scatter Recursive-Doubling Allgather
        scatter_ring_allgather                  - Force Scatter Ring

    - name        : MPIR_CVAR_BCAST_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select bcast algorithm
        auto                    - Internal algorithm selection
        nb                      - Force nonblocking algorithm
        remote_send_local_bcast - Force remote-send-local-bcast algorithm

    - name        : MPIR_CVAR_BCAST_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Bcast will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.  If set to false,
        the device-level bcast function will not be called.

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

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast_intra_auto(void *buffer,
                          int count,
                          MPI_Datatype datatype,
                          int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size;
    MPI_Aint nbytes = 0;
    MPI_Aint type_size;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_BCAST);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_BCAST);

    if (count == 0)
        goto fn_exit;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE ? type_size * count : 0;
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BCAST &&
        nbytes <= MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE && MPIR_Comm_is_node_aware(comm_ptr)) {
        mpi_errno = MPIR_Bcast_intra_smp(buffer, count, datatype, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        goto fn_exit;
    }

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, errflag);
    } else {    /* (nbytes >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_size >= MPIR_CVAR_BCAST_MIN_PROCS) */

        if ((nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE) && (MPL_is_pof2(comm_size, NULL))) {
            mpi_errno =
                MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype, root,
                                                                      comm_ptr, errflag);
        } else {        /* (nbytes >= MPIR_CVAR_BCAST_LONG_MSG_SIZE) || !(comm_size_is_pof2) */

            mpi_errno =
                MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr,
                                                        errflag);
        }
    }
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_BCAST);

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast_inter_auto(void *buffer,
                          int count,
                          MPI_Datatype datatype,
                          int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Bcast_inter_remote_send_local_bcast(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast_impl(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                    MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Bcast_intra_algo_choice) {
            case MPIR_BCAST_INTRA_ALGO_BINOMIAL:
                mpi_errno =
                    MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_BCAST_INTRA_ALGO_SCATTER_RECURSIVE_DOUBLING_ALLGATHER:
                mpi_errno =
                    MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype,
                                                                          root, comm_ptr, errflag);
                break;
            case MPIR_BCAST_INTRA_ALGO_SCATTER_RING_ALLGATHER:
                mpi_errno =
                    MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr,
                                                            errflag);
                break;
            case MPIR_BCAST_INTRA_ALGO_NB:
                mpi_errno = MPIR_Bcast_allcomm_nb(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_BCAST_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Bcast_intra_auto(buffer, count, datatype, root, comm_ptr, errflag);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Bcast_inter_algo_choice) {
            case MPIR_BCAST_INTER_ALGO_REMOTE_SEND_LOCAL_BCAST:
                mpi_errno =
                    MPIR_Bcast_inter_remote_send_local_bcast(buffer, count, datatype, root,
                                                             comm_ptr, errflag);
                break;
            case MPIR_BCAST_INTER_ALGO_NB:
                mpi_errno = MPIR_Bcast_allcomm_nb(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_BCAST_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Bcast_inter_auto(buffer, count, datatype, root, comm_ptr, errflag);
                break;
        }
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
               MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_BCAST_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm_ptr, errflag);
    }

    return mpi_errno;
}


#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

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

            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
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
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

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
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_bcast", "**mpi_bcast %p %d %D %d %C", buffer, count,
                                 datatype, root, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
