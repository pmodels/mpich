/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_create_group */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_create_group = PMPIX_Comm_create_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_create_group  MPIX_Comm_create_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_create_group as PMPIX_Comm_create_group
#endif
/* -- End Profiling Symbol Block */

PMPI_LOCAL int MPIR_Comm_create_group(MPID_Comm * comm_ptr, MPID_Group * group_ptr, int tag,
                                      MPID_Comm ** newcomm);


/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_create_group
#define MPIX_Comm_create_group PMPIX_Comm_create_group

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_group
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* comm create group impl; assumes that the standard error checking
 * has already taken place in the calling function */
PMPI_LOCAL int MPIR_Comm_create_group(MPID_Comm * comm_ptr, MPID_Group * group_ptr, int tag,
                                      MPID_Comm ** newcomm_ptr)
{
    int i, grp_me, me, merge_size;
    MPID_Comm *tmp_comm_ptr, *tmp_intercomm_ptr, *comm_self_ptr;
    MPID_Group *comm_group_ptr = NULL;
    int *granks = NULL, *ranks = NULL, rank_count;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_GROUP);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE_GROUP);

    ranks = NULL;

    /* Create a group for all of comm and translate it to "group",
     * which should be a subset of comm's group. If implemented inside
     * MPI, the below translation can be done more efficiently, since
     * we have access to the internal lpids directly. */
    mpi_errno = MPIR_Comm_group_impl(comm_ptr, &comm_group_ptr);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    rank_count = group_ptr->size;

    MPIU_CHKLMEM_MALLOC(granks, int *, rank_count * sizeof(int), mpi_errno, "granks");
    MPIU_CHKLMEM_MALLOC(ranks, int *, rank_count * sizeof(int), mpi_errno, "ranks");

    for (i = 0; i < rank_count; i++)
        granks[i] = i;

    mpi_errno =
        MPIR_Group_translate_ranks_impl(group_ptr, rank_count, granks, comm_group_ptr, ranks);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    me = MPIR_Comm_rank(comm_ptr);

    /* If the group size is 0, return MPI_COMM_NULL */
    if (rank_count == 0) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    MPID_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);

    /* If I am the only process in the group, return a dup of
     * MPI_COMM_SELF */
    if (rank_count == 1 && ranks[0] == me) {
        mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, newcomm_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    /* If I am not a part of the group, return MPI_COMM_NULL */
    grp_me = -1;
    for (i = 0; i < rank_count; i++) {
        if (ranks[i] == me) {
            grp_me = i;
            break;
        }
    }
    if (grp_me < 0) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, &tmp_comm_ptr);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    for (merge_size = 1; merge_size < rank_count; merge_size *= 2) {
        int gid = grp_me / merge_size;
        MPID_Comm *save_comm = tmp_comm_ptr;

        if (gid % 2 == 0) {
            /* Check if right partner doesn't exist */
            if ((gid + 1) * merge_size >= rank_count)
                continue;

            mpi_errno = MPIR_Intercomm_create_impl(tmp_comm_ptr, 0, comm_ptr,
                                                   ranks[(gid + 1) * merge_size], tag,
                                                   &tmp_intercomm_ptr);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);

            mpi_errno =
                MPIR_Intercomm_merge_impl(tmp_intercomm_ptr, 0 /* LOW */ , &tmp_comm_ptr);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
        }
        else {
            mpi_errno = MPIR_Intercomm_create_impl(tmp_comm_ptr, 0, comm_ptr,
                                                   ranks[(gid - 1) * merge_size], tag,
                                                   &tmp_intercomm_ptr);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);

            mpi_errno =
                MPIR_Intercomm_merge_impl(tmp_intercomm_ptr, 1 /* HIGH */ , &tmp_comm_ptr);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIR_Comm_free_impl(tmp_intercomm_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        if (save_comm->handle != MPI_COMM_SELF) {
            mpi_errno = MPIR_Comm_free_impl(save_comm);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
        }
    }

    *newcomm_ptr = tmp_comm_ptr;

  fn_exit:
    if (comm_group_ptr != NULL) {
        if (mpi_errno)  /* avoid squashing a more interesting error code */
            MPIR_Group_free_impl(comm_group_ptr);
        else
            mpi_errno = MPIR_Group_free_impl(comm_group_ptr);
    }
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE_INTER);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#endif /* !defined(MPICH_MPI_FROM_PMPI) */

#undef FUNCNAME
#define FUNCNAME MPIX_Comm_create_group
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@

MPIX_Comm_create_group - Creates a new communicator

Input Parameters:
+ comm - communicator (handle)
. group - group, which is a subset of the group of 'comm'  (handle)
- tag - safe tag unused by other communication

Output Parameter:
. newcomm - new communicator (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_GROUP

.seealso: MPI_Comm_free
@*/
int MPIX_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPID_Group *group_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE);

    /* Validate parameters, and convert MPI object handles to object pointers */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;

        MPID_Comm_get_ptr(comm, comm_ptr);

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            /* If comm_ptr is not valid, it will be reset to null */

            MPIR_ERRTEST_GROUP(group, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;

        MPID_Group_get_ptr(group, group_ptr);

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Check the group ptr */
            MPID_Group_valid_ptr(group_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#else
    {
        MPID_Comm_get_ptr(comm, comm_ptr);
        MPID_Group_get_ptr(group, group_ptr);
    }
#endif

    /* ... body of routine ...  */
    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);
    mpi_errno = MPIR_Comm_create_group(comm_ptr, group_ptr, tag, &newcomm_ptr);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    if (newcomm_ptr)
        MPIU_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    else
        *newcomm = MPI_COMM_NULL;
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_comm_create_group",
                                 "**mpi_comm_create_group %C %G %d %p", comm, group, tag,
                                 newcomm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
