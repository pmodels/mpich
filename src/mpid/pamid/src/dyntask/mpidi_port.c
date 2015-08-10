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

#include <mpidimpl.h>

#ifdef DYNAMIC_TASKING
#define MAX_HOST_DESCRIPTION_LEN 256
#define WORLDINTCOMMCNTR _global_world_intercomm_cntr
#ifdef USE_PMI2_API
#define MPID_MAX_JOBID_LEN 256
#define TOTAL_AM 3
#endif

transactionID_struct *_transactionID_list = NULL;

typedef struct {
  MPID_VCR vcr;
  int        port_name_tag;
}AM_struct;

conn_info  *_conn_info_list = NULL;
extern int mpidi_dynamic_tasking;
long long _global_world_intercomm_cntr = 0;

typedef struct MPIDI_Acceptq
{
    int             port_name_tag;
    MPID_VCR 	    vcr;
    struct MPIDI_Acceptq *next;
}
MPIDI_Acceptq_t;

static MPIDI_Acceptq_t * acceptq_head=0;
static int maxAcceptQueueSize = 0;
static int AcceptQueueSize    = 0;

pthread_mutex_t rem_connlist_mutex = PTHREAD_MUTEX_INITIALIZER;

/* FIXME: If dynamic processes are not supported, this file will contain
   no code and some compilers may warn about an "empty translation unit" */

/* FIXME: pg_translation is used for ? */
typedef struct pg_translation {
    int pg_index;    /* index of a process group (index in pg_node) */
    int pg_rank;     /* rank in that process group */
    pami_task_t pg_taskid;     /* rank in that process group */
} pg_translation;


typedef struct pg_node {
    int  index;            /* Internal index of process group
			      (see pg_translation) */
    char *pg_id;
    char *str;             /* String describing connection info for pg */
    int   lenStr;          /* Length of this string (including the null terminator(s)) */
    struct pg_node *next;
} pg_node;


void MPIDI_Recvfrom_remote_world(pami_context_t    context,
                void            * cookie,
                const void      * _msginfo,
                size_t            msginfo_size,
                const void      * sndbuf,
                size_t            sndlen,
                pami_endpoint_t   sender,
                pami_recv_t     * recv)
{
  AM_struct        *AM_data;
  MPID_VCR       *new_vcr;
  int              port_name_tag;
  MPIDI_Acceptq_t *q_item;
  pami_endpoint_t dest;


  q_item = MPIU_Malloc(sizeof(MPIDI_Acceptq_t));
  q_item->vcr = MPIU_Malloc(sizeof(struct MPID_VCR_t));
  q_item->vcr->pg = MPIU_Malloc(sizeof(MPIDI_PG_t));
  MPIU_Object_set_ref(q_item->vcr->pg, 0);
  TRACE_ERR("ENTER MPIDI_Acceptq_enqueue-1 q_item=%llx _msginfo=%llx (AM_struct *)_msginfo=%llx ((AM_struct *)_msginfo)->vcr=%llx\n", q_item, _msginfo, (AM_struct *)_msginfo, ((AM_struct *)_msginfo)->vcr);
  q_item->port_name_tag = ((AM_struct *)_msginfo)->port_name_tag;
  q_item->vcr->taskid = PAMIX_Endpoint_query(sender);
  TRACE_ERR("MPIDI_Recvfrom_remote_world INVOKED with new_vcr->taskid=%d\n",sender);

  /* Keep some statistics on the accept queue */
  AcceptQueueSize++;
  if (AcceptQueueSize > maxAcceptQueueSize)
    maxAcceptQueueSize = AcceptQueueSize;

  q_item->next = acceptq_head;
  acceptq_head = q_item;
  return;
}


/* These functions help implement the connect/accept algorithm */
static int MPIDI_ExtractLocalPGInfo( struct MPID_Comm *, pg_translation [],
			       pg_node **, int * );
static int MPIDI_ReceivePGAndDistribute( struct MPID_Comm *, struct MPID_Comm *, int, int *,
				   int, MPIDI_PG_t *[] );
static int MPIDI_SendPGtoPeerAndFree( struct MPID_Comm *, int *, pg_node * );
static int MPIDI_SetupNewIntercomm( struct MPID_Comm *comm_ptr, int remote_comm_size,
			      pg_translation remote_translation[],
			      int n_remote_pgs, MPIDI_PG_t **remote_pg,
			      struct MPID_Comm *intercomm );
static int MPIDI_Initialize_tmp_comm(struct MPID_Comm **comm_pptr,
					  struct MPID_VCR_t *vcr_ptr, int is_low_group, int context_id_offset);


/* ------------------------------------------------------------------------- */
/*
 * Structure of this file and the connect/accept algorithm:
 *
 * Here are the steps involved in implementating MPI_Comm_connect and
 * MPI_Comm_accept.  These same steps are used withing MPI_Comm_spawn
 * and MPI_Comm_spawn_multiple.
 *
 * First, the connecting process establishes a connection (not a virtual
 * connection!) to the designated accepting process.
 * This makes use of the usual (channel-specific) connection code.
 * Once this connection is established, the connecting process sends a packet
 * to the accepting process.
 * This packet contains a "port_tag_name", which is a value that
 * is used to separate different MPI port names (values from MPI_Open_port)
 * on the same process (this is a way to multiplex many MPI port names on
 * a single communication connection port).
 *
 * On the accepting side, the process waits until the progress engine
 * inserts the connect request into the accept queue (this is done with the
 * routine MPIDI_Acceptq_dequeue).  This routine returns the matched
 * virtual connection (VC).
 *
 * Once both sides have established there VC, they both invoke
 * MPIDI_Initialize_tmp_comm to create a temporary intercommunicator.
 * A temporary intercommunicator is constructed so that we can use
 * MPI routines to send the other information that we need to complete
 * the connect/accept operation (described below).
 *
 * The above is implemented with the routines
 *   MPIDI_Create_inter_root_communicator_connect
 *   MPIDI_Create_inter_root_communicator_accept
 *   MPIDI_Initialize_tmp_comm
 *
 * At this point, the two "root" processes of the communicators that are
 * connecting can use MPI communication.  They must then exchange the
 * following information:
 *
 *    The size of the "remote" communicator
 *    Description of all process groups; that is, all of the MPI_COMM_WORLDs
 *    that they know.
 *    The shared context id that will be used
 *
 *
 */
/* ------------------------------------------------------------------------- */

int MPIDU_send_AM_to_leader(MPID_VCR new_vcr, int port_name_tag, pami_task_t taskid)
{
   pami_send_t xferP;
   pami_endpoint_t dest;
   int              rc, current_val;

   AM_struct        AM_data;

   AM_data.vcr = new_vcr;
   TRACE_ERR("MPIDU_send_AM_to_leader - new_vcr->taskid=%d\n", new_vcr->taskid);
   AM_data.port_name_tag = port_name_tag;
   TRACE_ERR("send - %p %d %p %d\n", AM_data.vcr, AM_data.port_name_tag, AM_data.vcr, AM_data.vcr->taskid);


   memset(&xferP, 0, sizeof(pami_send_t));
   xferP.send.header.iov_base = (void*)&AM_data;
   xferP.send.header.iov_len  = sizeof(AM_struct);
   xferP.send.dispatch = MPIDI_Protocols_Dyntask;
   /*xferP.hints.use_rdma  = mpci_enviro.use_shmem;
   xferP.hints.use_shmem = mpci_enviro.use_shmem;*/
   rc = PAMI_Endpoint_create(MPIDI_Client, taskid, 0, &dest);
   TRACE_ERR("PAMI_Resume to taskid=%d\n", taskid);
	PAMI_Resume(MPIDI_Context[0],
                    &dest, 1);

   if(rc != 0)
     TRACE_ERR("PAMI_Endpoint_create failed\n");

   xferP.send.dest = dest;

   rc = PAMI_Send(MPIDI_Context[0], &xferP);

}


/*
 * These next two routines are used to create a virtual connection
 * (VC) and a temporary intercommunicator that can be used to
 * communicate between the two "root" processes for the
 * connect and accept.
 */

/* FIXME: Describe the algorithm for the connection logic */
int MPIDI_Connect_to_root(const char * port_name,
                          MPID_VCR * new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_VCR vc;
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port, port_name_tag; pami_task_t taskid_tag;
    int hasIfaddr = 0;
    AM_struct *conn;

    /* First, create a new vc (we may use this to pass to a generic
       connection routine) */
    vc = MPIU_Malloc(sizeof(struct MPID_VCR_t));
    vc->pg = MPIU_Malloc(sizeof(MPIDI_PG_t));
    MPIU_Object_set_ref(vc->pg, 0);
    TRACE_ERR("vc from MPIDI_Connect_to_root=%llx vc->pg=%llx\n", vc, vc->pg);
    /* FIXME - where does this vc get freed? */

    *new_vc = vc;

    /* FIXME: There may need to be an additional routine here, to ensure that the
       channel is initialized for this pair of process groups (this process
       and the remote process to which the vc will connect). */
/*    MPIDI_VC_Init(vc->vc, NULL, 0); */
    vc->taskid = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_TASK_ID  ).value.intval;
    TRACE_ERR("MPIDI_Connect_to_root - vc->taskid=%d\n", vc->taskid);

    mpi_errno = MPIDI_GetTagFromPort(port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
      TRACE_ERR("MPIDI_GetTagFromPort returned with mpi_errno=%d", mpi_errno);
    }
    mpi_errno = MPIDI_GetTaskidFromPort(port_name, &taskid_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
      TRACE_ERR("MPIDI_GetTaskidFromPort returned with mpi_errno=%d", mpi_errno);
    }

    TRACE_ERR("posting connect to host %s, port %d task %d vc %p\n",
	host_description, port, taskid_tag, vc );
    mpi_errno = MPIDU_send_AM_to_leader(vc, port_name_tag, taskid_tag);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* ------------------------------------------------------------------------- */
/* Business card management.  These routines insert or extract connection
   information when using sockets from the business card */
/* ------------------------------------------------------------------------- */

/* FIXME: These are small routines; we may want to bring them together
   into a more specific post-connection-for-sock */

/* The host_description should be of length MAX_HOST_DESCRIPTION_LEN */


static int MPIDI_Create_inter_root_communicator_connect(const char *port_name,
							struct MPID_Comm **comm_pptr,
							MPID_VCR *vc_pptr)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPID_Comm *tmp_comm;
    struct MPID_VCR_t *connect_vc= NULL;
    int port_name_tag, taskid_tag;
    /* Connect to the root on the other side. Create a
       temporary intercommunicator between the two roots so that
       we can use MPI functions to communicate data between them. */

    MPIDI_Connect_to_root(port_name, &(connect_vc));
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIDI_Connect_to_root returned with mpi_errno=%d\n", mpi_errno);
    }

    /* extract the tag from the port_name */
    mpi_errno = MPIDI_GetTagFromPort( port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	TRACE_ERR("MPIDI_GetTagFromPort returned with mpi_errno=%d\n", mpi_errno);
    }

    mpi_errno = MPIDI_GetTaskidFromPort(port_name, &taskid_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	TRACE_ERR("MPIDI_GetTaskidFromPort returned with mpi_errno=%d\n", mpi_errno);
    }
    connect_vc->taskid=taskid_tag;
    mpi_errno = MPIDI_Initialize_tmp_comm(&tmp_comm, connect_vc, 1, port_name_tag);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIDI_Initialize_tmp_comm returned with mpi_errno=%d\n", mpi_errno);
    }

    *comm_pptr = tmp_comm;
    *vc_pptr = connect_vc;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Creates a communicator for the purpose of communicating with one other
   process (the root of the other group).  It also returns the virtual
   connection */
static int MPIDI_Create_inter_root_communicator_accept(const char *port_name,
						struct MPID_Comm **comm_pptr,
						MPID_VCR *vc_pptr)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPID_Comm *tmp_comm;
    MPID_VCR new_vc;

    MPID_Progress_state progress_state;
    int port_name_tag;

    /* extract the tag from the port_name */
    mpi_errno = MPIDI_GetTagFromPort( port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	TRACE_ERR("MPIDI_GetTagFromPort returned with mpi_errnp=%d\n", mpi_errno);
    }

    /* FIXME: Describe the algorithm used here, and what routine
       is user on the other side of this connection */
    /* dequeue the accept queue to see if a connection with the
       root on the connect side has been formed in the progress
       engine (the connection is returned in the form of a vc). If
       not, poke the progress engine. */

    for(;;)
    {
	MPIDI_Acceptq_dequeue(&new_vc, port_name_tag);
	if (new_vc != NULL)
	{
	    break;
	}

	mpi_errno = MPID_Progress_wait(100);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno)
	{
	    TRACE_ERR("MPID_Progress_wait returned with mpi_errno=%d\n", mpi_errno);
	}
	/* --END ERROR HANDLING-- */
    }

    mpi_errno = MPIDI_Initialize_tmp_comm(&tmp_comm, new_vc, 0, port_name_tag);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIDI_Initialize_tmp_comm returned with mpi_errno=%d\n", mpi_errno);
    }

    *comm_pptr = tmp_comm;
    *vc_pptr = new_vc;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* This is a utility routine used to initialize temporary communicators
   used in connect/accept operations, and is only used in the above two
   routines */
static int MPIDI_Initialize_tmp_comm(struct MPID_Comm **comm_pptr,
					  struct MPID_VCR_t *vc_ptr, int is_low_group, int context_id_offset)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPID_Comm *tmp_comm, *commself_ptr;

    MPID_Comm_get_ptr( MPI_COMM_SELF, commself_ptr );

    /* WDG-old code allocated a context id that was then discarded */
    mpi_errno = MPIR_Comm_create(&tmp_comm);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIR_Comm_create returned with mpi_errno=%d\n", mpi_errno);
    }
    /* fill in all the fields of tmp_comm. */

    /* We use the second half of the context ID bits for dynamic
     * processes. This assumes that the context ID mask array is made
     * up of uint32_t's. */
    /* FIXME: This code is still broken for the following case:
     * If the same process opens connections to the multiple
     * processes, this context ID might get out of sync.
     */
    tmp_comm->context_id     = MPID_CONTEXT_SET_FIELD(DYNAMIC_PROC, context_id_offset, 1);
    tmp_comm->recvcontext_id = tmp_comm->context_id;

    /* sanity: the INVALID context ID value could potentially conflict with the
     * dynamic proccess space */
    MPIU_Assert(tmp_comm->context_id     != MPIU_INVALID_CONTEXT_ID);
    MPIU_Assert(tmp_comm->recvcontext_id != MPIU_INVALID_CONTEXT_ID);

    /* FIXME - we probably need a unique context_id. */
    tmp_comm->remote_size = 1;

    /* Fill in new intercomm */
    tmp_comm->local_size   = 1;
    tmp_comm->rank         = 0;
    tmp_comm->comm_kind    = MPID_INTERCOMM;
    tmp_comm->local_comm   = NULL;
    tmp_comm->is_low_group = is_low_group;

    /* No pg structure needed since vc has already been set up
       (connection has been established). */

    /* Point local vcr, vcrt at those of commself_ptr */
    /* FIXME: Explain why */
    tmp_comm->local_vcrt = commself_ptr->vcrt;
    MPID_VCRT_Add_ref(commself_ptr->vcrt);
    tmp_comm->local_vcr  = commself_ptr->vcr;

    /* No pg needed since connection has already been formed.
       FIXME - ensure that the comm_release code does not try to
       free an unallocated pg */

    /* Set up VC reference table */
    mpi_errno = MPID_VCRT_Create(tmp_comm->remote_size, &tmp_comm->vcrt);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPID_VCRT_Create returned with mpi_errno=%d", mpi_errno);
    }
    mpi_errno = MPID_VCRT_Get_ptr(tmp_comm->vcrt, &tmp_comm->vcr);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPID_VCRT_Get_ptr returned with mpi_errno=%d", mpi_errno);
    }

    /* FIXME: Why do we do a dup here? */
    MPID_VCR_Dup(vc_ptr, tmp_comm->vcr);

    *comm_pptr = tmp_comm;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


/**
 *  * Function to add a new trasaction id in the transaction id list. This function
 *   * gets called only when a new connection is made with remote tasks.
 *    */
void MPIDI_add_new_tranid(long long tranid)
{
  int i;
  transactionID_struct *tridtmp=NULL;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  if(_transactionID_list == NULL) {
    _transactionID_list = (transactionID_struct*) MPIU_Malloc(sizeof(transactionID_struct));
    _transactionID_list->cntr_for_AM = MPIU_Malloc(TOTAL_AM*sizeof(int));
    _transactionID_list->tranid = tranid;
    for(i=0;i<TOTAL_AM;i++)
      _transactionID_list->cntr_for_AM[i] = 0;
    _transactionID_list->next     = NULL;
    MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
    return;
  }

  tridtmp = _transactionID_list;
  while(tridtmp->next != NULL)
    tridtmp = tridtmp->next;

  tridtmp->next = (transactionID_struct*) MPIU_Malloc(sizeof(transactionID_struct));
  tridtmp = tridtmp->next;
  tridtmp->tranid  = tranid;
  tridtmp->cntr_for_AM = MPIU_Malloc(TOTAL_AM*sizeof(int));
  for(i=0;i<TOTAL_AM;i++)
    tridtmp->cntr_for_AM[i] = 0;
  tridtmp->next    = NULL;
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
}


/* ------------------------------------------------------------------------- */
/*
   MPIDI_Comm_connect()

   Algorithm: First create a connection (vc) between this root and the
   root on the accept side. Using this vc, create a temporary
   intercomm between the two roots. Use MPI functions to communicate
   the other information needed to create the real intercommunicator
   between the processes on the two sides. Then free the
   intercommunicator between the roots. Most of the complexity is
   because there can be multiple process groups on each side.
*/
int MPIDI_Comm_connect(const char *port_name, MPID_Info *info, int root,
		       struct MPID_Comm *comm_ptr, struct MPID_Comm **newcomm)
{
    int mpi_errno=MPI_SUCCESS;
    int j, i, rank, recv_ints[3], send_ints[3], context_id;
    int remote_comm_size=0;
    struct MPID_Comm *tmp_comm = NULL;
    MPID_VCR new_vc= NULL;
    int sendtag=100, recvtag=100, n_remote_pgs;
    int n_local_pgs=1, local_comm_size;
    pg_translation *local_translation = NULL, *remote_translation = NULL;
    pg_node *pg_list = NULL;
    MPIDI_PG_t **remote_pg = NULL;
    MPIU_Context_id_t recvcontext_id = MPIU_INVALID_CONTEXT_ID;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    long long comm_cntr, lcomm_cntr;

    /* Get the context ID here because we need to send it to the remote side */
    mpi_errno = MPIR_Get_contextid( comm_ptr, &recvcontext_id );
    TRACE_ERR("MPIDI_Comm_connect calling MPIR_Get_contextid = %d\n", recvcontext_id);
    if (mpi_errno) TRACE_ERR("MPIR_Get_contextid returned with mpi_errno=%d\n", mpi_errno);

    rank = comm_ptr->rank;
    local_comm_size = comm_ptr->local_size;
    TRACE_ERR("In MPIDI_Comm_connect - port_name=%s rank=%d root=%d\n", port_name, rank, root);

    WORLDINTCOMMCNTR += 1;
    comm_cntr = WORLDINTCOMMCNTR;
    lcomm_cntr = WORLDINTCOMMCNTR;

    if (rank == root)
    {
	/* Establish a communicator to communicate with the root on the
	   other side. */
	mpi_errno = MPIDI_Create_inter_root_communicator_connect(
	    port_name, &tmp_comm, &new_vc);
	if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIDI_Create_inter_root_communicator_connect returned mpi_errno=%d\n", mpi_errno);
	}
	TRACE_ERR("after MPIDI_Create_inter_root_communicator_connect - tmp_comm=%p  new_vc=%p mpi_errno=%d\n", tmp_comm, new_vc, mpi_errno);

	/* Make an array to translate local ranks to process group index
	   and rank */
	local_translation = MPIU_Malloc(local_comm_size*sizeof(pg_translation));
/*	MPIU_CHKLMEM_MALLOC(local_translation,pg_translation*,
			    local_comm_size*sizeof(pg_translation),
			    mpi_errno,"local_translation"); */

	/* Make a list of the local communicator's process groups and encode
	   them in strings to be sent to the other side.
	   The encoded string for each process group contains the process
	   group id, size and all its KVS values */
	mpi_errno = MPIDI_ExtractLocalPGInfo( comm_ptr, local_translation,
					&pg_list, &n_local_pgs );

	/* Send the remote root: n_local_pgs, local_comm_size,
           Recv from the remote root: n_remote_pgs, remote_comm_size,
           recvcontext_id for newcomm */

        send_ints[0] = n_local_pgs;
        send_ints[1] = local_comm_size;
        send_ints[2] = recvcontext_id;

	TRACE_ERR("connect:sending 3 ints, %d, %d, %d, and receiving 2 ints with sendtag=%d recvtag=%d\n", send_ints[0], send_ints[1], send_ints[2], sendtag, recvtag);
        mpi_errno = MPIC_Sendrecv(send_ints, 3, MPI_INT, 0,
                                  sendtag++, recv_ints, 3, MPI_INT,
                                  0, recvtag++, tmp_comm,
                                  MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
            /* this is a no_port error because we may fail to connect
               on the send if the port name is invalid */
	    TRACE_ERR("MPIC_Sendrecv returned with mpi_errno=%d\n", mpi_errno);
	}

        mpi_errno = MPIC_Sendrecv_replace(&comm_cntr, 1, MPI_LONG_LONG_INT, 0,
                                  sendtag++, 0, recvtag++, tmp_comm,
                                  MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
            /* this is a no_port error because we may fail to connect
               on the send if the port name is invalid */
            TRACE_ERR("MPIC_Sendrecv returned with mpi_errno=%d\n", mpi_errno);
        }
    }

    /* broadcast the received info to local processes */
    TRACE_ERR("accept:broadcasting 2 ints - %d and %d\n", recv_ints[0], recv_ints[1]);
    mpi_errno = MPIR_Bcast_intra(recv_ints, 3, MPI_INT, root, comm_ptr, &errflag);
    if (mpi_errno) TRACE_ERR("MPIR_Bcast_intra returned with mpi_errno=%d\n", mpi_errno);

    mpi_errno = MPIR_Bcast_intra(&comm_cntr, 1, MPI_LONG_LONG_INT, root, comm_ptr, &errflag);
    if (mpi_errno) TRACE_ERR("MPIR_Bcast_intra returned with mpi_errno=%d\n", mpi_errno);

    if(lcomm_cntr > comm_cntr)  comm_cntr = lcomm_cntr;

    /* check if root was unable to connect to the port */

    n_remote_pgs     = recv_ints[0];
    remote_comm_size = recv_ints[1];
    context_id	     = recv_ints[2];

   TRACE_ERR("MPIDI_Comm_connect - n_remote_pgs=%d remote_comm_size=%d context_id=%d\n", n_remote_pgs,
	remote_comm_size, context_id);
    remote_pg = MPIU_Malloc(n_remote_pgs * sizeof(MPIDI_PG_t*));
    remote_translation = MPIU_Malloc(remote_comm_size * sizeof(pg_translation));
    /* Exchange the process groups and their corresponding KVSes */
    if (rank == root)
    {
	mpi_errno = MPIDI_SendPGtoPeerAndFree( tmp_comm, &sendtag, pg_list );
	mpi_errno = MPIDI_ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					n_remote_pgs, remote_pg );
	/* Receive the translations from remote process rank to process group
	   index */
	mpi_errno = MPIC_Sendrecv(local_translation, local_comm_size * 3,
				  MPI_INT, 0, sendtag++,
				  remote_translation, remote_comm_size * 3,
				  MPI_INT, 0, recvtag++, tmp_comm,
				  MPI_STATUS_IGNORE, &errflag);
	if (mpi_errno) {
	    TRACE_ERR("MPIC_Sendrecv returned with mpi_errno=%d\n", mpi_errno);
	}

	for (i=0; i<remote_comm_size; i++)
	{
	    TRACE_ERR(" remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
		i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank);
	}
    }
    else
    {
	mpi_errno = MPIDI_ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					    n_remote_pgs, remote_pg );
    }

    /* Broadcast out the remote rank translation array */
    mpi_errno = MPIR_Bcast_intra(remote_translation, remote_comm_size * 3, MPI_INT,
                                 root, comm_ptr, &errflag);
    if (mpi_errno) TRACE_ERR("MPIR_Bcast_intra returned with mpi_errno=%d\n", mpi_errno);

    char *pginfo = MPIU_Malloc(256*sizeof(char));
    memset(pginfo, 0, 256);
    char cp[20];
    for (i=0; i<remote_comm_size; i++)
    {
	TRACE_ERR(" remote_translation[%d].pg_index = %d remote_translation[%d].pg_rank = %d remote_translation[%d].pg_taskid=%d\n",
	    i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank, i, remote_translation[i].pg_taskid);
	    TRACE_ERR("remote_pg[remote_translation[%d].pg_index]->id=%s\n",i, (char *)(remote_pg[remote_translation[i].pg_index]->id));
	    strcat(pginfo, (char *)(remote_pg[remote_translation[i].pg_index]->id));
	    sprintf(cp, ":%d ", remote_translation[i].pg_taskid);
	    strcat(pginfo, cp);


    }
    pginfo[strlen(pginfo)]='\0';
    TRACE_ERR("connection info %s\n", pginfo);
    /*MPIDI_Parse_connection_info(n_remote_pgs, remote_pg);*/
    MPIU_Free(pginfo);

    mpi_errno = MPIR_Comm_create(newcomm);
    if (mpi_errno) TRACE_ERR("MPIR_Comm_create returned with mpi_errno=%d\n", mpi_errno);

    (*newcomm)->context_id     = context_id;
    (*newcomm)->recvcontext_id = recvcontext_id;
    (*newcomm)->is_low_group   = 1;

    mpi_errno = MPIDI_SetupNewIntercomm( comm_ptr, remote_comm_size,
				   remote_translation, n_remote_pgs, remote_pg, *newcomm );
    (*newcomm)->mpid.world_intercomm_cntr   = comm_cntr;
    WORLDINTCOMMCNTR = comm_cntr;
    MPIDI_add_new_tranid(comm_cntr);

/*    MPIDI_Parse_connection_info(n_remote_pgs, remote_pg); */
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIDI_SetupNewIntercomm returned with mpi_errno=%d\n", mpi_errno);
    }

    /* synchronize with remote root */
    if (rank == root)
    {
        mpi_errno = MPIC_Sendrecv(&i, 0, MPI_INT, 0,
                                  sendtag++, &j, 0, MPI_INT,
                                  0, recvtag++, tmp_comm,
                                  MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIC_Sendrecv returned with mpi_errno=%d\n", mpi_errno);
        }

        /* All communication with remote root done. Release the communicator. */
        MPIR_Comm_release(tmp_comm,0);
    }

    TRACE_ERR("connect:barrier\n");
    mpi_errno = MPIR_Barrier_intra(comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIR_Barrier_intra returned with mpi_errno=%d\n", mpi_errno);
    }

    TRACE_ERR("connect:free new vc\n");

fn_exit:
    if(local_translation) MPIU_Free(local_translation);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/*
 * Extract all of the process groups from the given communicator and
 * form a list (returned in pg_list) of those process groups.
 * Also returned is an array (local_translation) that contains tuples mapping
 * rank in process group to rank in that communicator (local translation
 * must be allocated before this routine is called).  The number of
 * distinct process groups is returned in n_local_pgs_p .
 *
 * This allows an intercomm_create to exchange the full description of
 * all of the process groups that have made up the communicator that
 * will define the "remote group".
 */
static int MPIDI_ExtractLocalPGInfo( struct MPID_Comm *comm_p,
			       pg_translation local_translation[],
			       pg_node **pg_list_p,
			       int *n_local_pgs_p )
{
    pg_node        *pg_list = 0, *pg_iter, *pg_trailer;
    int            i, cur_index = 0, local_comm_size, mpi_errno = 0;
    char           *pg_id;

    local_comm_size = comm_p->local_size;

    /* Make a list of the local communicator's process groups and encode
       them in strings to be sent to the other side.
       The encoded string for each process group contains the process
       group id, size and all its KVS values */

    cur_index = 0;
    pg_list = MPIU_Malloc(sizeof(pg_node));

    pg_list->pg_id = MPIU_Strdup(comm_p->vcr[0]->pg->id);
    pg_list->index = cur_index++;
    pg_list->next = NULL;
    /* XXX DJG FIXME-MT should we be checking this?  the add/release macros already check this */
    TRACE_ERR("MPIU_Object_get_ref(comm_p->vcr[0]->pg) comm_p=%x vsr=%x pg=%x %d\n", comm_p, comm_p->vcr[0], comm_p->vcr[0]->pg, MPIU_Object_get_ref(comm_p->vcr[0]->pg));
    MPIU_Assert( MPIU_Object_get_ref(comm_p->vcr[0]->pg));
    mpi_errno = MPIDI_PG_To_string(comm_p->vcr[0]->pg, &pg_list->str,
				   &pg_list->lenStr );
    TRACE_ERR("pg_list->str=%s pg_list->lenStr=%d\n", pg_list->str, pg_list->lenStr);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIDI_PG_To_string returned with mpi_errno=%d\n", mpi_errno);
    }
    TRACE_ERR("PG as string is %s\n", pg_list->str );
    local_translation[0].pg_index = 0;
    local_translation[0].pg_rank = comm_p->vcr[0]->pg_rank;
    local_translation[0].pg_taskid = comm_p->vcr[0]->taskid;
    TRACE_ERR("local_translation[0].pg_index=%d local_translation[0].pg_rank=%d\n", local_translation[0].pg_index, local_translation[0].pg_rank);
    pg_iter = pg_list;
    for (i=1; i<local_comm_size; i++) {
	pg_iter = pg_list;
	pg_trailer = pg_list;
	while (pg_iter != NULL) {
	    /* Check to ensure pg is (probably) valid */
            /* XXX DJG FIXME-MT should we be checking this?  the add/release macros already check this */
	    MPIU_Assert(MPIU_Object_get_ref(comm_p->vcr[i]->pg) != 0);
	    if (MPIDI_PG_Id_compare(comm_p->vcr[i]->pg->id, pg_iter->pg_id)) {
		local_translation[i].pg_index = pg_iter->index;
		local_translation[i].pg_rank  = comm_p->vcr[i]->pg_rank;
		local_translation[i].pg_taskid  = comm_p->vcr[i]->taskid;
                TRACE_ERR("local_translation[%d].pg_index=%d local_translation[%d].pg_rank=%d\n", i, local_translation[i].pg_index, i,local_translation[i].pg_rank);
		break;
	    }
	    if (pg_trailer != pg_iter)
		pg_trailer = pg_trailer->next;
	    pg_iter = pg_iter->next;
	}
	if (pg_iter == NULL) {
	    /* We use MPIU_Malloc directly because we do not know in
	       advance how many nodes we may allocate */
	    pg_iter = (pg_node*)MPIU_Malloc(sizeof(pg_node));
	    pg_iter->pg_id = MPIU_Strdup(comm_p->vcr[i]->pg->id);
	    pg_iter->index = cur_index++;
	    pg_iter->next = NULL;
	    mpi_errno = MPIDI_PG_To_string(comm_p->vcr[i]->pg, &pg_iter->str,
					   &pg_iter->lenStr );

            TRACE_ERR("cur_index=%d pg_iter->str=%s pg_iter->lenStr=%d\n", cur_index, pg_iter->str, pg_iter->lenStr);
	    if (mpi_errno != MPI_SUCCESS) {
		TRACE_ERR("MPIDI_PG_To_string returned with mpi_errno=%d\n", mpi_errno);
	    }
	    local_translation[i].pg_index = pg_iter->index;
	    local_translation[i].pg_rank = comm_p->vcr[i]->pg_rank;
	    local_translation[i].pg_taskid = comm_p->vcr[i]->taskid;
	    pg_trailer->next = pg_iter;
	}
    }

    *n_local_pgs_p = cur_index;
    *pg_list_p     = pg_list;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



/* The root process in comm_ptr receives strings describing the
   process groups and then distributes them to the other processes
   in comm_ptr.
   See SendPGToPeer for the routine that sends the descriptions */
static int MPIDI_ReceivePGAndDistribute( struct MPID_Comm *tmp_comm, struct MPID_Comm *comm_ptr,
				   int root, int *recvtag_p,
				   int n_remote_pgs, MPIDI_PG_t *remote_pg[] )
{
    char *pg_str = 0;
    char *pginfo = 0;
    int  i, j, flag;
    int  rank = comm_ptr->rank;
    int  mpi_errno = 0;
    int  recvtag = *recvtag_p;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    TRACE_ERR("MPIDI_ReceivePGAndDistribute - n_remote_pgs=%d\n", n_remote_pgs);
    for (i=0; i<n_remote_pgs; i++) {

	if (rank == root) {
	    /* First, receive the pg description from the partner */
	    mpi_errno = MPIC_Recv(&j, 1, MPI_INT, 0, recvtag++,
				  tmp_comm, MPI_STATUS_IGNORE, &errflag);
	    *recvtag_p = recvtag;
	    if (mpi_errno != MPI_SUCCESS) {
		TRACE_ERR("MPIC_Recv returned with mpi_errno=%d\n", mpi_errno);
	    }
	    pg_str = (char*)MPIU_Malloc(j);
	    mpi_errno = MPIC_Recv(pg_str, j, MPI_CHAR, 0, recvtag++,
				  tmp_comm, MPI_STATUS_IGNORE, &errflag);
	    *recvtag_p = recvtag;
	    if (mpi_errno != MPI_SUCCESS) {
		TRACE_ERR("MPIC_Recv returned with mpi_errno=%d\n", mpi_errno);
	    }
	}

	/* Broadcast the size and data to the local communicator */
	TRACE_ERR("accept:broadcasting 1 int\n");
	mpi_errno = MPIR_Bcast_intra(&j, 1, MPI_INT, root, comm_ptr, &errflag);
	if (mpi_errno != MPI_SUCCESS) TRACE_ERR("MPIR_Bcast_intra returned with mpi_errno=%d\n", mpi_errno);

	if (rank != root) {
	    /* The root has already allocated this string */
	    pg_str = (char*)MPIU_Malloc(j);
	}
	TRACE_ERR("accept:broadcasting string of length %d\n", j);
	pg_str[j-1]='\0';
	mpi_errno = MPIR_Bcast_intra(pg_str, j, MPI_CHAR, root, comm_ptr, &errflag);
	if (mpi_errno != MPI_SUCCESS)
           TRACE_ERR("MPIR_Bcast_intra returned with mpi_errno=%d\n", mpi_errno);
	/* Then reconstruct the received process group.  This step
	   also initializes the created process group */

	TRACE_ERR("Adding connection information - pg_str=%s\n", pg_str);
	TRACE_ERR("Creating pg from string %s flag=%d\n", pg_str, flag);
	mpi_errno = MPIDI_PG_Create_from_string(pg_str, &remote_pg[i], &flag);
        TRACE_ERR("remote_pg[%d]->id=%s\n", i, (char*)(remote_pg[i]->id));
	if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIDI_PG_Create_from_string returned with mpi_errno=%d\n", mpi_errno);
	}

	MPIU_Free(pg_str);
    }
    /*MPIDI_Parse_connection_info(pg_str); */
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


void MPIDI_Add_connection_info(int wid, int wsize, pami_task_t *taskids) {
  int jobIdSize=64;
  char jobId[jobIdSize];
  int ref_count, i;
  conn_info *tmp_node1=NULL, *tmp_node2=NULL;

  TRACE_ERR("MPIDI_Add_connection_info ENTER wid=%d wsize=%d\n", wid, wsize);
  PMI2_Job_GetId(jobId, jobIdSize);
  if(atoi(jobId) == wid)
	return;

  /* FIXME: check the lock */
  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  if(_conn_info_list == NULL) { /* Connection list is not yet created */
    _conn_info_list = (conn_info*) MPIU_Malloc(sizeof(conn_info));
    _conn_info_list->rem_world_id = wid;
      _conn_info_list->ref_count    = 1;

    ref_count = _conn_info_list->ref_count;
    if(taskids != NULL) {
      _conn_info_list->rem_taskids = MPIU_Malloc((wsize+1)*sizeof(int));
      for(i=0;i<wsize;i++) {
        _conn_info_list->rem_taskids[i] = taskids[i];
      }
      _conn_info_list->rem_taskids[i]   = -1;
    }
    else
      _conn_info_list->rem_taskids = NULL;
    _conn_info_list->next = NULL;
  }
  else {
    tmp_node1 = _conn_info_list;
    while(tmp_node1) {
      tmp_node2 = tmp_node1;
      if(tmp_node1->rem_world_id == wid)
        break;
      tmp_node1 = tmp_node1->next;
    }
    if(tmp_node1) {  /* Connection already exists. Increment reference count */
      if(tmp_node1->ref_count == 0) {
        if(taskids != NULL) {
          tmp_node1->rem_taskids = MPIU_Malloc((wsize+1)*sizeof(int));
          for(i=0;i<wsize;i++) {
            tmp_node1->rem_taskids[i] = taskids[i];
          }
          tmp_node1->rem_taskids[i] = -1;
        }
        tmp_node1->rem_world_id = wid;
      }
      tmp_node1->ref_count++;
      ref_count = tmp_node1->ref_count;
    }
    else {           /* Connection do not exists. Create a new connection */
      tmp_node2->next = (conn_info*) MPIU_Malloc(sizeof(conn_info));
      tmp_node2 = tmp_node2->next;
      tmp_node2->rem_world_id = wid;
        tmp_node2->ref_count    = 1;

      ref_count = tmp_node2->ref_count;
      if(taskids != NULL) {
        tmp_node2->rem_taskids = MPIU_Malloc((wsize+1)*sizeof(int));
        for(i=0;i<wsize;i++) {
          tmp_node2->rem_taskids[i] = taskids[i];
        }
        tmp_node2->rem_taskids[i] = -1;
      }
      else
        tmp_node2->rem_taskids = NULL;
      tmp_node2->next = NULL;
    }
  }
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);

  tmp_node1 = _conn_info_list;
  while(tmp_node1) {
    TRACE_ERR("REM WORLD=%d ref_count=%d", tmp_node1->rem_world_id,tmp_node1->ref_count);

    tmp_node1 = tmp_node1->next;
  }
}


/**
 * This routine adds the remote world (wid) to local known world linked list
 * if there is no record of it before, or increment the reference count
 * associated with world (wid) if it is known before
 */
void MPIDI_Parse_connection_info(int n_remote_pgs, MPIDI_PG_t **remote_pg) {
  int i, p, ref_count=0;
  int jobIdSize=8;
  char jobId[jobIdSize];
  char *pginfo_sav, *pgid_taskid_sav, *pgid, *pgid_taskid[20], *pginfo_tmp, *cp3, *cp2;
  pami_task_t *taskids;
  int n_rem_wids=0;
  int mpi_errno = MPI_SUCCESS;
  MPIDI_PG_t *existing_pg;

  for(p=0; p<n_remote_pgs; p++) {
        TRACE_ERR("call MPIDI_PG_Find to find %s\n", (char*)(remote_pg[p]->id));
        mpi_errno = MPIDI_PG_Find(remote_pg[p]->id, &existing_pg);
        if (mpi_errno) TRACE_ERR("MPIDI_PG_Find failed\n");

         if (existing_pg != NULL) {
	  taskids = MPIU_Malloc((existing_pg->size)*sizeof(pami_task_t));
          for(i=0; i<existing_pg->size; i++) {
             taskids[i]=existing_pg->vct[i].taskid;
	     TRACE_ERR("id=%s taskids[%d]=%d\n", (char*)(remote_pg[p]->id), i, taskids[i]);
          }
          MPIDI_Add_connection_info(atoi((char*)(remote_pg[p]->id)), existing_pg->size, taskids);
	  MPIU_Free(taskids);
        }
  }
}



/**
 * Function to increment the active message counter for a particular trasaction id.
 * This function is used inside disconnect routine
 * whichAM = FIRST_AM/SECOND_AM/LAST_AM
 */
void MPIDI_increment_AM_cntr_for_tranid(long long tranid, int whichAM)
{
  transactionID_struct *tridtmp;

  /* No error thrown here if tranid not found. This is for the case where timout
   * happened in MPI_Comm_disconnect and tasks have freed the tranid list node
   * and now after this the Active message is received.
   */

  tridtmp = _transactionID_list;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  while(tridtmp != NULL) {
    if(tridtmp->tranid == tranid) {
      tridtmp->cntr_for_AM[whichAM]++;
      break;
    }
    tridtmp = tridtmp->next;
  }
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);

  TRACE_ERR("MPIDI_increment_AM_cntr_for_tranid - tridtmp->cntr_for_AM[%d]=%d\n",
          whichAM, tridtmp->cntr_for_AM[whichAM]);
}

/**
 * Function to free a partucular trasaction id node from the trasaction id list.
 * This function is called inside disconnect routine once the remote connection is
 * terminated
 */
void MPIDI_free_tranid_node(long long tranid)
{
  transactionID_struct *tridtmp, *tridtmp2;

  MPID_assert(_transactionID_list != NULL);

  tridtmp = tridtmp2 = _transactionID_list;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  while(tridtmp != NULL) {
    if(tridtmp->tranid == tranid) {
      /* If there is only one node */
      if(_transactionID_list->next == NULL) {
        MPIU_Free(_transactionID_list->cntr_for_AM);
        MPIU_Free(_transactionID_list);
        _transactionID_list = NULL;
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
        return;
      }
      /* If more than one node and if this is the first node of the list */
      if(tridtmp == _transactionID_list && tridtmp->next != NULL) {
        _transactionID_list = _transactionID_list->next;
        MPIU_Free(tridtmp->cntr_for_AM);
        MPIU_Free(tridtmp);
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
        return;
      }
      /* For rest all other nodes position of the list */
      tridtmp2->next = tridtmp->next;
      MPIU_Free(tridtmp->cntr_for_AM);
      MPIU_Free(tridtmp);
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
      return;
    }
    tridtmp2 = tridtmp;
    tridtmp = tridtmp->next;
  }
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
}

/** This routine is used inside finalize to free all the nodes
 * if the disconnect call has not been called
 */
void MPIDI_free_all_tranid_node()
{
  transactionID_struct *tridtmp;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  while(_transactionID_list != NULL) {
    tridtmp = _transactionID_list;
    _transactionID_list = _transactionID_list->next;
    MPIU_Free(tridtmp->cntr_for_AM);
    MPIU_Free(tridtmp);
  }
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
}

/* Sends the process group information to the peer and frees the
   pg_list */
static int MPIDI_SendPGtoPeerAndFree( struct MPID_Comm *tmp_comm, int *sendtag_p,
				pg_node *pg_list )
{
    int mpi_errno = 0;
    int sendtag = *sendtag_p, i;
    pg_node *pg_iter;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    while (pg_list != NULL) {
	pg_iter = pg_list;
        i = pg_iter->lenStr;
	TRACE_ERR("connect:sending 1 int: %d\n", i);
	mpi_errno = MPIC_Send(&i, 1, MPI_INT, 0, sendtag++, tmp_comm, &errflag);
	*sendtag_p = sendtag;
	if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIC_Send returned with mpi_errno=%d\n", mpi_errno);
	}

	TRACE_ERR("connect:sending string length %d\n", i);
	mpi_errno = MPIC_Send(pg_iter->str, i, MPI_CHAR, 0, sendtag++,
			      tmp_comm, &errflag);
	*sendtag_p = sendtag;
	if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIC_Send returned with mpi_errno=%d\n", mpi_errno);
	}

	pg_list = pg_list->next;
	MPIU_Free(pg_iter->str);
	MPIU_Free(pg_iter->pg_id);
	MPIU_Free(pg_iter);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* ---------------------------------------------------------------------- */
/*
 * MPIDI_Comm_accept()

   Algorithm: First dequeue the vc from the accept queue (it was
   enqueued by the progress engine in response to a connect request
   from the root process that is attempting the connection on
   the connect side). Use this vc to create an
   intercommunicator between this root and the root on the connect
   side. Use this intercomm. to communicate the other information
   needed to create the real intercommunicator between the processes
   on the two sides. Then free the intercommunicator between the
   roots. Most of the complexity is because there can be multiple
   process groups on each side.

 */
int MPIDI_Comm_accept(const char *port_name, MPID_Info *info, int root,
		      struct MPID_Comm *comm_ptr, struct MPID_Comm **newcomm)
{
    int mpi_errno=MPI_SUCCESS;
    int i, j, rank, recv_ints[3], send_ints[3], context_id;
    int remote_comm_size=0;
    struct MPID_Comm *tmp_comm = NULL, *intercomm;
    MPID_VCR new_vc = NULL;
    int sendtag=100, recvtag=100, local_comm_size;
    int n_local_pgs=1, n_remote_pgs;
    pg_translation *local_translation = NULL, *remote_translation = NULL;
    pg_node *pg_list = NULL;
    MPIDI_PG_t **remote_pg = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    char send_char[16], recv_char[16], remote_taskids[16];
    long long comm_cntr, lcomm_cntr;
    int leader_taskid;

    /* Create the new intercommunicator here. We need to send the
       context id to the other side. */
    mpi_errno = MPIR_Comm_create(newcomm);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIR_Comm_create returned with mpi_errno=%d\n", mpi_errno);
    }
    mpi_errno = MPIR_Get_contextid( comm_ptr, &(*newcomm)->recvcontext_id );
    TRACE_ERR("In MPIDI_Comm_accept - MPIR_Get_contextid=%d\n", (*newcomm)->recvcontext_id);
    if (mpi_errno) TRACE_ERR("MPIR_Get_contextid returned with mpi_errno=%d\n", mpi_errno);
    /* FIXME why is this commented out? */
    /*    (*newcomm)->context_id = (*newcomm)->recvcontext_id; */

    rank = comm_ptr->rank;
    local_comm_size = comm_ptr->local_size;

    WORLDINTCOMMCNTR += 1;
    comm_cntr = WORLDINTCOMMCNTR;
    lcomm_cntr = WORLDINTCOMMCNTR;

    if (rank == root)
    {
	/* Establish a communicator to communicate with the root on the
	   other side. */
	mpi_errno = MPIDI_Create_inter_root_communicator_accept(port_name,
						&tmp_comm, &new_vc);
	TRACE_ERR("done MPIDI_Create_inter_root_communicator_accept mpi_errno=%d tmp_comm=%p new_vc=%p \n", mpi_errno, tmp_comm, new_vc);
	if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIDI_Create_inter_root_communicator_accept returned with mpi_errno=%d\n", mpi_errno);
	}

	/* Make an array to translate local ranks to process group index and
	   rank */
	local_translation = MPIU_Malloc(local_comm_size*sizeof(pg_translation));
/*	MPIU_CHKLMEM_MALLOC(local_translation,pg_translation*,
			    local_comm_size*sizeof(pg_translation),
			    mpi_errno,"local_translation"); */

	/* Make a list of the local communicator's process groups and encode
	   them in strings to be sent to the other side.
	   The encoded string for each process group contains the process
	   group id, size and all its KVS values */
	mpi_errno = MPIDI_ExtractLocalPGInfo( comm_ptr, local_translation,
					&pg_list, &n_local_pgs );
        /* Send the remote root: n_local_pgs, local_comm_size, context_id for
	   newcomm.
           Recv from the remote root: n_remote_pgs, remote_comm_size */

        send_ints[0] = n_local_pgs;
        send_ints[1] = local_comm_size;
        send_ints[2] = (*newcomm)->recvcontext_id;

	TRACE_ERR("accept:sending 3 ints, %d, %d, %d, and receiving 2 ints with sendtag=%d recvtag=%d\n", send_ints[0], send_ints[1], send_ints[2], sendtag, recvtag);
        mpi_errno = MPIC_Sendrecv(send_ints, 3, MPI_INT, 0,
                                  sendtag++, recv_ints, 3, MPI_INT,
                                  0, recvtag++, tmp_comm,
                                  MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIC_Sendrecv returned with mpi_errno=%d\n", mpi_errno);
	}
#if 0
	send_char = pg_list->str;
	TRACE_ERR("accept:sending 1 string and receiving 1 string\n", send_char, recv_char);
        mpi_errno = MPIC_Sendrecv(send_char, 1, MPI_CHAR, 0,
                                     sendtag++, recv_char, 3, MPI_CHAR,
                                     0, recvtag++, tmp_comm,
                                     MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIC_Sendrecv returned with mpi_errno=%d\n", mpi_errno);
	}
#endif
        mpi_errno = MPIC_Sendrecv_replace(&comm_cntr, 1, MPI_LONG_LONG_INT, 0,
                                  sendtag++, 0, recvtag++, tmp_comm,
                                  MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
            /* this is a no_port error because we may fail to connect
               on the send if the port name is invalid */
            TRACE_ERR("MPIC_Sendrecv returned with mpi_errno=%d\n", mpi_errno);
        }

    }

    /* broadcast the received info to local processes */
    TRACE_ERR("accept:broadcasting 2 ints - %d and %d\n", recv_ints[0], recv_ints[1]);
    mpi_errno = MPIR_Bcast_intra(recv_ints, 3, MPI_INT, root, comm_ptr, &errflag);
    if (mpi_errno) TRACE_ERR("MPIR_Bcast_intra returned with mpi_errno=%d\n", mpi_errno);

    mpi_errno = MPIR_Bcast_intra(&comm_cntr, 1, MPI_LONG_LONG_INT, root, comm_ptr, &errflag);
    if (mpi_errno) TRACE_ERR("MPIR_Bcast_intra returned with mpi_errno=%d\n", mpi_errno);

    if(lcomm_cntr > comm_cntr)  comm_cntr = lcomm_cntr;
    n_remote_pgs     = recv_ints[0];
    remote_comm_size = recv_ints[1];
    context_id       = recv_ints[2];
    remote_pg = MPIU_Malloc(n_remote_pgs * sizeof(MPIDI_PG_t*));
    remote_translation =  MPIU_Malloc(remote_comm_size * sizeof(pg_translation));
    TRACE_ERR("[%d]accept:remote process groups: %d\nremote comm size: %d\nrecv_char: %s\n", rank, n_remote_pgs, remote_comm_size, remote_taskids);

    /* Exchange the process groups and their corresponding KVSes */
    if (rank == root)
    {
	/* The root receives the PG from the peer (in tmp_comm) and
	   distributes them to the processes in comm_ptr */
	mpi_errno = MPIDI_ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					n_remote_pgs, remote_pg );

	mpi_errno = MPIDI_SendPGtoPeerAndFree( tmp_comm, &sendtag, pg_list );

	/* Receive the translations from remote process rank to process group index */
	TRACE_ERR("accept:sending %d ints and receiving %d ints\n", local_comm_size * 2, remote_comm_size * 2);
	mpi_errno = MPIC_Sendrecv(local_translation, local_comm_size * 3,
				  MPI_INT, 0, sendtag++,
				  remote_translation, remote_comm_size * 3,
				  MPI_INT, 0, recvtag++, tmp_comm,
                                  MPI_STATUS_IGNORE, &errflag);
	for (i=0; i<remote_comm_size; i++)
	{
	    TRACE_ERR(" remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
		i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank);
	}
    }
    else
    {
	mpi_errno = MPIDI_ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					    n_remote_pgs, remote_pg );
    }
     for(i=0; i<n_remote_pgs; i++)
     {
	    TRACE_ERR("after calling MPIDI_ReceivePGAndDistribute - remote_pg[%d]->id=%s\n",i, (char *)(remote_pg[i]->id));
     }


    /* Broadcast out the remote rank translation array */
    TRACE_ERR("Broadcast remote_translation");
    mpi_errno = MPIR_Bcast_intra(remote_translation, remote_comm_size * 3, MPI_INT,
                                 root, comm_ptr, &errflag);
    if (mpi_errno) TRACE_ERR("MPIR_Bcast_intra returned with mpi_errno=%d\n", mpi_errno);
    TRACE_ERR("[%d]accept:Received remote_translation after broadcast:\n", rank);
    char *pginfo = MPIU_Malloc(256*sizeof(char));
    memset(pginfo, 0, 256);
    char cp[20];
    for (i=0; i<remote_comm_size; i++)
    {
	TRACE_ERR(" remote_translation[%d].pg_index = %d remote_translation[%d].pg_rank = %d remote_translation[%d].pg_taskid=%d\n",
	    i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank, i, remote_translation[i].pg_taskid);
	TRACE_ERR("remote_pg[remote_translation[%d].pg_index]->id=%s\n",i, (char *)(remote_pg[remote_translation[i].pg_index]->id));
	    strcat(pginfo, (char *)(remote_pg[remote_translation[i].pg_index]->id));
	    sprintf(cp, ":%d ", remote_translation[i].pg_taskid);
	    strcat(pginfo, cp);


    }
    pginfo[strlen(pginfo)]='\0';
    TRACE_ERR("connection info %s\n", pginfo);
/*    MPIDI_Parse_connection_info(n_remote_pgs, remote_pg); */
    MPIU_Free(pginfo);


    /* Now fill in newcomm */
    intercomm               = *newcomm;
    intercomm->context_id   = context_id;
    intercomm->is_low_group = 0;

    mpi_errno = MPIDI_SetupNewIntercomm( comm_ptr, remote_comm_size,
				   remote_translation, n_remote_pgs, remote_pg, intercomm );
    intercomm->mpid.world_intercomm_cntr   = comm_cntr;
    WORLDINTCOMMCNTR = comm_cntr;
    MPIDI_add_new_tranid(comm_cntr);

    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIDI_SetupNewIntercomm returned with mpi_errno=%d\n", mpi_errno);
    }

    /* synchronize with remote root */
    if (rank == root)
    {
        mpi_errno = MPIC_Sendrecv(&i, 0, MPI_INT, 0,
                                  sendtag++, &j, 0, MPI_INT,
                                  0, recvtag++, tmp_comm,
                                  MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("MPIC_Sendrecv returned with mpi_errno=%d\n", mpi_errno);
        }

        /* All communication with remote root done. Release the communicator. */
        MPIR_Comm_release(tmp_comm,0);
    }

    mpi_errno = MPIR_Barrier_intra(comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIR_Barrier_intra returned with mpi_errno=%d\n", mpi_errno);
    }

fn_exit:
    if(local_translation) MPIU_Free(local_translation);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------- */

/* This routine initializes the new intercomm, setting up the
   VCRT and other common structures.  The is_low_group and context_id
   fields are NOT set because they differ in the use of this
   routine in Comm_accept and Comm_connect.  The virtual connections
   are initialized from a collection of process groups.

   Input parameters:
+  comm_ptr - communicator that gives the group for the "local" group on the
   new intercommnicator
.  remote_comm_size - size of remote group
.  remote_translation - array that specifies the process group and rank in
   that group for each of the processes to include in the remote group of the
   new intercommunicator
-  remote_pg - array of remote process groups

   Input/Output Parameter:
.  intercomm - New intercommunicator.  The intercommunicator must already
   have been allocated; this routine initializes many of the fields

   Note:
   This routine performance a barrier over 'comm_ptr'.  Why?
*/
static int MPIDI_SetupNewIntercomm( struct MPID_Comm *comm_ptr, int remote_comm_size,
			      pg_translation remote_translation[],
			      int n_remote_pgs, MPIDI_PG_t **remote_pg,
			      struct MPID_Comm *intercomm )
{
    int mpi_errno = MPI_SUCCESS, i, j, index=0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int total_rem_world_cnts, p=0;
    char *world_tasks, *cp1;
    conn_info *tmp_node;
    int conn_world_ids[64];
    MPID_VCR *worldlist;
    int worldsize;
    pami_endpoint_t dest;
    MPID_Comm *comm;
    pami_task_t leader1=-1, leader2=-1, leader_taskid=-1;
    long long comm_cntr=0, lcomm_cntr=-1;
    int jobIdSize=64;
    char jobId[jobIdSize];

    TRACE_ERR("MPIDI_SetupNewIntercomm - remote_comm_size=%d\n", remote_comm_size);
    /* FIXME: How much of this could/should be common with the
       upper level (src/mpi/comm/ *.c) code? For best robustness,
       this should use the same routine (not copy/paste code) as
       in the upper level code. */
    intercomm->attributes   = NULL;
    intercomm->remote_size  = remote_comm_size;
    intercomm->local_size   = comm_ptr->local_size;
    intercomm->rank         = comm_ptr->rank;
    intercomm->local_group  = NULL;
    intercomm->remote_group = NULL;
    intercomm->comm_kind    = MPID_INTERCOMM;
    intercomm->local_comm   = NULL;
    intercomm->coll_fns     = NULL;
    intercomm->mpid.world_ids = NULL; /*FIXME*/

    /* Point local vcr, vcrt at those of incoming intracommunicator */
    intercomm->local_vcrt = comm_ptr->vcrt;
    MPID_VCRT_Add_ref(comm_ptr->vcrt);
    intercomm->local_vcr  = comm_ptr->vcr;
    for(i=0; i<comm_ptr->local_size; i++)
	TRACE_ERR("intercomm->local_vcr[%d]->pg_rank=%d comm_ptr->vcr[%d].pg_rank=%d intercomm->local_vcr[%d]->taskid=%d comm_ptr->vcr[%d]->taskid=%d\n", i, intercomm->local_vcr[i]->pg_rank, i, comm_ptr->vcr[i]->pg_rank, i, intercomm->local_vcr[i]->taskid, i, comm_ptr->vcr[i]->taskid);

    /* Set up VC reference table */
    mpi_errno = MPID_VCRT_Create(intercomm->remote_size, &intercomm->vcrt);
    if (mpi_errno != MPI_SUCCESS) {
        TRACE_ERR("MPID_VCRT_Create returned with mpi_errno=%d\n", mpi_errno);
    }
    mpi_errno = MPID_VCRT_Get_ptr(intercomm->vcrt, &intercomm->vcr);
    if (mpi_errno != MPI_SUCCESS) {
        TRACE_ERR("MPID_VCRT_Get_ptr returned with mpi_errno=%d\n", mpi_errno);
    }

    for (i=0; i < intercomm->remote_size; i++) {
	MPIDI_PG_Dup_vcr(remote_pg[remote_translation[i].pg_index],
			 remote_translation[i].pg_rank, remote_translation[i].pg_taskid,&intercomm->vcr[i]);
	TRACE_ERR("MPIDI_SetupNewIntercomm - pg_id=%s pg_rank=%d pg_taskid=%d intercomm->vcr[%d]->taskid=%d intercomm->vcr[%d]->pg=%x\n ", remote_pg[remote_translation[i].pg_index]->id, remote_translation[i].pg_rank, remote_translation[i].pg_taskid, i, intercomm->vcr[i]->taskid, i, intercomm->vcr[i]->pg);
	PAMI_Endpoint_create(MPIDI_Client, remote_translation[i].pg_taskid, 0, &dest);
	PAMI_Resume(MPIDI_Context[0],
                    &dest, 1);
    }

    MPIDI_Parse_connection_info(n_remote_pgs, remote_pg);

    /* anchor connection information in mpid */
    total_rem_world_cnts = 0;
    tmp_node = _conn_info_list;
    p=0;
    while(tmp_node != NULL) {
       total_rem_world_cnts++;
       conn_world_ids[p++]=tmp_node->rem_world_id;
       tmp_node = tmp_node->next;
    }
    if(intercomm->mpid.world_ids) { /* need to look at other places that may populate world id list for this communicator */
      for(i=0;intercomm->mpid.world_ids[i]!=-1;i++)
      {
        for(j=0;j<total_rem_world_cnts;j++) {
          if(intercomm->mpid.world_ids[i] == conn_world_ids[j]) {
            conn_world_ids[j] = -1;
          }
        }
      }
      /* Now Total world_ids inside intercomm->world_ids = i, excluding last entry of ' -1' */
      index = 0;
      for(j=0;j<total_rem_world_cnts;j++) {
        if(conn_world_ids[j] != -1)
          index++;
      }
      if(index) {
        intercomm->mpid.world_ids = MPIU_Malloc((index+i+1)*sizeof(int));
        /* Current index i inside intercomm->mpid.world_ids is
         * the place where next world_id can be added
         */
        for(j=0;j<total_rem_world_cnts;j++) {
          if(conn_world_ids[j] != -1) {
            intercomm->mpid.world_ids[i++] = conn_world_ids[j];
          }
        }
        intercomm->mpid.world_ids[i] = -1;
      }
   }
   else {
    index=0;
    intercomm->mpid.world_ids = MPIU_Malloc((n_remote_pgs+1)*sizeof(int));
    PMI2_Job_GetId(jobId, jobIdSize);
    for(i=0;i<n_remote_pgs;i++) {
      if(atoi(jobId) != atoi((char *)remote_pg[i]->id) )
	intercomm->mpid.world_ids[index++] = atoi((char *)remote_pg[i]->id);
    }
    intercomm->mpid.world_ids[index++] = -1;
   }
   for(i=0; intercomm->mpid.world_ids[i] != -1; i++)
     TRACE_ERR("intercomm=%x intercomm->mpid.world_ids[%d]=%d\n", intercomm, i, intercomm->mpid.world_ids[i]);

   leader_taskid = comm_ptr->vcr[0]->taskid;

   MPID_Comm *comm_world_ptr = MPIR_Process.comm_world;
   worldlist = comm_world_ptr->vcr;
   worldsize = comm_world_ptr->local_size;
   comm = intercomm;
   for(i=0;i<intercomm->local_size;i++)
     {
       for(j=0;j<comm_world_ptr->local_size;j++)
	 {
	   if(intercomm->local_vcr[i]->taskid == comm_world_ptr->vcr[j]->taskid) {
	     leader1 = comm_world_ptr->vcr[j]->taskid;
	     break;
	   }
	 }
       if(leader1 != -1)
	 break;
     }
   for(i=0;i<intercomm->remote_size;i++)
     {
       for(j=0;j<comm_world_ptr->local_size;j++)
	 {
	   if(intercomm->vcr[i]->taskid == comm_world_ptr->vcr[j]->taskid) {
	     leader2 = comm_world_ptr->vcr[j]->taskid;
	     break;
	   }
	 }
       if(leader2 != -1)
	 break;
     }
   
   if(leader1 == -1)
     leader_taskid = leader2;
   else if(leader2 == -1)
     leader_taskid = leader1;
   else
     leader_taskid = leader1 < leader2 ? leader1 : leader2;
   intercomm->mpid.local_leader = leader_taskid;

   mpi_errno = MPIR_Comm_commit(intercomm);
   if (mpi_errno) TRACE_ERR("MPIR_Comm_commit returned with mpi_errno=%d\n", mpi_errno);

    mpi_errno = MPIR_Barrier_intra(comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIR_Barrier_intra returned with mpi_errno=%d\n", mpi_errno);
   }

 fn_exit:
    if(remote_pg) MPIU_Free(remote_pg);
    if(remote_translation) MPIU_Free(remote_translation);
    return mpi_errno;

 fn_fail:
    goto fn_exit;
}


/* Attempt to dequeue a vc from the accept queue. If the queue is
   empty or the port_name_tag doesn't match, return a NULL vc. */
int MPIDI_Acceptq_dequeue(MPID_VCR * vcr, int port_name_tag)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_Acceptq_t *q_item, *prev;
    *vcr = NULL;
    q_item = acceptq_head;
    prev = q_item;

    while (q_item != NULL)
    {
	if (q_item->port_name_tag == port_name_tag)
	{
	    *vcr = q_item->vcr;

	    if ( q_item == acceptq_head )
		acceptq_head = q_item->next;
	    else
		prev->next = q_item->next;

	    MPIU_Free(q_item);
	    AcceptQueueSize--;
	    break;;
	}
	else
	{
	    prev = q_item;
	    q_item = q_item->next;
	}
    }

    return mpi_errno;
}


/**
 * This routine return the list of taskids associated with world (wid)
 */
int* MPIDI_get_taskids_in_world_id(int wid) {
  conn_info *tmp_node;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  tmp_node = _conn_info_list;
  while(tmp_node != NULL) {
    if(tmp_node->rem_world_id == wid) {
      break;
    }
    tmp_node = tmp_node->next;
  }
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
  if(tmp_node == NULL)
    return NULL;
  else
    return (tmp_node->rem_taskids);
}

void MPIDI_IpState_reset(int dest)
{
  MPIDI_In_cntr_t *in_cntr;
  in_cntr=&MPIDI_In_cntr[dest];

  in_cntr->n_OutOfOrderMsgs = 0;
  in_cntr->nMsgs = 0;
  in_cntr->OutOfOrderList = NULL;
}


void MPIDI_OpState_reset(int dest)
{
  MPIDI_Out_cntr_t *out_cntr;
  out_cntr=&MPIDI_Out_cntr[dest];

  out_cntr->nMsgs = 0;
  out_cntr->unmatched = 0;
}


/**
 * This routine return the connection reference count associated with the
 * remote world identified by wid
 */
int MPIDI_get_refcnt_of_world(int wid) {
  conn_info *tmp_node;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  tmp_node = _conn_info_list;
  while(tmp_node != NULL) {
    if(tmp_node->rem_world_id == wid) {
      break;
    }
    tmp_node = tmp_node->next;
  }
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
  if(tmp_node == NULL)
    return 0;
  else
    return (tmp_node->ref_count);
}

/**
 * This routine delete world (wid) from linked list of known world descriptors
 */
void MPIDI_delete_conn_record(int wid) {
  conn_info *tmp_node1, *tmp_node2;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  tmp_node1 = tmp_node2 = _conn_info_list;
  while(tmp_node1) {
    if(tmp_node1->rem_world_id == wid) {
      if(tmp_node1 == tmp_node2) {
        _conn_info_list = tmp_node1->next;
      }
      else {
        tmp_node2->next = tmp_node1->next;
      }
      if(tmp_node1->rem_taskids != NULL)
        MPIU_Free(tmp_node1->rem_taskids);
      MPIU_Free(tmp_node1);
      break;
    }
    tmp_node2 = tmp_node1;
    tmp_node1 = tmp_node1->next;
  }
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
}


int MPID_PG_BCast( MPID_Comm *peercomm_p, MPID_Comm *comm_p, int root )
{
    int n_local_pgs=0, mpi_errno = MPI_SUCCESS;
    pg_translation *local_translation = 0;
    pg_node *pg_list, *pg_next, *pg_head = 0;
    int rank, i, peer_comm_size;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIU_CHKLMEM_DECL(1);

    peer_comm_size = comm_p->local_size;
    rank            = comm_p->rank;

    local_translation = (pg_translation*)MPIU_Malloc(peer_comm_size*sizeof(pg_translation));
    if (rank == root) {
	/* Get the process groups known to the *peercomm* */
	MPIDI_ExtractLocalPGInfo( peercomm_p, local_translation, &pg_head,
			    &n_local_pgs );
    }

    /* Now, broadcast the number of local pgs */
    mpi_errno = MPIR_Bcast_impl( &n_local_pgs, 1, MPI_INT, root, comm_p, &errflag);

    pg_list = pg_head;
    for (i=0; i<n_local_pgs; i++) {
	int len, flag;
	char *pg_str=0;
	MPIDI_PG_t *pgptr;

	if (rank == root) {
	    if (!pg_list) {
		/* FIXME: Error, the pg_list is broken */
		printf( "Unexpected end of pg_list\n" ); fflush(stdout);
		break;
	    }
	    pg_str  = pg_list->str;
	    len     = pg_list->lenStr;
	    pg_list = pg_list->next;
	}
	mpi_errno = MPIR_Bcast_impl( &len, 1, MPI_INT, root, comm_p, &errflag);
	if (rank != root) {
	    pg_str = (char *)MPIU_Malloc(len);
	    if (!pg_str) {
		goto fn_exit;
	    }
	}
	mpi_errno = MPIR_Bcast_impl( pg_str, len, MPI_CHAR, root, comm_p, &errflag);
	if (mpi_errno) {
	    if (rank != root)
		MPIU_Free( pg_str );
	}

	if (rank != root) {
	    /* flag is true if the pg was created, false if it
	       already existed. This step
	       also initializes the created process group  */
	    MPIDI_PG_Create_from_string( pg_str, &pgptr, &flag );
	    if (flag) {
		/*printf( "[%d]Added pg named %s to list\n", rank,
			(char *)pgptr->id );
			fflush(stdout); */
	    }
	    MPIU_Free( pg_str );
	}
    }

    /* Free pg_list */
    pg_list = pg_head;

    /* FIXME: We should use the PG destroy function for this, and ensure that
       the PG fields are valid for that function */
    while (pg_list) {
	pg_next = pg_list->next;
	MPIU_Free( pg_list->str );
	if (pg_list->pg_id ) {
	    MPIU_Free( pg_list->pg_id );
	}
	MPIU_Free( pg_list );
	pg_list = pg_next;
    }

 fn_exit:
    MPIU_Free(local_translation);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
#endif
