/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*@
   mm_vc_from_communicator - get the virtual connection from a communicator and rank

   Parameters:
+  MPID_Comm *comm_ptr - communicator
-  int rank - rank

   Notes:
@*/
MPIDI_VC * mm_vc_from_communicator(MPID_Comm *comm_ptr, int rank)
{
    int mpi_errno;
    MPIDI_VC *vc_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_VC_FROM_COMMUNICATOR);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_VC_FROM_COMMUNICATOR);
    dbg_printf("mm_vc_from_communicator\n");

#ifdef MPICH_DEV_BUILD
    if ((comm_ptr == NULL) || (rank < 0) || (rank >= comm_ptr->remote_size))
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FROM_COMMUNICATOR);
	return NULL;
    }
#endif

    /* check if this communicator has its vc reference table initialized yet */
    if (comm_ptr->vcrt == NULL)
    {
	/* allocate a vc reference table */
	mpi_errno = MPID_VCRT_Create(comm_ptr->remote_size, &comm_ptr->vcrt);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FROM_COMMUNICATOR);
	    return NULL;
	}
	/* get an alias to the array of vc pointers */
	mpi_errno = MPID_VCRT_Get_ptr(comm_ptr->vcrt, &comm_ptr->vcr);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FROM_COMMUNICATOR);
	    return NULL;
	}
    }

    /* lock vcr */
    vc_ptr = comm_ptr->vcr[rank];
    if (vc_ptr == NULL)
    {
	/* allocate and connect a virtual connection */
	comm_ptr->vcr[rank] = vc_ptr = mm_vc_connect_alloc(comm_ptr, rank);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FROM_COMMUNICATOR);
    return vc_ptr;
}

/*@
   mm_vc_from_context - get the virtual connection pointer from a remote context and rank

   Parameters:
+  int comm_context - context of the communicator
-  int rank - rank

   Notes:
@*/
MPIDI_VC * mm_vc_from_context(int comm_context, int rank)
{
    int mpi_errno;
    MPIDI_VC *vc_ptr;
    MPID_Comm *comm_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_VC_FROM_CONTEXT);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_VC_FROM_CONTEXT);
    dbg_printf("mm_vc_from_context\n");

    /*comm_ptr = MPID_Get_comm_from_context(comm_context); */
    comm_ptr = MPIR_Process.comm_world;

    /* check if this communicator has its vc reference table initialized yet */
    if (comm_ptr->vcrt == NULL)
    {
	/* allocate a vc reference table */
	mpi_errno = MPID_VCRT_Create(comm_ptr->remote_size, &comm_ptr->vcrt);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FROM_CONTEXT);
	    return NULL;
	}
	/* get an alias to the array of vc pointers */
	mpi_errno = MPID_VCRT_Get_ptr(comm_ptr->vcrt, &comm_ptr->vcr);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FROM_CONTEXT);
	    return NULL;
	}
    }

    vc_ptr = comm_ptr->vcr[rank];
    if (vc_ptr == NULL)
    {
	/* allocate a virtual connection */
	comm_ptr->vcr[rank] = vc_ptr = mm_vc_alloc(MM_UNBOUND_METHOD);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FROM_CONTEXT);
    return vc_ptr;
}
