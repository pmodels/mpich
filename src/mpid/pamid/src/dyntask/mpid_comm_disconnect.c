/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */

#include "mpidimpl.h"

#ifdef DYNAMIC_TASKING

extern conn_info *_conn_info_list;
/*@
   MPID_Comm_disconnect - Disconnect a communicator

   Arguments:
.  comm_ptr - communicator

   Notes:

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Comm_disconnect(MPID_Comm *comm_ptr)
{
    int rc, i,ref_count,mpi_errno, probe_flag=0;
    MPI_Status status;
    MPIDI_PG_t *pg;

    if(comm_ptr->mpid.world_ids != NULL) {
	rc = MPID_Iprobe(comm_ptr->rank, MPI_ANY_TAG, comm_ptr, MPID_CONTEXT_INTER_PT2PT, &probe_flag, &status);
        if(rc || probe_flag) {
          TRACE_ERR("PENDING_PTP");
	  exit(1);
        }

        for(i=0; comm_ptr->mpid.world_ids[i] != -1; i++) {
          ref_count = MPIDI_Decrement_ref_count(comm_ptr->mpid.world_ids[i]);
          TRACE_ERR("ref_count=%d with world=%d comm_ptr=%x\n", ref_count, comm_ptr->mpid.world_ids[i], comm_ptr);
          if(ref_count == -1)
	    TRACE_ERR("something is wrong\n");
        }

        MPIU_Free(comm_ptr->mpid.world_ids);
        mpi_errno = MPIR_Comm_release(comm_ptr,1);
        if (mpi_errno) TRACE_ERR("MPIR_Comm_release returned with mpi_errno=%d\n", mpi_errno);
    }
    return mpi_errno;
}


int MPIDI_Decrement_ref_count(int wid) {
  conn_info *tmp_node;
  int ref_count=-1;

  tmp_node = _conn_info_list;
  while(tmp_node != NULL) {
    if(tmp_node->rem_world_id == wid) {
      ref_count = --tmp_node->ref_count;
      TRACE_ERR("decrement_ref_count: ref_count decremented to %d for remote world %d\n",ref_count,wid);
      break;
    }
    tmp_node = tmp_node->next;
  }
  return ref_count;
}
#endif
