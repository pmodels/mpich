/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IGATHERV_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select igatherv algorithm
        auto   - Internal algorithm selection
        linear - Force linear algorithm

    - name        : MPIR_CVAR_IGATHERV_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select igatherv algorithm
        auto   - Internal algorithm selection
        linear - Force linear algorithm

    - name        : MPIR_CVAR_IGATHERV_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Igatherv will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level igatherv function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Igatherv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Igatherv = PMPI_Igatherv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Igatherv  MPI_Igatherv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Igatherv as PMPI_Igatherv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Igatherv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Igatherv
#define MPI_Igatherv PMPI_Igatherv

#undef FUNCNAME
#define FUNCNAME MPIR_Igatherv_sched_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igatherv_sched_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const int recvcounts[], const int displs[],
                                   MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                   MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Igatherv_sched_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                           displs, recvtype, root, comm_ptr, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Igatherv_sched_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igatherv_sched_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const int recvcounts[], const int displs[],
                                   MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                   MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Igatherv_sched_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                           displs, recvtype, root, comm_ptr, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Igatherv_sched_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igatherv_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const int recvcounts[], const int displs[],
                             MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_Igatherv_intra_algo_choice) {
            case MPIR_IGATHERV_INTRA_ALGO_LINEAR:
                mpi_errno =
                    MPIR_Igatherv_sched_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcounts, displs, recvtype, root, comm_ptr,
                                                       s);
                break;
            case MPIR_IGATHERV_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno =
                    MPIR_Igatherv_sched_intra_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                   recvcounts, displs, recvtype, root, comm_ptr, s);
                break;
        }
    } else {
        switch (MPIR_Igatherv_inter_algo_choice) {
            case MPIR_IGATHERV_INTER_ALGO_LINEAR:
                mpi_errno =
                    MPIR_Igatherv_sched_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcounts, displs, recvtype, root, comm_ptr,
                                                       s);
                break;
            case MPIR_IGATHERV_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno =
                    MPIR_Igatherv_sched_inter_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                   recvcounts, displs, recvtype, root, comm_ptr, s);
                break;
        }
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Igatherv_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                        const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root,
                        MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcounts, displs, recvtype, root, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Igatherv_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                             recvcounts, displs, recvtype, root, comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Igatherv_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                       const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                       int root, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = NULL;

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIR_Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype,
                            root, comm_ptr, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, request);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                  int root, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                  recvtype, root, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Igatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                       recvtype, root, comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Igatherv - Gathers into specified locations from all processes in a group
               in a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements in send buffer (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcounts - non-negative integer array (of length group size) containing the number of elements that are received from each process (significant only at root)
. displs - integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i (significant only at root)
. recvtype - data type of receive buffer elements (significant only at root) (handle)
. root - rank of receiving process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (significant only at root) (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IGATHERV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IGATHERV);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
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
            MPIR_Datatype *sendtype_ptr = NULL, *recvtype_ptr = NULL;
            int i, rank, comm_size;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                if (sendbuf != MPI_IN_PLACE) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }
                    MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);
                }

                rank = comm_ptr->rank;
                if (rank == root) {
                    comm_size = comm_ptr->local_size;
                    for (i = 0; i < comm_size; i++) {
                        MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    }
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }

                    for (i = 0; i < comm_size; i++) {
                        if (recvcounts[i] > 0) {
                            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[i], mpi_errno);
                            MPIR_ERRTEST_USERBUFFER(recvbuf, recvcounts[i], recvtype, mpi_errno);
                            break;
                        }
                    }

                    /* catch common aliasing cases */
                    if (sendbuf != MPI_IN_PLACE && sendtype == recvtype &&
                        recvcounts[comm_ptr->rank] != 0 && sendcount != 0) {
                        int recvtype_size;
                        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(sendbuf,
                                                (char *) recvbuf +
                                                displs[comm_ptr->rank] * recvtype_size, mpi_errno);
                    }
                } else
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
                    comm_size = comm_ptr->remote_size;
                    for (i = 0; i < comm_size; i++) {
                        MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    }
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }
                    for (i = 0; i < comm_size; i++) {
                        if (recvcounts[i] > 0) {
                            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[i], mpi_errno);
                            MPIR_ERRTEST_USERBUFFER(recvbuf, recvcounts[i], recvtype, mpi_errno);
                            break;
                        }
                    }
                } else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno =
        MPIR_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root,
                      comm_ptr, &request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IGATHERV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_igatherv", "**mpi_igatherv %p %d %D %p %p %p %D %d %C %p",
                                 sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                 recvtype, root, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
