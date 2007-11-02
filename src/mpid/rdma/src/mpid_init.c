/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

MPIDI_Process_t MPIDI_Process;

#undef FUNCNAME
#define FUNCNAME MPID_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Init(int * argc, char *** argv, int requested, int * provided, int * has_args, int * has_env)
{
    int mpi_errno = MPI_SUCCESS;
    int has_parent;
#ifdef HAVE_WINDOWS_H
    DWORD size = 128;
#endif
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    
    MPIDI_Process.recvq_posted_head = NULL;
    MPIDI_Process.recvq_posted_tail = NULL;
    MPIDI_Process.recvq_unexpected_head = NULL;
    MPIDI_Process.recvq_unexpected_tail = NULL;
    MPID_Thread_lock_init(&MPIDI_Process.recvq_mutex);
    
    /*
     * Initialize the collective operations for the MPI_COMM_WORLD and MPI_COMM_SELF
     *
     * NOTE: now using the default collective operations supplied by MPICH
     */
#   if FALSE
    {
	MPID_Collops * collops;
	
	collops = MPIU_Calloc(1, sizeof(MPID_Collops));
	assert(collops != NULL);

	collops->ref_count = 2;
	collops->Barrier = MPIDI_Barrier;
	collops->Bcast = NULL;
	collops->Gather = NULL;
	collops->Gatherv = NULL;
	collops->Scatter = NULL;
	collops->Scatterv = NULL;
	collops->Allgather = NULL;
	collops->Allgatherv = NULL;
	collops->Alltoall = NULL;
	collops->Alltoallv = NULL;
	collops->Alltoallw = NULL;
	collops->Reduce = NULL;
	collops->Allreduce = NULL;
	collops->Reduce_scatter = NULL;
	collops->Scan = NULL;
	collops->Exscan = NULL;
    
	MPIR_Process.comm_world->coll_fns = collops;
	MPIR_Process.comm_self->coll_fns = collops;
    }
#   endif
    
    /*
     * Set process attributes.  These can be overridden by the channel if necessary.
     */
    MPIR_Process.attrs.tag_ub          = INT_MAX;
    
#   if defined(HAVE_GETHOSTNAME)
    {
	/* The value 128 is returned by the ch3/Makefile for the echomaxprocname target.  */
	MPIDI_Process.processor_name = MPIU_Malloc(128);
#ifdef HAVE_WINDOWS_H
	if (GetComputerName(MPIDI_Process.processor_name, &size) == 0)
	{
	    MPIU_Free(MPIDI_Process.processor_name);
	    MPIDI_Process.processor_name = NULL;
	}
#else
	if(gethostname(MPIDI_Process.processor_name, 128) != 0)
	{
	    MPIU_Free(MPIDI_Process.processor_name);
	    MPIDI_Process.processor_name = NULL;
	}
#endif
    }
#   else
    {
	MPIDI_Process.processor_name = NULL;
    }
#   endif
    
    /*
     * Let the channel perform any necessary initialization
     */
    mpi_errno = MPIDI_CH3_Init(argc, argv, has_args, has_env, &has_parent);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init", 0);
	return mpi_errno;
    }

    /* MT:  only single threaded applications are supported at the moment... */
    if (provided != NULL)
    {
	*provided = MPI_THREAD_SINGLE;
    }

    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    return mpi_errno;
}
