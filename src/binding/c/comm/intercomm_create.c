/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Intercomm_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Intercomm_create = PMPI_Intercomm_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Intercomm_create  MPI_Intercomm_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Intercomm_create as PMPI_Intercomm_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm,
                         int remote_leader, int tag, MPI_Comm *newintercomm)
                          __attribute__ ((weak, alias("PMPI_Intercomm_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Intercomm_create
#define MPI_Intercomm_create PMPI_Intercomm_create

#endif

/*@
   MPI_Intercomm_create - Creates an intercommuncator from two intracommunicators

Input Parameters:
+ local_comm - local intra-communicator (handle)
. local_leader - rank of local group leader in local_comm (integer)
. peer_comm - ``peer communicator; significant only at the local_leader (handle)
. remote_leader - rank of remote group leader in peer_comm; significant only at the local_leader (integer)
- tag - tag (integer)

Output Parameters:
. newintercomm - new inter-communicator (handle)

Notes:
   'peer_comm' is significant only for the process designated the
   'local_leader' in the 'local_comm'.

  The MPI 1.1 Standard contains two mutually exclusive comments on the
  input intercommunicators.  One says that their repective groups must be
  disjoint; the other that the leaders can be the same process.  After
  some discussion by the MPI Forum, it has been decided that the groups must
  be disjoint.  Note that the `reason` given for this in the standard is
  `not` the reason for this choice; rather, the `other` operations on
  intercommunicators (like 'MPI_Intercomm_merge') do not make sense if the
  groups are not disjoint.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_COMM
.N MPI_ERR_TAG
.seealso: MPI_Intercomm_merge, MPI_Comm_free, MPI_Comm_remote_group, MPI_Comm_remote_size
@*/

int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm,
                         int remote_leader, int tag, MPI_Comm *newintercomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *local_comm_ptr = NULL;
    MPIR_Comm *peer_comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INTERCOMM_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INTERCOMM_CREATE);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(local_comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Comm_get_ptr(local_comm, local_comm_ptr);
    MPIR_Comm_get_ptr(peer_comm, peer_comm_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_SEND_TAG(tag, mpi_errno);
            MPIR_ERRTEST_ARGNULL(newintercomm, "newintercomm", mpi_errno);
            /* Validate local_comm_ptr */
            MPIR_Comm_valid_ptr(local_comm_ptr, mpi_errno, FALSE);
            if (local_comm_ptr) {
                /*  Only check if local_comm_ptr valid */
                MPIR_ERRTEST_COMM_INTRA(local_comm_ptr, mpi_errno);
                if ((local_leader < 0 || local_leader >= local_comm_ptr->local_size)) {
                    MPIR_ERR_SET2(mpi_errno, MPI_ERR_RANK,
                                    "**ranklocal", "**ranklocal %d %d",
                                    local_leader, local_comm_ptr->local_size - 1);
                    /* If local_comm_ptr is not valid, it will be reset to null */
                    if (mpi_errno)
                        goto fn_fail;
                }
                if (local_comm_ptr->rank == local_leader) {
                    MPIR_ERRTEST_COMM(peer_comm, mpi_errno);
                    MPIR_Comm_valid_ptr(peer_comm_ptr, mpi_errno, FALSE);
                    /* Note: In MPI 1.0, peer_comm was restricted to
                        * intracommunicators.  In 1.1, it may be any communicator */

                    /* In checking the rank of the remote leader,
                        * allow the peer_comm to be in intercommunicator
                        * by checking against the remote size */
                    if (!mpi_errno && peer_comm_ptr &&
                        (remote_leader < 0 || remote_leader >= peer_comm_ptr->remote_size)) {
                        MPIR_ERR_SET2(mpi_errno, MPI_ERR_RANK,
                                        "**rankremote", "**rankremote %d %d",
                                        remote_leader, peer_comm_ptr->remote_size - 1);
                    }
                    /* Check that the local leader and the remote leader are
                        * different processes.  This test requires looking at
                        * the lpid for the two ranks in their respective
                        * communicators.  However, an easy test is for
                        * the same ranks in an intracommunicator; we only
                        * need the lpid comparison for intercommunicators */
                    /* Here is the test.  We restrict this test to the
                        * process that is the local leader (local_comm_ptr->rank ==
                        * local_leader because we can then use peer_comm_ptr->rank
                        * to get the rank in peer_comm of the local leader. */
                    if (peer_comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                        local_comm_ptr->rank == local_leader && peer_comm_ptr->rank == remote_leader) {
                        MPIR_ERR_SET(mpi_errno, MPI_ERR_RANK, "**ranksdistinct");
                    }
                    if (mpi_errno)
                        goto fn_fail;
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Comm *newintercomm_ptr = NULL;
    *newintercomm = MPI_COMM_NULL;
    mpi_errno = MPIR_Intercomm_create_impl(local_comm_ptr, local_leader, peer_comm_ptr, remote_leader,
                                           tag, &newintercomm_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    if (newintercomm_ptr) {
        MPIR_OBJ_PUBLISH_HANDLE(*newintercomm, newintercomm_ptr->handle);
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_intercomm_create",
                                     "**mpi_intercomm_create %C %i %C %i %t %p", local_comm,
                                     local_leader, peer_comm, remote_leader, tag, newintercomm);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
