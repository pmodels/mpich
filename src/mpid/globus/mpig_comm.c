/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

/*
 * internal data structures
 */
MPIG_STATIC mpig_genq_t mpig_comm_active_list;

/*
 * int mpig_comm_init(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_init
int mpig_comm_init(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_init);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering"));

    mpig_genq_construct(&mpig_comm_active_list);

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_init);
    return mpi_errno;
}
/* mpig_comm_init() */

/*
 * int mpig_comm_finalize(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_finalize
int mpig_comm_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_genq_entry_t * comm_q_entry;
    MPID_Comm * comm;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_finalize);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering"));

    /* release application references to communicators */
    comm_q_entry = mpig_genq_peek_head_entry(&mpig_comm_active_list);
    while (comm_q_entry != NULL)
    {
        comm = mpig_genq_entry_get_value(comm_q_entry);
        MPIU_Assert(HANDLE_GET_KIND(comm->handle) != HANDLE_KIND_INVALID);

        /* the call to MPIR_Comm_release() below may cause the communicator object containing the genq entry to be destroyed.
           therefore, the next entry in the queue must be acquired before MPIR_Comm_release() is called. */
        comm_q_entry = mpig_genq_entry_get_next(comm_q_entry);
	
        if (comm->dev.app_ref && HANDLE_GET_KIND(comm->handle) != HANDLE_KIND_BUILTIN)
        {
            MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "releasing communicator not freed by application: comm=" MPIG_HANDLE_FMT
                ",commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));
            comm->dev.app_ref = FALSE;
            MPIR_Comm_release(comm);
        }
    }
    
    /* wait for all requests on all communicators to complete */
    while (mpig_genq_is_empty(&mpig_comm_active_list) == FALSE)
    {
	MPID_Progress_state pe_state;
	
        comm_q_entry = mpig_genq_peek_head_entry(&mpig_comm_active_list);
        comm = mpig_genq_entry_get_value(comm_q_entry);
        
	MPID_Progress_start(&pe_state);
	{
	    while(mpig_genq_peek_head_entry(&mpig_comm_active_list) == comm_q_entry && comm->ref_count > 0)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "waiting for operations on communicator to complete: comm="
		    MPIG_HANDLE_FMT ",commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));
		mpi_errno = MPID_Progress_wait(&pe_state);
		if (mpi_errno)
		{   /* --BEGIN ERROR HANDLING-- */
		    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|pe_wait");
		    MPID_Progress_end(&pe_state);
		    goto fn_fail;
		}   /* --END ERROR HANDLING-- */
	    }
	}
	MPID_Progress_end(&pe_state);

	/* MPIR_Comm_release() is never called for the predefined communicators, so mpig_comm_destruct() is called here to
	   release any device related resources still held by the communicator and remove the communicator from the list of
	   active communicators. */
	if (mpig_genq_peek_head_entry(&mpig_comm_active_list) == comm_q_entry &&
            HANDLE_GET_KIND(comm->handle) == HANDLE_KIND_BUILTIN)
	{
	    mpig_comm_destruct(comm);
	}
    }
    
    mpig_genq_destruct(&mpig_comm_active_list, NULL);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_finalize);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_comm_finalize() */

/*
 * int mpig_comm_construct([IN/MOD] comm)
 *
 * comm [IN/MOD] - communicator being created
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_construct
int mpig_comm_construct(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_construct);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_construct);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));

    mpi_errno = mpig_topology_comm_construct(comm);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|topology_comm_construct",
	"*globus|topology_comm_construct %C", comm->handle);

    comm->dev.app_ref = TRUE;
    mpig_genq_entry_construct(&comm->dev.active_list);
    mpig_genq_entry_set_value(&comm->dev.active_list, comm);
    mpig_genq_enqueue_tail_entry(&mpig_comm_active_list, &comm->dev.active_list);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, comm->handle, MPIG_PTR_CAST(comm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_construct);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_comm_construct() */

/*
 * int mpig_comm_destruct([IN/MOD] comm)
 *
 * comm [IN/MOD] - communicator being destroyed
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_destruct
int mpig_comm_destruct(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_destruct);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_destruct);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));

    /* if this communicator was created by MPIR_Setup_intercomm_localcomm(), then skip the rest of the destruction process.  see
       comments below. */
    if (mpig_genq_entry_get_value(&comm->dev.active_list) == NULL) goto fn_return;
    
    /* call the VMPI CM's hook, allowing it to free the associated vendor communicators */
#   if defined(MPIG_VMPI)
    {
	mpig_cm_vmpi_comm_destruct_hook(comm);
	/* FIXME: convert this into a generic CM function table list/array so that any CM can register hooks */
    }
#   endif /* #if defined(MPIG_VMPI) */

    /* a local intracommunicator may be attached to an intercommunicator.  this intracommunicator is created locally (meaning not
       collectively) by MPIR_Setup_intercomm_localcomm() when needed.  the device is not notified of its creation and thus has
       not intialized any of the device information.  we set the active list previous and next fields of the local
       intracommunicator to point at itself so that we can detect when one of these local intracommuncators is being destroyed
       and skip the destruction process. */
    if (comm->comm_kind == MPID_INTERCOMM)
    {
	MPID_Comm * const local_comm = comm->local_comm;
	
	if (local_comm != NULL)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "encountered an intercommunicator with a local intracommunicator: comm="
		MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT ", local_comm"  MPIG_HANDLE_FMT ", local_commp=" MPIG_PTR_FMT, 
		comm->handle, MPIG_PTR_CAST(comm), local_comm->handle, MPIG_PTR_CAST(local_comm)));
	    
            local_comm->dev.app_ref = FALSE;
            mpig_genq_entry_construct(&local_comm->dev.active_list);
            mpig_genq_entry_set_value(&local_comm->dev.active_list, NULL);
	}
    }

    MPIU_Assert(comm->dev.app_ref == FALSE);
    mpig_genq_remove_entry(&mpig_comm_active_list, &comm->dev.active_list);
    mpig_genq_entry_destruct(&comm->dev.active_list);
    
    mrc = mpig_topology_comm_destruct(comm);
    if (mrc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_ERR_ADD(mpi_errno, mrc);
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|topology_comm_destruct", "*globus|topology_comm_destruct %C",
	    comm->handle);
    }   /* --BEGIN ERROR HANDLING-- */

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, comm->handle, MPIG_PTR_CAST(comm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_destruct);
    return mpi_errno;
}
/* mpig_comm_destruct() */

/*
 * <mpi_errno> mpig_comm_free_hook[IN/MOD] comm)
 *
 * comm [IN/MOD] - communicator being freed
 * mpi_errno [OUT] - MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_free_hook
int mpig_comm_free_hook(MPID_Comm * comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_free_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_free_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));
    
    /* clear the application reference flag to indicate that the application no longer holds a reference to this communicator */
    comm->dev.app_ref = FALSE;

    /* call the VMPI CM's hook, allowing it to free the associated vendor communicators */
#   if defined(MPIG_VMPI)
    {
	mpi_errno = mpig_cm_vmpi_comm_free_hook(comm);
	/* FIXME: convert this into a generic CM function table list/array so that any CM can register hooks */
    }
#   endif /* #if defined(MPIG_VMPI) */

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_free_hook);
    return mpi_errno;
}
/* mpig_comm_free_hook() */
