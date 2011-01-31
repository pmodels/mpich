/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * This file replaces ch3u_comm_connect.c and ch3u_comm_accept.c .  These
 * routines need to be used together, particularly since they must exchange
 * information.  In addition, many of the steps that the take are identical,
 * such as building the new process group information after a connection.
 * By having these routines in the same file, it is easier for them
 * to share internal routines and it is easier to ensure that communication
 * between the two root processes (the connector and acceptor) are
 * consistent.
 */

/* FIXME: If dynamic processes are not supported, this file will contain
   no code and some compilers may warn about an "empty translation unit" */
#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS

/* FIXME: pg_translation is used for ? */
typedef struct pg_translation {
    int pg_index;    /* index of a process group (index in pg_node) */
    int pg_rank;     /* rank in that process group */
} pg_translation;


typedef struct pg_node {
    int  index;            /* Internal index of process group 
			      (see pg_translation) */
    char *pg_id;
    char *str;             /* String describing connection info for pg */
    int   lenStr;          /* Length of this string (including the null terminator(s)) */
    struct pg_node *next;
} pg_node;

/* These functions help implement the connect/accept algorithm */
static int ExtractLocalPGInfo( MPID_Comm *, pg_translation [], 
			       pg_node **, int * );
static int ReceivePGAndDistribute( MPID_Comm *, MPID_Comm *, int, int *,
				   int, MPIDI_PG_t *[] );
static int SendPGtoPeerAndFree( MPID_Comm *, int *, pg_node * );
static int FreeNewVC( MPIDI_VC_t *new_vc );
static int SetupNewIntercomm( MPID_Comm *comm_ptr, int remote_comm_size, 
			      pg_translation remote_translation[],
			      MPIDI_PG_t **remote_pg, 
			      MPID_Comm *intercomm );
static int MPIDI_CH3I_Initialize_tmp_comm(MPID_Comm **comm_pptr, 
					  MPIDI_VC_t *vc_ptr, int is_low_group, int context_id_offset);
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
 * (type MPIDI_CH3I_PKT_SC_CONN_ACCEPT) to the accepting process.
 * This packet contains a "port_tag_name", which is a value that
 * is used to separate different MPI port names (values from MPI_Open_port)
 * on the same process (this is a way to multiplex many MPI port names on 
 * a single communication connection port).
 *
 * At this point, the accepting process creates a virtual connection (VC)
 * for this connection, initializes it, sends a packet back with the type
 * MPIDI_CH3I_PKT_SC_OPEN_RESP.  In addition, the connection is saved in 
 * an accept queue with the port_tag_name.
 *
 * On the accepting side, the process waits until the progress engine
 * inserts the connect request into the accept queue (this is done with the
 * routine MPIDI_CH3I_Acceptq_dequeue).  This routine returns the matched
 * virtual connection (VC).
 *
 * Once both sides have established there VC, they both invoke
 * MPIDI_CH3I_Initialize_tmp_comm to create a temporary intercommunicator.
 * A temporary intercommunicator is constructed so that we can use
 * MPI routines to send the other information that we need to complete
 * the connect/accept operation (described below).
 *
 * The above is implemented with the routines
 *   MPIDI_Create_inter_root_communicator_connect
 *   MPIDI_Create_inter_root_communicator_accept
 *   MPIDI_CH3I_Initialize_tmp_comm
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

/* 
 * These next two routines are used to create a virtual connection
 * (VC) and a temporary intercommunicator that can be used to 
 * communicate between the two "root" processes for the 
 * connect and accept.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_Create_inter_root_communicator_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_Create_inter_root_communicator_connect(const char *port_name, 
							MPID_Comm **comm_pptr, 
							MPIDI_VC_t **vc_pptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *tmp_comm;
    MPIDI_VC_t *connect_vc = NULL;
    int port_name_tag;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_CONNECT);

    /* Connect to the root on the other side. Create a
       temporary intercommunicator between the two roots so that
       we can use MPI functions to communicate data between them. */

    mpi_errno = MPIU_CALL(MPIDI_CH3,Connect_to_root(port_name, &connect_vc));
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* extract the tag from the port_name */
    mpi_errno = MPIDI_GetTagFromPort( port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIDI_CH3I_Initialize_tmp_comm(&tmp_comm, connect_vc, 1, port_name_tag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    *comm_pptr = tmp_comm;
    *vc_pptr = connect_vc;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_CONNECT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Creates a communicator for the purpose of communicating with one other 
   process (the root of the other group).  It also returns the virtual
   connection */
#undef FUNCNAME
#define FUNCNAME MPIDI_Create_inter_root_communicator_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_Create_inter_root_communicator_accept(const char *port_name, 
						MPID_Comm **comm_pptr, 
						MPIDI_VC_t **vc_pptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *tmp_comm;
    MPIDI_VC_t *new_vc = NULL;
    MPID_Progress_state progress_state;
    int port_name_tag;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_ACCEPT);

    /* extract the tag from the port_name */
    mpi_errno = MPIDI_GetTagFromPort( port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* FIXME: Describe the algorithm used here, and what routine 
       is user on the other side of this connection */
    /* dequeue the accept queue to see if a connection with the
       root on the connect side has been formed in the progress
       engine (the connection is returned in the form of a vc). If
       not, poke the progress engine. */

    MPID_Progress_start(&progress_state);
    for(;;)
    {
	MPIDI_CH3I_Acceptq_dequeue(&new_vc, port_name_tag);
	if (new_vc != NULL)
	{
	    break;
	}

	mpi_errno = MPID_Progress_wait(&progress_state);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno)
	{
	    MPID_Progress_end(&progress_state);
	    MPIU_ERR_POP(mpi_errno);
	}
	/* --END ERROR HANDLING-- */
    }
    MPID_Progress_end(&progress_state);

    mpi_errno = MPIDI_CH3I_Initialize_tmp_comm(&tmp_comm, new_vc, 0, port_name_tag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    *comm_pptr = tmp_comm;
    *vc_pptr = new_vc;

    MPIU_DBG_MSG_FMT(CH3_CONNECT,VERBOSE,(MPIU_DBG_FDEST,
		  "new_vc=%p", new_vc));

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_ACCEPT);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/* This is a utility routine used to initialize temporary communicators
   used in connect/accept operations, and is only used in the above two 
   routines */
#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Initialize_tmp_comm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Initialize_tmp_comm(MPID_Comm **comm_pptr, 
					  MPIDI_VC_t *vc_ptr, int is_low_group, int context_id_offset)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *tmp_comm, *commself_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_INITIALIZE_TMP_COMM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_INITIALIZE_TMP_COMM);

    MPID_Comm_get_ptr( MPI_COMM_SELF, commself_ptr );

    /* WDG-old code allocated a context id that was then discarded */
    mpi_errno = MPIR_Comm_create(&tmp_comm);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
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
    MPIU_Assert(tmp_comm->context_id     != MPIR_INVALID_CONTEXT_ID);
    MPIU_Assert(tmp_comm->recvcontext_id != MPIR_INVALID_CONTEXT_ID);

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
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_vcrt");
    }
    mpi_errno = MPID_VCRT_Get_ptr(tmp_comm->vcrt, &tmp_comm->vcr);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_getptr");
    }

    /* FIXME: Why do we do a dup here? */
    MPID_VCR_Dup(vc_ptr, tmp_comm->vcr);

    *comm_pptr = tmp_comm;

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_INITIALIZE_TMP_COMM);
    return mpi_errno;
fn_fail:
    goto fn_exit;
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

#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Comm_connect(const char *port_name, MPID_Info *info, int root, 
		       MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
    int mpi_errno=MPI_SUCCESS;
    int j, i, rank, recv_ints[3], send_ints[3], context_id;
    int remote_comm_size=0;
    MPID_Comm *tmp_comm = NULL;
    MPIDI_VC_t *new_vc = NULL;
    int sendtag=100, recvtag=100, n_remote_pgs;
    int n_local_pgs=1, local_comm_size;
    pg_translation *local_translation = NULL, *remote_translation = NULL;
    pg_node *pg_list = NULL;
    MPIDI_PG_t **remote_pg = NULL;
    MPIR_Context_id_t recvcontext_id = MPIR_INVALID_CONTEXT_ID;
    int errflag = FALSE;
    MPIU_CHKLMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_COMM_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_COMM_CONNECT);

    /* Get the context ID here because we need to send it to the remote side */
    mpi_errno = MPIR_Get_contextid( comm_ptr, &recvcontext_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    rank = comm_ptr->rank;
    local_comm_size = comm_ptr->local_size;

    if (rank == root)
    {
	/* Establish a communicator to communicate with the root on the 
	   other side. */
	mpi_errno = MPIDI_Create_inter_root_communicator_connect(
	    port_name, &tmp_comm, &new_vc);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP_LABEL(mpi_errno, no_port);
	}

	/* Make an array to translate local ranks to process group index 
	   and rank */
	MPIU_CHKLMEM_MALLOC(local_translation,pg_translation*,
			    local_comm_size*sizeof(pg_translation),
			    mpi_errno,"local_translation");

	/* Make a list of the local communicator's process groups and encode 
	   them in strings to be sent to the other side.
	   The encoded string for each process group contains the process 
	   group id, size and all its KVS values */
	mpi_errno = ExtractLocalPGInfo( comm_ptr, local_translation, 
					&pg_list, &n_local_pgs );
        MPIU_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Can't extract local PG info.");

	/* Send the remote root: n_local_pgs, local_comm_size,
           Recv from the remote root: n_remote_pgs, remote_comm_size,
           recvcontext_id for newcomm */

        send_ints[0] = n_local_pgs;
        send_ints[1] = local_comm_size;
        send_ints[2] = recvcontext_id;

	MPIU_DBG_MSG_FMT(CH3_CONNECT,VERBOSE,(MPIU_DBG_FDEST,
		  "sending 3 ints, %d, %d and %d, and receiving 3 ints", 
                  send_ints[0], send_ints[1], send_ints[2]));
        mpi_errno = MPIC_Sendrecv(send_ints, 3, MPI_INT, 0,
                                  sendtag++, recv_ints, 3, MPI_INT,
                                  0, recvtag++, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) {
            /* this is a no_port error because we may fail to connect
               on the send if the port name is invalid */
	    MPIU_ERR_POP_LABEL(mpi_errno, no_port);
	}
    }

    /* broadcast the received info to local processes */
    MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"broadcasting the received 3 ints");
    mpi_errno = MPIR_Bcast_intra(recv_ints, 3, MPI_INT, root, comm_ptr, &errflag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    /* check if root was unable to connect to the port */
    MPIU_ERR_CHKANDJUMP1(recv_ints[0] == -1, mpi_errno, MPI_ERR_PORT, "**portexist", "**portexist %s", port_name);
    
    n_remote_pgs     = recv_ints[0];
    remote_comm_size = recv_ints[1];
    context_id	     = recv_ints[2];

    MPIU_CHKLMEM_MALLOC(remote_pg,MPIDI_PG_t**,
			n_remote_pgs * sizeof(MPIDI_PG_t*),
			mpi_errno,"remote_pg");
    MPIU_CHKLMEM_MALLOC(remote_translation,pg_translation*,
			remote_comm_size * sizeof(pg_translation),
			mpi_errno,"remote_translation");
    MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"allocated remote process groups");

    /* Exchange the process groups and their corresponding KVSes */
    if (rank == root)
    {
	mpi_errno = SendPGtoPeerAndFree( tmp_comm, &sendtag, pg_list );
	mpi_errno = ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					n_remote_pgs, remote_pg );
	/* Receive the translations from remote process rank to process group 
	   index */
	MPIU_DBG_MSG_FMT(CH3_CONNECT,VERBOSE,(MPIU_DBG_FDEST,
               "sending %d ints, receiving %d ints", 
	      local_comm_size * 2, remote_comm_size * 2));
	mpi_errno = MPIC_Sendrecv(local_translation, local_comm_size * 2, 
				  MPI_INT, 0, sendtag++,
				  remote_translation, remote_comm_size * 2, 
				  MPI_INT, 0, recvtag++, tmp_comm->handle, 
				  MPI_STATUS_IGNORE);
	if (mpi_errno) {
	    MPIU_ERR_POP(mpi_errno);
	}

#ifdef MPICH_DBG_OUTPUT
	MPIU_DBG_PRINTF(("[%d]connect:Received remote_translation:\n", rank));
	for (i=0; i<remote_comm_size; i++)
	{
	    MPIU_DBG_PRINTF((" remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
		i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank));
	}
#endif
    }
    else
    {
	mpi_errno = ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					    n_remote_pgs, remote_pg );
    }

    /* Broadcast out the remote rank translation array */
    MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"Broadcasting remote translation");
    mpi_errno = MPIR_Bcast_intra(remote_translation, remote_comm_size * 2, MPI_INT,
                                 root, comm_ptr, &errflag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

#ifdef MPICH_DBG_OUTPUT
    MPIU_DBG_PRINTF(("[%d]connect:Received remote_translation after broadcast:\n", rank));
    for (i=0; i<remote_comm_size; i++)
    {
	MPIU_DBG_PRINTF((" remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
	    i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank));
    }
#endif

    mpi_errno = MPIR_Comm_create(newcomm);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    (*newcomm)->context_id     = context_id;
    (*newcomm)->recvcontext_id = recvcontext_id;
    (*newcomm)->is_low_group   = 1;

    mpi_errno = SetupNewIntercomm( comm_ptr, remote_comm_size, 
				   remote_translation, remote_pg, *newcomm );
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* synchronize with remote root */
    if (rank == root)
    {
	MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"sync with peer");
        mpi_errno = MPIC_Sendrecv(&i, 0, MPI_INT, 0,
                                  sendtag++, &j, 0, MPI_INT,
                                  0, recvtag++, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
        }

        /* All communication with remote root done. Release the communicator. */
        MPIR_Comm_release(tmp_comm,0);
    }

    /*printf("connect:barrier\n");fflush(stdout);*/
    mpi_errno = MPIR_Barrier_intra(comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* Free new_vc. It was explicitly allocated in MPIDI_CH3_Connect_to_root.*/
    if (rank == root) {
	FreeNewVC( new_vc );
    }

 fn_exit: 
    MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"Exiting ch3u_comm_connect");
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_COMM_CONNECT);
    return mpi_errno;
 fn_fail:
    {
        int mpi_errno2 = MPI_SUCCESS;
        if (new_vc) {
	    mpi_errno2 = MPIU_CALL(MPIDI_CH3,VC_Destroy(new_vc));
            if (mpi_errno2) MPIU_ERR_SET(mpi_errno2, MPI_ERR_OTHER, "**fail");
        }

        if (recvcontext_id != MPIR_INVALID_CONTEXT_ID)
            MPIR_Free_contextid(recvcontext_id);
        
        if (mpi_errno2) MPIU_ERR_ADD(mpi_errno, mpi_errno2);

        goto fn_exit;
    }
 no_port:
    {
        int mpi_errno2 = MPI_SUCCESS;

       /* broadcast error notification to other processes */
        MPIU_Assert(rank == root);
        recv_ints[0] = -1;
        recv_ints[1] = -1;
        recv_ints[2] = -1;
        MPIU_ERR_SET1(mpi_errno, MPI_ERR_PORT, "**portexist", "**portexist %s", port_name);

        /* notify other processes to return an error */
        MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"broadcasting 3 ints: error case");
        mpi_errno2 = MPIR_Bcast_intra(recv_ints, 3, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno2) MPIU_ERR_ADD(mpi_errno, mpi_errno2);
        if (errflag) {
            MPIU_ERR_SET(mpi_errno2, MPI_ERR_OTHER, "**coll_fail");
            MPIU_ERR_ADD(mpi_errno, mpi_errno2);
        }
        goto fn_fail;
    }
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
#undef FUNCNAME
#define FUNCNAME ExtractLocalPGInfo
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ExtractLocalPGInfo( MPID_Comm *comm_p, 
			       pg_translation local_translation[], 
			       pg_node **pg_list_p,
			       int *n_local_pgs_p )
{
    pg_node        *pg_list = 0, *pg_iter, *pg_trailer;
    int            i, cur_index = 0, local_comm_size, mpi_errno = 0;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_EXTRACTLOCALPGINFO);

    MPIDI_FUNC_ENTER(MPID_STATE_EXTRACTLOCALPGINFO);

    /* If we are in the case of singleton-init, we may need to reset the
       id string for comm world.  We do this before doing anything else */
    MPIDI_PG_CheckForSingleton();

    local_comm_size = comm_p->local_size;

    /* Make a list of the local communicator's process groups and encode 
       them in strings to be sent to the other side.
       The encoded string for each process group contains the process 
       group id, size and all its KVS values */
    
    cur_index = 0;
    MPIU_CHKPMEM_MALLOC(pg_list,pg_node*,sizeof(pg_node),mpi_errno,
			"pg_list");
    
    pg_list->pg_id = MPIU_Strdup(comm_p->vcr[0]->pg->id);
    pg_list->index = cur_index++;
    pg_list->next = NULL;
    /* XXX DJG FIXME-MT should we be checking this?  the add/release macros already check this */
    MPIU_Assert( MPIU_Object_get_ref(comm_p->vcr[0]->pg));
    mpi_errno = MPIDI_PG_To_string(comm_p->vcr[0]->pg, &pg_list->str, 
				   &pg_list->lenStr );
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    MPIU_DBG_STMT(CH3_CONNECT,VERBOSE,MPIDI_PrintConnStr(__FILE__,__LINE__,"PG as string is", pg_list->str ));
    local_translation[0].pg_index = 0;
    local_translation[0].pg_rank = comm_p->vcr[0]->pg_rank;
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
	    if (!pg_iter) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    pg_iter->pg_id = MPIU_Strdup(comm_p->vcr[i]->pg->id);
	    pg_iter->index = cur_index++;
	    pg_iter->next = NULL;
	    mpi_errno = MPIDI_PG_To_string(comm_p->vcr[i]->pg, &pg_iter->str,
					   &pg_iter->lenStr );
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    local_translation[i].pg_index = pg_iter->index;
	    local_translation[i].pg_rank = comm_p->vcr[i]->pg_rank;
	    pg_trailer->next = pg_iter;
	}
    }

    *n_local_pgs_p = cur_index;
    *pg_list_p     = pg_list;
    
#ifdef MPICH_DBG_OUTPUT
    pg_iter = pg_list;
    while (pg_iter != NULL) {
	MPIU_DBG_PRINTF(("connect:PG: '%s'\n<%s>\n", pg_iter->pg_id, pg_iter->str));
	pg_iter = pg_iter->next;
    }
#endif


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_EXTRACTLOCALPGINFO);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


/* The root process in comm_ptr receives strings describing the
   process groups and then distributes them to the other processes
   in comm_ptr.
   See SendPGToPeer for the routine that sends the descriptions */
#undef FUNCNAME
#define FUNCNAME ReceivePGAndDistribute
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ReceivePGAndDistribute( MPID_Comm *tmp_comm, MPID_Comm *comm_ptr, 
				   int root, int *recvtag_p, 
				   int n_remote_pgs, MPIDI_PG_t *remote_pg[] )
{
    char *pg_str = 0;
    int  i, j, flag;
    int  rank = comm_ptr->rank;
    int  mpi_errno = 0;
    int  recvtag = *recvtag_p;
    int errflag = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_RECEIVEPGANDDISTRIBUTE);

    MPIDI_FUNC_ENTER(MPID_STATE_RECEIVEPGANDDISTRIBUTE);

    for (i=0; i<n_remote_pgs; i++) {

	if (rank == root) {
	    /* First, receive the pg description from the partner */
	    mpi_errno = MPIC_Recv(&j, 1, MPI_INT, 0, recvtag++, 
				  tmp_comm->handle, MPI_STATUS_IGNORE);
	    *recvtag_p = recvtag;
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    pg_str = (char*)MPIU_Malloc(j);
	    if (pg_str == NULL) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    mpi_errno = MPIC_Recv(pg_str, j, MPI_CHAR, 0, recvtag++, 
				  tmp_comm->handle, MPI_STATUS_IGNORE);
	    *recvtag_p = recvtag;
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	}

	/* Broadcast the size and data to the local communicator */
	/*printf("accept:broadcasting 1 int\n");fflush(stdout);*/
	mpi_errno = MPIR_Bcast_intra(&j, 1, MPI_INT, root, comm_ptr, &errflag);
	if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

	if (rank != root) {
	    /* The root has already allocated this string */
	    pg_str = (char*)MPIU_Malloc(j);
	    if (pg_str == NULL) {
		MPIU_ERR_POP(mpi_errno);
	    }
	}
	/*printf("accept:broadcasting string of length %d\n", j);fflush(stdout);*/
	mpi_errno = MPIR_Bcast_intra(pg_str, j, MPI_CHAR, root, comm_ptr, &errflag);
	if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	/* Then reconstruct the received process group.  This step
	   also initializes the created process group */

	MPIU_DBG_STMT(CH3_CONNECT,VERBOSE,MPIDI_PrintConnStr(__FILE__,__LINE__,"Creating pg from string", pg_str ));
	mpi_errno = MPIDI_PG_Create_from_string(pg_str, &remote_pg[i], &flag);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
	
	MPIU_Free(pg_str);
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_RECEIVEPGANDDISTRIBUTE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Used internally to broadcast process groups belonging to peercomm to
 all processes in comm.  The process with rank root in comm is the 
 process in peercomm from which the process groups are taken. This routine 
 is collective over comm_p . */
#undef FUNCNAME
#define FUNCNAME MPID_PG_BCast
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_PG_BCast( MPID_Comm *peercomm_p, MPID_Comm *comm_p, int root )
{
    int n_local_pgs=0, mpi_errno = MPI_SUCCESS;
    pg_translation *local_translation = 0;
    pg_node *pg_list, *pg_next, *pg_head = 0;
    int rank, i, peer_comm_size;
    int errflag = FALSE;
    MPIU_CHKLMEM_DECL(1);

    peer_comm_size = comm_p->local_size;
    rank            = comm_p->rank;

    MPIU_CHKLMEM_MALLOC(local_translation,pg_translation*,
			peer_comm_size*sizeof(pg_translation),
			mpi_errno,"local_translation");
    
    if (rank == root) {
	/* Get the process groups known to the *peercomm* */
	ExtractLocalPGInfo( peercomm_p, local_translation, &pg_head, 
			    &n_local_pgs );
    }

    /* Now, broadcast the number of local pgs */
    mpi_errno = MPIR_Bcast_impl( &n_local_pgs, 1, MPI_INT, root, comm_p, &errflag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

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
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	if (rank != root) {
	    pg_str = (char *)MPIU_Malloc(len);
            if (!pg_str) {
                MPIU_CHKMEM_SETERR(mpi_errno, len, "pg_str");
                goto fn_exit;
            }
	}
	mpi_errno = MPIR_Bcast_impl( pg_str, len, MPI_CHAR, root, comm_p, &errflag);
        if (mpi_errno) {
            if (rank != root)
                MPIU_Free( pg_str );
            MPIU_ERR_POP(mpi_errno);
        }
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

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
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Sends the process group information to the peer and frees the 
   pg_list */
#undef FUNCNAME
#define FUNCNAME SendPGtoPeerAndFree
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int SendPGtoPeerAndFree( MPID_Comm *tmp_comm, int *sendtag_p, 
				pg_node *pg_list )
{
    int mpi_errno = 0;
    int sendtag = *sendtag_p, i;
    pg_node *pg_iter;
    MPIDI_STATE_DECL(MPID_STATE_SENDPGTOPEERANDFREE);

    MPIDI_FUNC_ENTER(MPID_STATE_SENDPGTOPEERANDFREE);

    while (pg_list != NULL) {
	pg_iter = pg_list;
	i = pg_iter->lenStr;
	/*printf("connect:sending 1 int: %d\n", i);fflush(stdout);*/
	mpi_errno = MPIC_Send(&i, 1, MPI_INT, 0, sendtag++, tmp_comm->handle);
	*sendtag_p = sendtag;
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
	
	/* printf("connect:sending string length %d\n", i);fflush(stdout); */
	mpi_errno = MPIC_Send(pg_iter->str, i, MPI_CHAR, 0, sendtag++, 
			      tmp_comm->handle);
	*sendtag_p = sendtag;
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
	
	pg_list = pg_list->next;
	MPIU_Free(pg_iter->str);
	MPIU_Free(pg_iter->pg_id);
	MPIU_Free(pg_iter);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SENDPGTOPEERANDFREE);
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
#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Comm_accept(const char *port_name, MPID_Info *info, int root, 
		      MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
    int mpi_errno=MPI_SUCCESS;
    int i, j, rank, recv_ints[3], send_ints[3], context_id;
    int remote_comm_size=0;
    MPID_Comm *tmp_comm = NULL, *intercomm;
    MPIDI_VC_t *new_vc = NULL;
    int sendtag=100, recvtag=100, local_comm_size;
    int n_local_pgs=1, n_remote_pgs;
    pg_translation *local_translation = NULL, *remote_translation = NULL;
    pg_node *pg_list = NULL;
    MPIDI_PG_t **remote_pg = NULL;
    int errflag = FALSE;
    MPIU_CHKLMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_COMM_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_COMM_ACCEPT);

    /* Create the new intercommunicator here. We need to send the
       context id to the other side. */
    mpi_errno = MPIR_Comm_create(newcomm);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    mpi_errno = MPIR_Get_contextid( comm_ptr, &(*newcomm)->recvcontext_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    /* FIXME why is this commented out? */
    /*    (*newcomm)->context_id = (*newcomm)->recvcontext_id; */
    
    rank = comm_ptr->rank;
    local_comm_size = comm_ptr->local_size;
    
    if (rank == root)
    {
	/* Establish a communicator to communicate with the root on the 
	   other side. */
	mpi_errno = MPIDI_Create_inter_root_communicator_accept(port_name, 
						&tmp_comm, &new_vc);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}

	/* Make an array to translate local ranks to process group index and 
	   rank */
	MPIU_CHKLMEM_MALLOC(local_translation,pg_translation*,
			    local_comm_size*sizeof(pg_translation),
			    mpi_errno,"local_translation");

	/* Make a list of the local communicator's process groups and encode 
	   them in strings to be sent to the other side.
	   The encoded string for each process group contains the process 
	   group id, size and all its KVS values */
	mpi_errno = ExtractLocalPGInfo( comm_ptr, local_translation, 
					&pg_list, &n_local_pgs );
        /* Send the remote root: n_local_pgs, local_comm_size, context_id for 
	   newcomm.
           Recv from the remote root: n_remote_pgs, remote_comm_size */

        send_ints[0] = n_local_pgs;
        send_ints[1] = local_comm_size;
        send_ints[2] = (*newcomm)->recvcontext_id;

	/*printf("accept:sending 3 ints, %d, %d, %d, and receiving 2 ints\n", send_ints[0], send_ints[1], send_ints[2]);fflush(stdout);*/
        mpi_errno = MPIC_Sendrecv(send_ints, 3, MPI_INT, 0,
                                  sendtag++, recv_ints, 3, MPI_INT,
                                  0, recvtag++, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }

    /* broadcast the received info to local processes */
    /*printf("accept:broadcasting 2 ints - %d and %d\n", recv_ints[0], recv_ints[1]);fflush(stdout);*/
    mpi_errno = MPIR_Bcast_intra(recv_ints, 3, MPI_INT, root, comm_ptr, &errflag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");


    n_remote_pgs     = recv_ints[0];
    remote_comm_size = recv_ints[1];
    context_id       = recv_ints[2];
    MPIU_CHKLMEM_MALLOC(remote_pg,MPIDI_PG_t**,
			n_remote_pgs * sizeof(MPIDI_PG_t*),
			mpi_errno,"remote_pg");
    MPIU_CHKLMEM_MALLOC(remote_translation,pg_translation*,
			remote_comm_size * sizeof(pg_translation),
			mpi_errno, "remote_translation");
    MPIU_DBG_PRINTF(("[%d]accept:remote process groups: %d\nremote comm size: %d\n", rank, n_remote_pgs, remote_comm_size));

    /* Exchange the process groups and their corresponding KVSes */
    if (rank == root)
    {
	/* The root receives the PG from the peer (in tmp_comm) and
	   distributes them to the processes in comm_ptr */
	mpi_errno = ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					n_remote_pgs, remote_pg );
	
	mpi_errno = SendPGtoPeerAndFree( tmp_comm, &sendtag, pg_list );

	/* Receive the translations from remote process rank to process group index */
	/*printf("accept:sending %d ints and receiving %d ints\n", local_comm_size * 2, remote_comm_size * 2);fflush(stdout);*/
	mpi_errno = MPIC_Sendrecv(local_translation, local_comm_size * 2, 
				  MPI_INT, 0, sendtag++,
				  remote_translation, remote_comm_size * 2, 
				  MPI_INT, 0, recvtag++, tmp_comm->handle, 
				  MPI_STATUS_IGNORE);
#ifdef MPICH_DBG_OUTPUT
	MPIU_DBG_PRINTF(("[%d]accept:Received remote_translation:\n", rank));
	for (i=0; i<remote_comm_size; i++)
	{
	    MPIU_DBG_PRINTF((" remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
		i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank));
	}
#endif
    }
    else
    {
	mpi_errno = ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					    n_remote_pgs, remote_pg );
    }

    /* Broadcast out the remote rank translation array */
    MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"Broadcast remote_translation");
    mpi_errno = MPIR_Bcast_intra(remote_translation, remote_comm_size * 2, MPI_INT, 
                                 root, comm_ptr, &errflag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
#ifdef MPICH_DBG_OUTPUT
    MPIU_DBG_PRINTF(("[%d]accept:Received remote_translation after broadcast:\n", rank));
    for (i=0; i<remote_comm_size; i++)
    {
	MPIU_DBG_PRINTF((" remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
	    i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank));
    }
#endif


    /* Now fill in newcomm */
    intercomm               = *newcomm;
    intercomm->context_id   = context_id;
    intercomm->is_low_group = 0;

    mpi_errno = SetupNewIntercomm( comm_ptr, remote_comm_size, 
				   remote_translation, remote_pg, intercomm );
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* synchronize with remote root */
    if (rank == root)
    {
	MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"sync with peer");
        mpi_errno = MPIC_Sendrecv(&i, 0, MPI_INT, 0,
                                  sendtag++, &j, 0, MPI_INT,
                                  0, recvtag++, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
        }

        /* All communication with remote root done. Release the communicator. */
        MPIR_Comm_release(tmp_comm,0);
    }

    MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"Barrier");
    mpi_errno = MPIR_Barrier_intra(comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* Free new_vc once the connection is completed. It was explicitly 
       allocated in ch3_progress.c and returned by 
       MPIDI_CH3I_Acceptq_dequeue. */
    if (rank == root) {
	FreeNewVC( new_vc );
    }

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_COMM_ACCEPT);
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
#undef FUNCNAME
#define FUNCNAME SetupNewIntercomm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int SetupNewIntercomm( MPID_Comm *comm_ptr, int remote_comm_size, 
			      pg_translation remote_translation[],
			      MPIDI_PG_t **remote_pg, 
			      MPID_Comm *intercomm )
{
    int mpi_errno = MPI_SUCCESS, i;
    int errflag = FALSE;
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

    /* Point local vcr, vcrt at those of incoming intracommunicator */
    intercomm->local_vcrt = comm_ptr->vcrt;
    MPID_VCRT_Add_ref(comm_ptr->vcrt);
    intercomm->local_vcr  = comm_ptr->vcr;

    /* Set up VC reference table */
    mpi_errno = MPID_VCRT_Create(intercomm->remote_size, &intercomm->vcrt);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_vcrt");
    }
    mpi_errno = MPID_VCRT_Get_ptr(intercomm->vcrt, &intercomm->vcr);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_getptr");
    }
    for (i=0; i < intercomm->remote_size; i++) {
	MPIDI_PG_Dup_vcr(remote_pg[remote_translation[i].pg_index], 
			 remote_translation[i].pg_rank, &intercomm->vcr[i]);
    }

    MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"Barrier");
    mpi_errno = MPIR_Barrier_intra(comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;

 fn_fail:
    goto fn_exit;
}

/* Free new_vc. It was explicitly allocated in MPIDI_CH3_Connect_to_root. */
/* FIXME: The free and the create routines should be in the same file */
#undef FUNCNAME
#define FUNCNAME FreeNewVC
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int FreeNewVC( MPIDI_VC_t *new_vc )
{
    MPID_Progress_state progress_state;
    int mpi_errno = MPI_SUCCESS;
    
    if (new_vc->state != MPIDI_VC_STATE_INACTIVE) {
	/* If the new_vc isn't done, run the progress engine until
	   the state of the new vc is complete */
	MPID_Progress_start(&progress_state);
	while (new_vc->state != MPIDI_VC_STATE_INACTIVE) {
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
                {
                    MPID_Progress_end(&progress_state);
		    MPIU_ERR_POP(mpi_errno);
                }
	    /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&progress_state);
    }

    MPIU_CALL(MPIDI_CH3,VC_Destroy(new_vc));
    MPIU_Free(new_vc);

 fn_fail:
    return mpi_errno;
}

/* ------------------------------------------------------------------------- */
/*
 * 
 */

/* FIXME: What is an Accept queue and who uses it?  
   Is this part of the connect/accept support?  
   These routines appear to be called by channel progress routines; 
   perhaps this belongs in util/sock (note the use of a port_name_tag in the 
   dequeue code, though this could be any string).

   Are the locks required?  If this is only called within the progress
   engine, then the progress engine locks should be sufficient.  If a
   finer grain lock model is used, it needs to be very carefully 
   designed and documented.
*/

typedef struct MPIDI_CH3I_Acceptq_s
{
    struct MPIDI_VC *vc;
    int             port_name_tag;
    struct MPIDI_CH3I_Acceptq_s *next;
}
MPIDI_CH3I_Acceptq_t;

static MPIDI_CH3I_Acceptq_t * acceptq_head=0;
static int maxAcceptQueueSize = 0;
static int AcceptQueueSize    = 0;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Acceptq_enqueue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Acceptq_enqueue(MPIDI_VC_t * vc, int port_name_tag )
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_CH3I_Acceptq_t *q_item;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);

    /* FIXME: Use CHKPMEM */
    q_item = (MPIDI_CH3I_Acceptq_t *)
        MPIU_Malloc(sizeof(MPIDI_CH3I_Acceptq_t)); 
    /* --BEGIN ERROR HANDLING-- */
    if (q_item == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIDI_CH3I_Acceptq_t" );
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    q_item->vc		  = vc;
    q_item->port_name_tag = port_name_tag;

    /* Keep some statistics on the accept queue */
    AcceptQueueSize++;
    if (AcceptQueueSize > maxAcceptQueueSize) 
	maxAcceptQueueSize = AcceptQueueSize;

    /* FIXME: Stack or queue? */
    MPIU_DBG_MSG_P(CH3_CONNECT,TYPICAL,"vc=%p:Enqueuing accept connection",vc);
    q_item->next = acceptq_head;
    acceptq_head = q_item;
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);
    return mpi_errno;
}


/* Attempt to dequeue a vc from the accept queue. If the queue is
   empty or the port_name_tag doesn't match, return a NULL vc. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Acceptq_dequeue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Acceptq_dequeue(MPIDI_VC_t ** vc, int port_name_tag)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_CH3I_Acceptq_t *q_item, *prev;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);

    *vc = NULL;
    q_item = acceptq_head;
    prev = q_item;

    while (q_item != NULL)
    {
	if (q_item->port_name_tag == port_name_tag)
	{
	    *vc = q_item->vc;

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
    
    mpi_errno = MPIDI_CH3_Complete_Acceptq_dequeue(*vc);

    MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,
	      (MPIU_DBG_FDEST,"vc=%p:Dequeuing accept connection with tag %d",
	       *vc,port_name_tag));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);
    return mpi_errno;
}

#else  /* MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS is defined */

#endif /* MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS */

