/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create_group */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create_group = PMPI_Comm_create_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create_group  MPI_Comm_create_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create_group as PMPI_Comm_create_group
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm) __attribute__((weak,alias("PMPI_Comm_create_group")));
#endif
/* -- End Profiling Symbol Block */


/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_create_group
#define MPI_Comm_create_group PMPI_Comm_create_group

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_group
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* comm create group impl; assumes that the standard error checking
 * has already taken place in the calling function */
int MPIR_Comm_create_group(MPID_Comm * comm_ptr, MPID_Group * group_ptr, int tag,
                           MPID_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_Context_id_t new_context_id = 0;
    int *mapping = NULL;
    int n;

    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_GROUP);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE_GROUP);

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);

    n = group_ptr->size;
    *newcomm_ptr = NULL;

    /* Create a new communicator from the specified group members */

    if (group_ptr->rank != MPI_UNDEFINED) {
        MPID_Comm *mapping_comm = NULL;

        /* For this routine, creation of the id is collective over the input
           *group*, so processes not in the group do not participate. */

        mpi_errno = MPIR_Get_contextid_sparse_group( comm_ptr, group_ptr, tag, &new_context_id, 0 );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIU_Assert(new_context_id != 0);

        mpi_errno = MPIR_Comm_create_calculate_mapping(group_ptr, comm_ptr, 
                                                       &mapping, &mapping_comm);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* Get the new communicator structure and context id */

        mpi_errno = MPIR_Comm_create( newcomm_ptr );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        (*newcomm_ptr)->recvcontext_id = new_context_id;
        (*newcomm_ptr)->rank           = group_ptr->rank;
        (*newcomm_ptr)->comm_kind      = comm_ptr->comm_kind;
        /* Since the group has been provided, let the new communicator know
           about the group */
        (*newcomm_ptr)->local_comm     = 0;
        (*newcomm_ptr)->local_group    = group_ptr;
        MPIR_Group_add_ref( group_ptr );

        (*newcomm_ptr)->remote_group   = group_ptr;
        MPIR_Group_add_ref( group_ptr );
        (*newcomm_ptr)->context_id     = (*newcomm_ptr)->recvcontext_id;
        (*newcomm_ptr)->remote_size    = (*newcomm_ptr)->local_size = n;

        /* Setup the communicator's vc table.  This is for the remote group,
           which is the same as the local group for intracommunicators */
        mpi_errno = MPIR_Comm_create_map(n, 0,
                                         mapping,
                                         NULL,
                                         mapping_comm,
                                         *newcomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* This process is not in the group */
        new_context_id = 0;
    }

fn_exit:
    if (mapping)
        MPIU_Free(mapping);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE_GROUP);
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (*newcomm_ptr != NULL) {
        MPIR_Comm_release(*newcomm_ptr);
        new_context_id = 0; /* MPIR_Comm_release frees the new ctx id */
    }
    if (new_context_id != 0)
        MPIR_Free_contextid(new_context_id);
    /* --END ERROR HANDLING-- */

    goto fn_exit;
}

#endif /* !defined(MPICH_MPI_FROM_PMPI) */

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create_group
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Comm_create_group - Creates a new communicator

Input Parameters:
+ comm - communicator (handle)
. group - group, which is a subset of the group of 'comm'  (handle)
- tag - safe tag unused by other communication

Output Parameters:
. newcomm - new communicator (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_GROUP

.seealso: MPI_Comm_free
@*/
int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPID_Group *group_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE_GROUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE_GROUP);

    /* Validate parameters, and convert MPI object handles to object pointers */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;

        MPID_Comm_get_ptr( comm, comm_ptr );

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno) goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);

            /* only test for MPI_GROUP_NULL after attempting to convert the comm
             * so that any errhandlers on comm will (correctly) be invoked */
            MPIR_ERRTEST_GROUP(group, mpi_errno);
            MPIR_ERRTEST_COMM_TAG(tag, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;

        MPID_Group_get_ptr( group, group_ptr );

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Check the group ptr */
            MPID_Group_valid_ptr( group_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   else
    {
        MPID_Comm_get_ptr( comm, comm_ptr );
        MPID_Group_get_ptr( group, group_ptr );
    }
#   endif

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_create_group(comm_ptr, group_ptr, tag, &newcomm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (newcomm_ptr)
        MPID_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    else
        *newcomm = MPI_COMM_NULL;
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_GROUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
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
