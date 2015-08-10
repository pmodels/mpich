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

#define DISCONNECT_LAPI_XFER_TIMEOUT  5*60*1000000
#define TOTAL_AM    3
#define FIRST_AM    0
#define SECOND_AM   1
#define LAST_AM     2

/* Returns time in micro seconds "double ticks" */
#define CURTIME(ticks) {                                        \
  struct timeval tp;                                            \
  struct timezone tzp;                                          \
  gettimeofday(&tp,&tzp);                                       \
  ticks = (double) tp.tv_sec * 1000000 + (double) tp.tv_usec;   \
}


/* Used inside termination message send by smaller task to larger task in discon
nect */
typedef struct {
  long long tranid;
  int       whichAM;
}AM_struct2;


extern transactionID_struct *_transactionID_list;
void MPIDI_get_allremote_leaders(int *tid_arr, MPID_Comm *comm_ptr);

void MPIDI_send_AM_to_remote_leader_on_disconnect(int taskid, long long comm_cntr, int whichAM)
{
   pami_send_immediate_t xferP;

   int              rc, current_val;
   AM_struct2       AM_data;
   pami_endpoint_t  dest;

   AM_data.tranid  = comm_cntr;
   AM_data.whichAM = whichAM;

   memset(&xferP, 0, sizeof(pami_send_immediate_t));
   xferP.header.iov_base = (void*)&AM_data;
   xferP.header.iov_len  = sizeof(AM_struct2);
   xferP.dispatch = (size_t)MPIDI_Protocols_Dyntask_disconnect;

   rc = PAMI_Endpoint_create(MPIDI_Client, taskid, 0, &dest);
   TRACE_ERR("PAMI_Resume to taskid=%d\n", taskid);
        PAMI_Resume(MPIDI_Context[0],
                    &dest, 1);

   if(rc != 0)
     TRACE_ERR("PAMI_Endpoint_create failed\n");

   xferP.dest = dest;

   rc = PAMI_Send_immediate(MPIDI_Context[0], &xferP);
}

void MPIDI_Recvfrom_remote_world_disconnect(pami_context_t    context,
                void            * cookie,
                const void      * _msginfo,
                size_t            msginfo_size,
                const void      * sndbuf,
                size_t            sndlen,
                pami_endpoint_t   sender,
                pami_recv_t     * recv)
{
  AM_struct2        *AM_data;
  long long        tranid;
  int              whichAM;

  AM_data  = ((AM_struct2 *)_msginfo);
  tranid   = AM_data->tranid;
  whichAM  = AM_data->whichAM;
  MPIDI_increment_AM_cntr_for_tranid(tranid, whichAM);

  TRACE_ERR("MPIDI_Recvfrom_remote_world_disconnect: invoked for tranid = %lld, whichAM = %d \n",tranid, whichAM);

  return;
}


/**
 * Function to retreive the active message counter for a particular trasaction id.
 * This function is used inside disconnect routine.
 * whichAM = FIRST_AM/SECOND_AM/LAST_AM
 */
int MPIDI_get_AM_cntr_for_tranid(long long tranid, int whichAM)
{
  transactionID_struct *tridtmp;

  if(_transactionID_list == NULL)
    TRACE_ERR("MPIDI_get_AM_cntr_for_tranid - _transactionID_list is NULL\n");

  tridtmp = _transactionID_list;

  while(tridtmp != NULL) {
    if(tridtmp->tranid == tranid) {
      return tridtmp->cntr_for_AM[whichAM];
    }
    tridtmp = tridtmp->next;
  }

  return -1;
}


/**
 * Function used to gurantee the delivery of LAPI active message at the
 * destination. Called at the destination taskid and returns only whe
 * the expected LAPI active message is recived. If the sequence number
 * of the LAPI active message is LAST_AM, the other condition under which
 * this function may exit is if DISCONNECT_LAPI_XFER_TIMEOUT happens.
 */
void MPIDI_wait_for_AM(long long tranid, int expected_AM, int whichAM)
{
  double starttime, currtime, elapsetime;
  int    rc, curr_AMcntr;

  rc = PAMI_Context_advance(MPIDI_Context[0], (size_t)100);
  if(whichAM == LAST_AM) {
    CURTIME(starttime)
    do {
      CURTIME(currtime)
      elapsetime = currtime - starttime;

      rc = PAMI_Context_advance(MPIDI_Context[0], (size_t)100);
      curr_AMcntr = MPIDI_get_AM_cntr_for_tranid(tranid, whichAM);
      /*TRACE_ERR("_try_to_disconnect: Looping in timer for TranID %lld, whichAM %d expected_AM = %d, Current AM = %d\n",tranid,whichAM,expected_AM,curr_AMcntr); */
    }while(curr_AMcntr != expected_AM && elapsetime < DISCONNECT_LAPI_XFER_TIMEOUT);
  }
  else {
    do {
      rc = PAMI_Context_advance(MPIDI_Context[0], (size_t)100);
      curr_AMcntr = MPIDI_get_AM_cntr_for_tranid(tranid, whichAM);
      /*TRACE_ERR("_try_to_disconnect: Looping in timer for TranID %lld, whichAM %d expected_AM = %d, Current AM = %d\n",tranid,whichAM,expected_AM,curr_AMcntr);*/
    }while(curr_AMcntr != expected_AM);
  }
}

/* function to swap two integers. Used inside function _qsort_dyntask below */
static void _swap_dyntask(int t[],int i,int j)
{
   int  tmp;

   tmp = t[i];
   t[i] = t[j];
   t[j] = tmp;
}

/* qsort sorting function which is used only in this file */
static void _qsort_dyntask(int t[],int left,int right)
{
   int i,last;

   if(left >= right)  return;
   last = left;
   for(i=left+1;i<=right;i++)
      if(t[i] < t[left])
         _swap_dyntask(t,++last,i);
   _swap_dyntask(t,left,last);
   _qsort_dyntask(t,left,last-1);
   _qsort_dyntask(t,last+1,right);
}



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
    int rc, i,j, k, ref_count,mpi_errno=0, probe_flag=0;
    pami_task_t *local_list;
    MPI_Status status;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIDI_PG_t *pg;
    int total_leaders=0, gsize;
    pami_task_t *leader_tids;
    int expected_firstAM=0, expected_secondAM=0, expected_lastAM=0;
    MPID_Comm *commworld_ptr;
    MPID_Group *group_ptr = NULL,  *new_group_ptr = NULL;
    MPID_VCR *glist;
    MPID_Comm *lcomm;
    int *ranks;
    int local_tasks=0, localtasks_in_remglist=0;
    int jobIdSize=64;
    char jobId[jobIdSize];
    int MY_TASKID = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_TASK_ID  ).value.intval;

    /*if( (comm_ptr->comm_kind == MPID_INTERCOMM) && (comm_ptr->mpid.world_ids != NULL)) { */
    if(comm_ptr->mpid.world_ids != NULL) {
	rc = MPID_Iprobe(comm_ptr->rank, MPI_ANY_TAG, comm_ptr, MPID_CONTEXT_INTER_PT2PT, &probe_flag, &status);
        if(rc || probe_flag) {
          TRACE_ERR("PENDING_PTP");
	  exit(1);
        }

	/* make commSubWorld */
	{
	  /*           MPID_Comm_get_ptr( MPI_COMM_WORLD, commworld_ptr ); */
	  commworld_ptr = MPIR_Process.comm_world;
	  mpi_errno = MPIR_Comm_group_impl(commworld_ptr, &group_ptr);
	  if (mpi_errno)
	    {
	      TRACE_ERR("Error while creating group_ptr from MPI_COMM_WORLD in MPIDI_Comm_create_from_pami_geom\n");
	      return PAMI_ERROR;
	    }

	  
	  glist = commworld_ptr->vcr;
	  gsize = commworld_ptr->local_size;
	  for(i=0;i<comm_ptr->local_size;i++) {
	    for(j=0;j<gsize;j++) {
	      if(comm_ptr->local_vcr[i]->taskid == glist[j]->taskid)
		local_tasks++;
	    }
	  }

	  /**
	   * Tasks belonging to the same local world may also be part of
	   * the GROUPREMLIST, so these tasks will have to be use in addition
	   * to the tasks in GROUPLIST to construct lcomm
	   **/
	  if(comm_ptr->comm_kind == MPID_INTERCOMM) {
	    for(i=0;i<comm_ptr->remote_size;i++) {
	      for(j=0;j<gsize;j++) {
		if(comm_ptr->vcr[i]->taskid == glist[j]->taskid) {
		  local_tasks++;
		  localtasks_in_remglist++;
		}
	      }
	    }
	  }
	  k=0;
	  /*	  local_list = MPIU_Malloc(local_tasks*sizeof(pami_task_t)); */
	  ranks = MPIU_Malloc(local_tasks*sizeof(int));

	  for(i=0;i<comm_ptr->local_size;i++) {
	    for(j=0;j<gsize;j++) {
	      if(comm_ptr->local_vcr[i]->taskid == glist[j]->taskid)
		/*	local_list[k] = glist[j]->taskid; */
		ranks[k++] = j;
	    }
	  }
	  if((comm_ptr->comm_kind == MPID_INTERCOMM) && localtasks_in_remglist) {
	    for(i=0;i<comm_ptr->remote_size;i++) {
	      for(j=0;j<gsize;j++) {
		if(comm_ptr->vcr[i]->taskid == glist[j]->taskid)
		  /*	  local_list[k] = glist[j]->taskid; */
		  ranks[k++] = j;
	      }
	    }
	    /* Sort the local_list when there are localtasks_in_remglist */
/*	    _qsort_dyntask(local_list, 0, local_tasks-1); */
	    _qsort_dyntask(ranks, 0, local_tasks-1);
	  }

	  /* Now we have all we need to create the new group. Create it */
	  /*  mpi_errno = MPIR_Group_incl_impl(group_ptr, local_tasks, ranks, &new_group_ptr); */
	  mpi_errno = MPIR_Group_incl_impl(group_ptr, local_tasks, ranks, &new_group_ptr);
	  if (mpi_errno)
	    {
	      TRACE_ERR("Error while creating new_group_ptr from group_ptr in MPIDI_Comm_create_from_pami_geom\n");
	      return PAMI_ERROR;
	    }
	  
	  /* Now create the communicator using the new_group_ptr */
	  mpi_errno = MPIR_Comm_create_group(commworld_ptr, new_group_ptr, 0, &lcomm);
	  /*  mpi_errno = MPIR_Comm_create_intra(commworld_ptr, new_group_ptr, &lcomm); */
	  if (mpi_errno)
	    {
	      TRACE_ERR("Error while creating new_comm_ptr from group_ptr in MPIDI_Comm_create_from_pami_geom\n");
	      return PAMI_ERROR;
	    }

#if 0
	  mpi_errno = MPIR_Comm_create(&lcomm);
	  if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIR_Comm_create returned with mpi_errno=%d\n", mpi_errno);
	  }

	  /* fill in all the fields of lcomm. */
	  if(localtasks_in_remglist==0) {
	    lcomm->context_id     = MPID_CONTEXT_SET_FIELD(DYNAMIC_PROC, comm_ptr->recvcontext_id, 1);
	    lcomm->recvcontext_id = lcomm->context_id;
	  } else {
	    lcomm->context_id     = MPID_CONTEXT_SET_FIELD(DYNAMIC_PROC, comm_ptr->recvcontext_id, 1);
	    lcomm->recvcontext_id = MPID_CONTEXT_SET_FIELD(DYNAMIC_PROC, comm_ptr->context_id, 1);
	  }
	  TRACE_ERR("lcomm->context_id =%d\n", lcomm->context_id);

	  /* sanity: the INVALID context ID value could potentially conflict with the
	   * dynamic proccess space */
	  MPIU_Assert(lcomm->context_id     != MPIU_INVALID_CONTEXT_ID);
	  MPIU_Assert(lcomm->recvcontext_id != MPIU_INVALID_CONTEXT_ID);

	  /* FIXME - we probably need a unique context_id. */

	  /* Fill in new intercomm */
	  lcomm->comm_kind    = MPID_INTRACOMM;
	  lcomm->remote_size = lcomm->local_size = local_tasks;

	  /* Set up VC reference table */
	  mpi_errno = MPID_VCRT_Create(lcomm->remote_size, &lcomm->vcrt);
	  if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPID_VCRT_Create returned with mpi_errno=%d", mpi_errno);
	  }
	  MPID_VCRT_Add_ref(lcomm->vcrt);
	  mpi_errno = MPID_VCRT_Get_ptr(lcomm->vcrt, &lcomm->vcr);
	  if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPID_VCRT_Get_ptr returned with mpi_errno=%d", mpi_errno);
	  }

	  for(i=0; i<local_tasks; i++) {
	    if(MY_TASKID == local_list[i]) lcomm->rank = i;
	    lcomm->vcr[i]->taskid = local_list[i];
	  }
#endif

	}

	TRACE_ERR("subcomm for disconnect is established local_tasks=%d calling MPIR_Barrier_intra\n", local_tasks);
	mpi_errno = MPIR_Barrier_intra(lcomm, &errflag);
	if (mpi_errno != MPI_SUCCESS) {
	  TRACE_ERR("MPIR_Barrier_intra returned with mpi_errno=%d\n", mpi_errno);
	}
	TRACE_ERR("after barrier in disconnect\n");

	if(MY_TASKID != comm_ptr->mpid.local_leader) {
	  for(i=0; comm_ptr->mpid.world_ids[i] != -1; i++) {
	    ref_count = MPIDI_Decrement_ref_count(comm_ptr->mpid.world_ids[i]);
	    TRACE_ERR("ref_count=%d with world=%d comm_ptr=%x\n", ref_count, comm_ptr->mpid.world_ids[i], comm_ptr);
	    if(ref_count == -1)
	      TRACE_ERR("something is wrong\n");
	  }
	}

	if(MY_TASKID == comm_ptr->mpid.local_leader) {
	  PMI2_Job_GetId(jobId, jobIdSize);
	  for(i=0;comm_ptr->mpid.world_ids[i]!=-1;i++)  {
	    if(atoi(jobId) != comm_ptr->mpid.world_ids[i])
	      total_leaders++;
	  }
	  TRACE_ERR("total_leaders=%d\n", total_leaders);
	  leader_tids = MPIU_Malloc(total_leaders*sizeof(int));
	  MPIDI_get_allremote_leaders(leader_tids, comm_ptr);
	  
	  { /* First Pair of Send / Recv -- All smaller task send to all larger tasks */
	    for(i=0;i<total_leaders;i++) {
	      MPID_assert(leader_tids[i] != -1);
	      if(MY_TASKID < leader_tids[i]) {
		TRACE_ERR("_try_to_disconnect: FIRST: comm_ptr->mpid.world_intercomm_cntr %lld, toTaskid %d\n",comm_ptr->mpid.world_intercomm_cntr,leader_tids[i]);
		expected_firstAM++;
		MPIDI_send_AM_to_remote_leader_on_disconnect(leader_tids[i], comm_ptr->mpid.world_intercomm_cntr, FIRST_AM);
	      }
	      else {
		expected_secondAM++;
	      }
	    }
	    if(expected_secondAM) {
	      MPIDI_wait_for_AM(comm_ptr->mpid.world_intercomm_cntr, expected_secondAM, FIRST_AM);
	    }
	  }
	  
	  { /* Second Pair of Send / Recv -- All larger tasks send to all smaller tasks */
	    for(i=0;i<total_leaders;i++) {
	      MPID_assert(leader_tids[i] != -1);
	      if(MY_TASKID > leader_tids[i]) {
		TRACE_ERR("_try_to_disconnect: SECOND: comm_ptr->mpid.world_intercomm_cntr %lld, toTaskid %d\n",comm_ptr->mpid.world_intercomm_cntr,leader_tids[i]);
		MPIDI_send_AM_to_remote_leader_on_disconnect(leader_tids[i], comm_ptr->mpid.world_intercomm_cntr, SECOND_AM);
	      }
	    }
	    if(expected_firstAM) {
	      MPIDI_wait_for_AM(comm_ptr->mpid.world_intercomm_cntr, expected_firstAM, SECOND_AM);
	    }
	  }

	  for(i=0; comm_ptr->mpid.world_ids[i] != -1; i++) {
	    ref_count = MPIDI_Decrement_ref_count(comm_ptr->mpid.world_ids[i]);
	    TRACE_ERR("ref_count=%d with world=%d comm_ptr=%x\n", ref_count, comm_ptr->mpid.world_ids[i], comm_ptr);
	    if(ref_count == -1)
	      TRACE_ERR("something is wrong\n");
	  }

	  for(i=0;i<total_leaders;i++) {
	    MPID_assert(leader_tids[i] != -1);
	    if(MY_TASKID < leader_tids[i]) {
	      TRACE_ERR("_try_to_disconnect: LAST: comm_ptr->mpid.world_intercomm_cntr %lld, toTaskid %d\n",comm_ptr->mpid.world_intercomm_cntr,leader_tids[i]);
	      MPIDI_send_AM_to_remote_leader_on_disconnect(leader_tids[i], comm_ptr->mpid.world_intercomm_cntr, LAST_AM);
	    }
	    else {
	      expected_lastAM++;
	    }
	  }
	  if(expected_lastAM) {
	    MPIDI_wait_for_AM(comm_ptr->mpid.world_intercomm_cntr, expected_lastAM,
			      LAST_AM);
	  }
	  MPIU_Free(leader_tids);
	}

	TRACE_ERR("_try_to_disconnect: Going inside final barrier for tranid %lld\n",comm_ptr->mpid.world_intercomm_cntr);
	mpi_errno = MPIR_Barrier_intra(lcomm, &errflag);
	if (mpi_errno != MPI_SUCCESS) {
	  TRACE_ERR("MPIR_Barrier_intra returned with mpi_errno=%d\n", mpi_errno);
	}
        mpi_errno = MPIR_Comm_release(lcomm,0);
        if (mpi_errno) TRACE_ERR("MPIR_Comm_release returned with mpi_errno=%d\n", mpi_errno);

	MPIDI_free_tranid_node(comm_ptr->mpid.world_intercomm_cntr);
        mpi_errno = MPIR_Comm_release(comm_ptr,1);
        if (mpi_errno) TRACE_ERR("MPIR_Comm_release returned with mpi_errno=%d\n", mpi_errno);
	/*	MPIU_Free(local_list); */
	MPIU_Free(ranks);
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

void MPIDI_get_allremote_leaders(int *tid_arr, MPID_Comm *comm_ptr)
{
  conn_info  *tmp_node;
  int        i,j,k,arr_len,gsize, found=0;
  int        leader1=-1, leader2=-1;
  MPID_VCR   *glist;
  
  for(i=0;comm_ptr->mpid.world_ids[i] != -1;i++)
  {
    TRACE_ERR("i=%d comm_ptr->mpid.world_ids[i]=%d\n", i, comm_ptr->mpid.world_ids[i]);
    tmp_node = _conn_info_list;
    found=0;
    if(tmp_node==NULL) {TRACE_ERR("_conn_info_list is NULL\n");}
    while(tmp_node != NULL) {
      if(tmp_node->rem_world_id == comm_ptr->mpid.world_ids[i]) {
        if(comm_ptr->comm_kind == MPID_INTRACOMM) {
          glist = comm_ptr->local_vcr;
          gsize = comm_ptr->local_size;
        }
        else {
          glist = comm_ptr->vcr;
          gsize = comm_ptr->remote_size;
        }
        for(j=0;j<gsize;j++) {
          for(k=0;tmp_node->rem_taskids[k]!=-1;k++) {
            TRACE_ERR("j=%d k=%d glist[j]->taskid=%d tmp_node->rem_taskids[k]=%d\n", j, k,glist[j]->taskid, tmp_node->rem_taskids[k]);
            if(glist[j]->taskid == tmp_node->rem_taskids[k]) {
              leader1 = glist[j]->taskid;
              found = 1;
              break;
            }
          }
          if(found) break;
        }
        /*
	 * There may be the case where my local_comm's GROUPLIST contains tasks
	 * fro remote world-x and GROUPREMLIST contains other remaining tasks of world-x
	 * If the smallest task of world-x is in my GROUPLIST then the above iteration
	 * will give the leader as smallest task from world-x in my GROUPREMLIST.
	 * But this will not be the correct leader_taskid. I should find the smallest task
	 * of world-x in my GROUPLIST and then see which of the two leaders is the
	 * smallest one. The smallest one is the one in which I am interested.
	 **/
        if(comm_ptr->comm_kind == MPID_INTERCOMM) {
          found=0;
          glist = comm_ptr->local_vcr;
          gsize = comm_ptr->local_size;
          for(j=0;j<gsize;j++) {
            for(k=0;tmp_node->rem_taskids[k]!=-1;k++) {
            TRACE_ERR("j=%d k=%d glist[j]->taskid=%d tmp_node->rem_taskids[k]=%d\n", j, k, glist[j]->taskid, tmp_node->rem_taskids[k]);
              if(glist[j]->taskid == tmp_node->rem_taskids[k]) {
                leader2 = glist[j]->taskid;
                found = 1;
                break;
              }
            }
            if(found) break;
          }
        }
        if(found) {
          break;
        }
      } else  {TRACE_ERR("world id is different tmp_node->rem_world_id =%d comm_ptr=%x comm_ptr->mpid.world_ids[i]=%d\n", tmp_node->rem_world_id, comm_ptr, comm_ptr->mpid.world_ids[i]);}
      tmp_node = tmp_node->next;
    }

    TRACE_ERR("comm_ptr=%x leader1=%d leader2=%d\n", comm_ptr, leader1, leader2);
    if(leader1 == -1)
      *(tid_arr+i) = leader2;
    else if(leader2 == -1)
      *(tid_arr+i) = leader1;
    else
      *(tid_arr+i) = leader1 < leader2 ? leader1 : leader2;
  }
}

#endif
