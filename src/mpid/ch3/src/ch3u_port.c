/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpid_port.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH3_COMM_CONNECT_TIMEOUT
      category    : CH3
      type        : int
      default     : 180
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP_EQ
      description : >-
        The default time out period in seconds for a connection attempt to the
        server communicator where the named port exists but no pending accept.
        User can change the value for a specified connection through its info
        argument.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

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
static int ExtractLocalPGInfo( MPIR_Comm *, pg_translation [],
			       pg_node **, int * );
static int ReceivePGAndDistribute( MPIR_Comm *, MPIR_Comm *, int, int *,
				   int, MPIDI_PG_t *[] );
static int SendPGtoPeerAndFree( MPIR_Comm *, int *, pg_node * );
static int FreeNewVC( MPIDI_VC_t *new_vc );
static int SetupNewIntercomm( MPIR_Comm *comm_ptr, int remote_comm_size,
			      pg_translation remote_translation[],
			      MPIDI_PG_t **remote_pg, 
			      MPIR_Comm *intercomm );
static int MPIDI_CH3I_Initialize_tmp_comm(MPIR_Comm **comm_pptr,
					  MPIDI_VC_t *vc_ptr, int is_low_group, int context_id_offset);

static int MPIDI_CH3I_Acceptq_dequeue(MPIDI_CH3I_Port_connreq_t ** connreq_ptr, int port_name_tag);
static int MPIDI_CH3I_Acceptq_cleanup(MPIDI_CH3I_Port_connreq_q_t * accept_connreq_q);

static int MPIDI_CH3I_Revokeq_cleanup(void);

static int MPIDI_CH3I_Port_connreq_create(MPIDI_VC_t * vc,
                                          MPIDI_CH3I_Port_connreq_t ** connreq_ptr);
static int MPIDI_CH3I_Port_connreq_free(MPIDI_CH3I_Port_connreq_t * connreq);


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
 * -----------------------------------------------------------------------------
 * To support connection timeout, we add additional handshake protocol in above
 * VC establishing step. The implementation includes init/destroy processing in
 * port open/close routine and MPI_Finalize, packet handlers, and modified code
 * in following subroutines:
 *   - MPIDI_Create_inter_root_communicator_connect
 *   - and MPIDI_Create_inter_root_communicator_accept
 *
 * 1. Every connection attempt is described as an *request object* on both sides,
 *    with following state change (see MPIDI_CH3I_Port_connreq_stat_t).
 *    - Connecting side: INITED -> REVOKE / ACCEPTED / ERR_CLOSE -> FREE
 *    - Accepting side : INITED -> ACCEPT -> (ACCEPTED) -> FREE
 *
 * 2. Handshake protocol:
 *    I. Connecting side:
 *      a. After VC is created, a connection *request object* is created with
 *         INITED state, then we pokes progress to wait the state being changed
 *         in specified timeout period (default CVAR).
 *      b. If waiting time exceeded threshold, set state to REVOKE.
 *      c. Progress engine receives a MPIDI_CH3_PKT_CONN_ACK packet indicating
 *         whether the other side could accept the request.
 *         - ACK (TRUE): if request state is INITED, then set to ACCEPTED and
 *           reply a MPIDI_CH3_PKT_ACCEPT_ACK packet with TRUE ACK; otherwise
 *           such request is already being revoked, then set to FREE and reply
 *           packet with FALSE ACK.
 *         - ACK (FALSE): if request state is INITED, then set to ERR_CLOSE;
 *           otherwise set to FREE (see MPIDI_CH3_PktHandler_ConnAck).
 *      d. If the process is still waiting, then it checks the changed state:
 *         if request is ACCEPTED, then continue connection; otherwise report
 *         MPI_ERR_PORT -- unexpected port closing.
 *
 *    II. Accepting side:
 *      a. If port exists, the new VC is enqueued into the accept_queue and
 *         described as a *request object* with INITED state; otherwise, reply a
 *         MPIDI_CH3_PKT_CONN_ACK packet with FALSE ACK (closed port)
 *         (see MPIDI_CH3I_Acceptq_dequeue routine).
 *      b. After VC is dequeued in accept routine, send a MPIDI_CH3_PKT_CONN_ACK
 *         packet with TRUE ACK to connecting side and change state to ACCEPT.
 *         Then we poll progress to wait for state change.
 *      c. Progress engine receives a MPIDI_CH3_PKT_ACCEPT_ACK packet indicating
 *         whether the other side can match the acceptance (not being revoked).
 *         - ACK (TRUE): change request state to ACCEPTED
 *         - ACK (FALSE): set state to FREE
 *         (see MPIDI_CH3_PktHandler_AcceptAck).
 *      d. In accept routine, process checks the changed state. If it became
 *         ACCEPTED, then finish acceptance; otherwise free this request and VC
 *         and then wait for next coming request.
 *
 * 3. Resource cleanup. In case of timed out connection, following user code
 *    shall be considered as correct program (no description in standard yet).
 *    Thus the resource of first connect must be cleaned up before exit.
 *              ================================================
 *              Server:               Client:
 *              open_port();          connect(); ** timed out **
 *              accept();             connect();
 *              close_port();
 *              ================================================
 *    (* The most critical part is closing VC because of closing synchronization
 *    with the other side (see note in MPIDI_CH3I_Port_local_close_vc and
 *    ch3u_handle_connection.c), VC object cannot be freed before it became
 *    INACTIVE state, otherwise segfault !)
 *
 *    Here is the design to ensure resource cleanup. It also covers most
 *    incorrect user code (e.g., no accept, no close_port), except no finalize.
 *    I. Connecting side :
 *      (use revoked_connreq_q)
 *      a. A REVOKE state request is enqueued to revoked_connreq_q (in 2-I-b).
 *      b. If request changed state INITED->ERR_CLOSE in 2-I-c, we free such
 *         request in the connect call before return (step 2-I-d).
 *      c. If REVOKE->FREE in step 2-I-c, we start VC closing in packet handler,
 *         and free request and VC at finalize (see MPIDI_CH3I_Revokeq_cleanup).
 *      d. For any REVOKE state request at finalize, both request object and VC
 *         will be eventually freed once the other side started finalize.
 *
 *    II. Accepting side :
 *      (use port_queue, port->accept_queue, global unexpected_queue)
 *      a. We create a port object for every opened port and each of them holds
 *         a separate accept queue (to ensure no mess in multiple-ports).
 *      b. If request becomes FREE in 2-II-a, we start VC closing there and
 *         enqueue it to global unexpected_queue.
 *      c. If request updated state ACCEPT->FREE in 2-II-c, we start VC closing
 *         in packet handler and free both request and VC in accept call (2-II-d).
 *      d. For any requests still in port->accept_queue at close_port or finalize,
 *         issue MPIDI_CH3_PKT_CONN_ACK packet with FALSE ACK to the other side,
 *         then blocking free both VC and request object there. It also ensures
 *         3-I-d (see MPIDI_CH3I_Acceptq_cleanup).
 *      e. For any requests in global unexpected_queue, we blocking free both VC
 *         and request at finalize (see MPIDI_Port_finalize).
 *
 * - END of connection timeout support.
 * -----------------------------------------------------------------------------
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


/*** Queues for supporting connection timeout ***/

/* Server side queues
 *  - unexpected connection requests queue (global)
 *  - active port queue */
static MPIDI_CH3I_Port_connreq_q_t unexpt_connreq_q = {NULL, NULL, 0};
static MPIDI_CH3I_Port_q_t active_portq = {NULL, NULL, 0};

/* Client side queue
 * - revoked connection requests (i.e., timeout) */
static MPIDI_CH3I_Port_connreq_q_t revoked_connreq_q = {NULL, NULL, 0};


/*
 * These next two routines are used to create a virtual connection
 * (VC) and a temporary intercommunicator that can be used to 
 * communicate between the two "root" processes for the 
 * connect and accept.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_Create_inter_root_communicator_connect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_Create_inter_root_communicator_connect(const char *port_name, 
							int timeout, MPIR_Comm **comm_pptr,
							MPIDI_VC_t **vc_pptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *tmp_comm;
    MPIDI_VC_t *connect_vc = NULL;
    int port_name_tag;
    MPIDI_CH3I_Port_connreq_t *connreq = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_CONNECT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_CONNECT);

    /* Connect to the root on the other side. Create a
       temporary intercommunicator between the two roots so that
       we can use MPI functions to communicate data between them. */

    mpi_errno = MPIDI_CH3_Connect_to_root(port_name, &connect_vc);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }

    /* extract the tag from the port_name */
    mpi_errno = MPIDI_GetTagFromPort( port_name, &port_name_tag);
    if (mpi_errno != MPL_STR_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIDI_CH3I_Port_connreq_create(connect_vc, &connreq);
    MPIR_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Can't create communicator connection object.");

    /* Poke progress to wait server response for the connection request
     * before timed out. The response is handled in MPIDI_CH3_PktHandler_ConnResp
     * in progress.*/
    {
        MPID_Time_t time_sta, time_now;
        double time_gap = 0;

        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                       (MPL_DBG_FDEST, "connect: waiting accept in %d(s)", timeout));

        MPID_Wtime(&time_sta);
        do {
            mpi_errno = MPID_Progress_poke();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);

            MPID_Wtime(&time_now);
            MPID_Wtime_diff(&time_sta, &time_now, &time_gap);

            /* Avoid blocking other threads since I am inside an infinite loop */
            MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        } while (connreq->stat == MPIDI_CH3I_PORT_CONNREQ_INITED
                 && (int) time_gap < timeout);
    }

    switch (connreq->stat) {
    case MPIDI_CH3I_PORT_CONNREQ_ACCEPTED:
        /* Successfully matched an acceptance, then finish connection. */
        MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT, VERBOSE, "Matched with server accept");
        break;

    case MPIDI_CH3I_PORT_CONNREQ_INITED:
        /* Connection timed out.
         * Enqueue to revoked queue. Packet handler will notify server when
         * when server starts on it. The request will be released at finalize. */
        MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT, VERBOSE, "Connection timed out");

        MPIDI_CH3I_Port_connreq_q_enqueue(&revoked_connreq_q, connreq);
        MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, REVOKE);
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_PORT, "**ch3|conntimeout",
                             "**ch3|conntimeout %d", timeout);
        break;

    case MPIDI_CH3I_PORT_CONNREQ_ERR_CLOSE:
        /* Unexpected port closing on server.
         * The same as no port case, return MPI_ERR_PORT. Now we caught the error,
         * close vc and free connection request at fn_fail. */
        MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                    "Error - remote closed without matching this connection");

        mpi_errno = MPIDI_CH3I_Port_local_close_vc(connreq->vc);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, FREE);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_PORT, "**ch3|portclose");
        break;

    default:
        /* Unexpected status, internal error. */
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_INTERN, "**unknown");
        break;
    }

    mpi_errno = MPIDI_CH3I_Initialize_tmp_comm(&tmp_comm, connect_vc, 1, port_name_tag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }

    *comm_pptr = tmp_comm;
    *vc_pptr = connect_vc;

    MPL_free(connreq);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_CONNECT);
    return mpi_errno;
 fn_fail:
    if (connreq != NULL) {
      int mpi_errno2 = MPI_SUCCESS;
      mpi_errno2 = MPIDI_CH3I_Port_connreq_free(connreq);
      if (mpi_errno2)
          MPIR_ERR_ADD(mpi_errno, mpi_errno2);
    }
    goto fn_exit;
}

/* Creates a communicator for the purpose of communicating with one other 
   process (the root of the other group).  It also returns the virtual
   connection */
#undef FUNCNAME
#define FUNCNAME MPIDI_Create_inter_root_communicator_accept
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_Create_inter_root_communicator_accept(const char *port_name, 
						MPIR_Comm **comm_pptr,
						MPIDI_VC_t **vc_pptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *tmp_comm;
    MPIDI_VC_t *new_vc = NULL;
    MPID_Progress_state progress_state;
    int port_name_tag;
    MPIDI_CH3I_Port_connreq_t *connreq = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_ACCEPT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_ACCEPT);

    /* extract the tag from the port_name */
    mpi_errno = MPIDI_GetTagFromPort( port_name, &port_name_tag);
    if (mpi_errno != MPL_STR_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }

    /* FIXME: Describe the algorithm used here, and what routine 
       is user on the other side of this connection */
    /* dequeue the accept queue to see if a connection request with
     * the root on the connect side has been formed in the progress
     * engine. If not, poke the progress engine; If a new connection
     * request has be found, then we start accepting such request by
     * sending an ACK packet to client; Pork progress engine unless
     * we get response from client (state changed). */

    MPID_Progress_start(&progress_state);
    for (;;) {
        int matched = 0;

        if (connreq == NULL) {
            mpi_errno = MPIDI_CH3I_Acceptq_dequeue(&connreq, port_name_tag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        if (connreq != NULL && connreq->stat == MPIDI_CH3I_PORT_CONNREQ_INITED) {
            new_vc = connreq->vc;

            /* locally accept */
            MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, ACCEPT);

            /* Send connection ACK: accept to client */
            mpi_errno = MPIDI_CH3I_Port_issue_conn_ack(connreq->vc, TRUE /*accept*/);
            MPIR_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Cannot issue acceptance packet");

            MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                        "Sent acceptance to client, waiting for ACK");
        }

        /* Wait either new connection request or response packet for existing one. */
        mpi_errno = MPID_Progress_wait(&progress_state);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS) {
            MPID_Progress_end(&progress_state);
            MPIR_ERR_POP(mpi_errno);
        }
        /* --END ERROR HANDLING-- */

        /* Packet handler received response from client, check updated state */
        if (connreq != NULL && connreq->stat != MPIDI_CH3I_PORT_CONNREQ_ACCEPT) {
            switch (connreq->stat) {
            case MPIDI_CH3I_PORT_CONNREQ_ACCEPTED:
                /* Matched, now finish acceptance. */
                MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT, VERBOSE, "Matched with client connect");
                matched = 1;    /* leave loop */
                break;

            case MPIDI_CH3I_PORT_CONNREQ_FREE:
                /* Client revoked, free connection request.*/
                MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                            "Connection is already closed on client");

                /* Client already started vc closing process, thus it is safe to
                 * blocking wait here till vc freed. */
                mpi_errno = MPIDI_CH3I_Port_connreq_free(connreq);
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);

                connreq = NULL;
                break;  /* continue while loop */
            default:
                /* report internal error -- unexpected state */
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_INTERN, "**unknown");
                break;
            }
        }

        if (matched)
            break;
    }
    MPID_Progress_end(&progress_state);

    mpi_errno = MPIDI_CH3I_Initialize_tmp_comm(&tmp_comm, new_vc, 0, port_name_tag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }

    *comm_pptr = tmp_comm;
    *vc_pptr = new_vc;

    MPL_free(connreq);

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT,VERBOSE,(MPL_DBG_FDEST,
		  "new_vc=%p", new_vc));

fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR_ACCEPT);
    return mpi_errno;

fn_fail:
    if(connreq != NULL) {
        int mpi_errno2 = MPI_SUCCESS;
        mpi_errno2 = MPIDI_CH3I_Port_connreq_free(connreq);
        if (mpi_errno2)
            MPIR_ERR_ADD(mpi_errno, mpi_errno2);
    }
    goto fn_exit;
}

/* This is a utility routine used to initialize temporary communicators
   used in connect/accept operations, and is only used in the above two 
   routines */
#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Initialize_tmp_comm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Initialize_tmp_comm(MPIR_Comm **comm_pptr,
					  MPIDI_VC_t *vc_ptr, int is_low_group, int context_id_offset)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *tmp_comm, *commself_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_INITIALIZE_TMP_COMM);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_INITIALIZE_TMP_COMM);

    MPIR_Comm_get_ptr( MPI_COMM_SELF, commself_ptr );

    /* WDG-old code allocated a context id that was then discarded */
    mpi_errno = MPIR_Comm_create(&tmp_comm);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }
    /* fill in all the fields of tmp_comm. */

    /* We use the second half of the context ID bits for dynamic
     * processes. This assumes that the context ID mask array is made
     * up of uint32_t's. */
    /* FIXME: This code is still broken for the following case:
     * If the same process opens connections to the multiple
     * processes, this context ID might get out of sync.
     */
    tmp_comm->context_id     = MPIR_CONTEXT_SET_FIELD(DYNAMIC_PROC, context_id_offset, 1);
    tmp_comm->recvcontext_id = tmp_comm->context_id;

    /* sanity: the INVALID context ID value could potentially conflict with the
     * dynamic proccess space */
    MPIR_Assert(tmp_comm->context_id     != MPIR_INVALID_CONTEXT_ID);
    MPIR_Assert(tmp_comm->recvcontext_id != MPIR_INVALID_CONTEXT_ID);

    /* FIXME - we probably need a unique context_id. */
    tmp_comm->remote_size = 1;

    /* Fill in new intercomm */
    tmp_comm->local_size   = 1;
    tmp_comm->pof2         = 0;
    tmp_comm->rank         = 0;
    tmp_comm->comm_kind    = MPIR_COMM_KIND__INTERCOMM;
    tmp_comm->local_comm   = NULL;
    tmp_comm->is_low_group = is_low_group;

    /* No pg structure needed since vc has already been set up 
       (connection has been established). */

    /* Point local vcrt at those of commself_ptr */
    /* FIXME: Explain why */
    tmp_comm->dev.local_vcrt = commself_ptr->dev.vcrt;
    MPIDI_VCRT_Add_ref(commself_ptr->dev.vcrt);

    /* No pg needed since connection has already been formed. 
       FIXME - ensure that the comm_release code does not try to
       free an unallocated pg */

    /* Set up VC reference table */
    mpi_errno = MPIDI_VCRT_Create(tmp_comm->remote_size, &tmp_comm->dev.vcrt);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_vcrt");
    }

    /* FIXME: Why do we do a dup here? */
    MPIDI_VCR_Dup(vc_ptr, &tmp_comm->dev.vcrt->vcr_table[0]);

    MPIR_Coll_comm_init(tmp_comm);

    /* Even though this is a tmp comm and we don't call
       MPI_Comm_commit, we still need to call the creation hook
       because the destruction hook will be called in comm_release */
    mpi_errno = MPID_Comm_create_hook(tmp_comm);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    *comm_pptr = tmp_comm;

fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_INITIALIZE_TMP_COMM);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Comm_connect(const char *port_name, MPIR_Info *info, int root,
		       MPIR_Comm *comm_ptr, MPIR_Comm **newcomm)
{
    int mpi_errno=MPI_SUCCESS;
    int j, i, rank, recv_ints[3], send_ints[3], context_id;
    int remote_comm_size=0;
    MPIR_Comm *tmp_comm = NULL;
    MPIDI_VC_t *new_vc = NULL;
    int sendtag=100, recvtag=100, n_remote_pgs;
    int n_local_pgs=1, local_comm_size;
    pg_translation *local_translation = NULL, *remote_translation = NULL;
    pg_node *pg_list = NULL;
    MPIDI_PG_t **remote_pg = NULL;
    MPIR_Context_id_t recvcontext_id = MPIR_INVALID_CONTEXT_ID;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(3);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMM_CONNECT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_CONNECT);

    /* Get the context ID here because we need to send it to the remote side */
    mpi_errno = MPIR_Get_contextid_sparse( comm_ptr, &recvcontext_id, FALSE );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    rank = comm_ptr->rank;
    local_comm_size = comm_ptr->local_size;

    if (rank == root)
    {
        int timeout = MPIR_CVAR_CH3_COMM_CONNECT_TIMEOUT;

        /* Check if user specifies timeout threshold. */
        if (info != NULL) {
            int info_flag = 0;
            char info_value[MPI_MAX_INFO_VAL + 1];
            MPIR_Info_get_impl(info, "timeout", MPI_MAX_INFO_VAL, info_value, &info_flag);
            if (info_flag) {
                timeout = atoi(info_value);
            }
        }

	/* Establish a communicator to communicate with the root on the 
	   other side. */
	mpi_errno = MPIDI_Create_inter_root_communicator_connect(
	    port_name, timeout, &tmp_comm, &new_vc);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_POP_LABEL(mpi_errno, no_port);
	}

	/* Make an array to translate local ranks to process group index 
	   and rank */
	MPIR_CHKLMEM_MALLOC(local_translation,pg_translation*,
			    local_comm_size*sizeof(pg_translation),
			    mpi_errno,"local_translation", MPL_MEM_DYNAMIC);

	/* Make a list of the local communicator's process groups and encode 
	   them in strings to be sent to the other side.
	   The encoded string for each process group contains the process 
	   group id, size and all its KVS values */
	mpi_errno = ExtractLocalPGInfo( comm_ptr, local_translation, 
					&pg_list, &n_local_pgs );
        MPIR_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Can't extract local PG info.");

	/* Send the remote root: n_local_pgs, local_comm_size,
           Recv from the remote root: n_remote_pgs, remote_comm_size,
           recvcontext_id for newcomm */

        send_ints[0] = n_local_pgs;
        send_ints[1] = local_comm_size;
        send_ints[2] = recvcontext_id;

	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT,VERBOSE,(MPL_DBG_FDEST,
		  "sending 3 ints, %d, %d and %d, and receiving 3 ints", 
                  send_ints[0], send_ints[1], send_ints[2]));
        mpi_errno = MPIC_Sendrecv(send_ints, 3, MPI_INT, 0,
                                     sendtag++, recv_ints, 3, MPI_INT,
                                     0, recvtag++, tmp_comm,
                                     MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
            /* this is a no_port error because we may fail to connect
               on the send if the port name is invalid */
	    MPIR_ERR_POP_LABEL(mpi_errno, no_port);
	}
    }

    /* broadcast the received info to local processes */
    MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"broadcasting the received 3 ints");
    mpi_errno = MPIR_Bcast_intra_auto(recv_ints, 3, MPI_INT, root, comm_ptr, &errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    /* check if root was unable to connect to the port */
    MPIR_ERR_CHKANDJUMP1(recv_ints[0] == -1, mpi_errno, MPI_ERR_PORT, "**portexist", "**portexist %s", port_name);
    
    n_remote_pgs     = recv_ints[0];
    remote_comm_size = recv_ints[1];
    context_id	     = recv_ints[2];

    MPIR_CHKLMEM_MALLOC(remote_pg,MPIDI_PG_t**,
			n_remote_pgs * sizeof(MPIDI_PG_t*),
			mpi_errno,"remote_pg", MPL_MEM_DYNAMIC);
    MPIR_CHKLMEM_MALLOC(remote_translation,pg_translation*,
			remote_comm_size * sizeof(pg_translation),
			mpi_errno,"remote_translation", MPL_MEM_DYNAMIC);
    MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"allocated remote process groups");

    /* Exchange the process groups and their corresponding KVSes */
    if (rank == root)
    {
	mpi_errno = SendPGtoPeerAndFree( tmp_comm, &sendtag, pg_list );
	mpi_errno = ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					n_remote_pgs, remote_pg );
	/* Receive the translations from remote process rank to process group 
	   index */
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT,VERBOSE,(MPL_DBG_FDEST,
               "sending %d ints, receiving %d ints", 
	      local_comm_size * 2, remote_comm_size * 2));
	mpi_errno = MPIC_Sendrecv(local_translation, local_comm_size * 2,
				  MPI_INT, 0, sendtag++,
				  remote_translation, remote_comm_size * 2, 
				  MPI_INT, 0, recvtag++, tmp_comm,
				  MPI_STATUS_IGNORE, &errflag);
	if (mpi_errno) {
	    MPIR_ERR_POP(mpi_errno);
	}

#ifdef MPICH_DBG_OUTPUT
	MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE,"[%d]connect:Received remote_translation:\n", rank);
	for (i=0; i<remote_comm_size; i++)
	{
	    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
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
    MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"Broadcasting remote translation");
    mpi_errno = MPIR_Bcast_intra_auto(remote_translation, remote_comm_size * 2, MPI_INT,
                                 root, comm_ptr, &errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

#ifdef MPICH_DBG_OUTPUT
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE,"[%d]connect:Received remote_translation after broadcast:\n", rank);
    for (i=0; i<remote_comm_size; i++)
    {
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
	    i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank));
    }
#endif

    mpi_errno = MPIR_Comm_create(newcomm);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    (*newcomm)->context_id     = context_id;
    (*newcomm)->recvcontext_id = recvcontext_id;
    (*newcomm)->is_low_group   = 1;

    mpi_errno = SetupNewIntercomm( comm_ptr, remote_comm_size, 
				   remote_translation, remote_pg, *newcomm );
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }

    /* synchronize with remote root */
    if (rank == root)
    {
	MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"sync with peer");
        mpi_errno = MPIC_Sendrecv(&i, 0, MPI_INT, 0,
                                     sendtag++, &j, 0, MPI_INT,
                                     0, recvtag++, tmp_comm,
                                     MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_POP(mpi_errno);
        }

        /* All communication with remote root done. Release the communicator. */
        MPIR_Comm_release(tmp_comm);
    }

    /*printf("connect:barrier\n");fflush(stdout);*/
    mpi_errno = MPIR_Barrier_intra_auto(comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }

    /* Free new_vc. It was explicitly allocated in MPIDI_CH3_Connect_to_root.*/
    if (rank == root) {
	FreeNewVC( new_vc );
    }

 fn_exit: 
    MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"Exiting ch3u_comm_connect");
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_CONNECT);
    return mpi_errno;
 fn_fail:
    {
        int mpi_errno2 = MPI_SUCCESS;
        if (new_vc) {
	    mpi_errno2 = MPIDI_CH3_VC_Destroy(new_vc);
            if (mpi_errno2) MPIR_ERR_SET(mpi_errno2, MPI_ERR_OTHER, "**fail");
        }

        if (recvcontext_id != MPIR_INVALID_CONTEXT_ID)
            MPIR_Free_contextid(recvcontext_id);
        
        if (mpi_errno2) MPIR_ERR_ADD(mpi_errno, mpi_errno2);

        goto fn_exit;
    }
 no_port:
    {
        int mpi_errno_noport = MPI_SUCCESS;
        int mpi_errno2 = MPI_SUCCESS;

       /* broadcast error notification to other processes */
        MPIR_Assert(rank == root);
        recv_ints[0] = -1;
        recv_ints[1] = -1;
        recv_ints[2] = -1;

        /* append no port error message */
        MPIR_ERR_SET1(mpi_errno_noport, MPI_ERR_PORT, "**portexist", "**portexist %s", port_name);
        MPIR_ERR_ADD(mpi_errno_noport, mpi_errno);
        mpi_errno = mpi_errno_noport;

        /* notify other processes to return an error */
        MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"broadcasting 3 ints: error case");
        mpi_errno2 = MPIR_Bcast_intra_auto(recv_ints, 3, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno2) MPIR_ERR_ADD(mpi_errno, mpi_errno2);
        if (errflag) {
            MPIR_ERR_SET(mpi_errno2, MPI_ERR_OTHER, "**coll_fail");
            MPIR_ERR_ADD(mpi_errno, mpi_errno2);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
static int ExtractLocalPGInfo( MPIR_Comm *comm_p,
			       pg_translation local_translation[], 
			       pg_node **pg_list_p,
			       int *n_local_pgs_p )
{
    pg_node        *pg_list = 0, *pg_iter, *pg_trailer;
    int            i, cur_index = 0, local_comm_size, mpi_errno = 0;
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_EXTRACTLOCALPGINFO);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_EXTRACTLOCALPGINFO);

    /* If we are in the case of singleton-init, we may need to reset the
       id string for comm world.  We do this before doing anything else */
    MPIDI_PG_CheckForSingleton();

    local_comm_size = comm_p->local_size;

    /* Make a list of the local communicator's process groups and encode 
       them in strings to be sent to the other side.
       The encoded string for each process group contains the process 
       group id, size and all its KVS values */
    
    cur_index = 0;
    MPIR_CHKPMEM_MALLOC(pg_list,pg_node*,sizeof(pg_node),mpi_errno,
			"pg_list", MPL_MEM_ADDRESS);
    
    pg_list->pg_id = MPL_strdup(comm_p->dev.vcrt->vcr_table[0]->pg->id);
    pg_list->index = cur_index++;
    pg_list->next = NULL;
    /* XXX DJG FIXME-MT should we be checking this?  the add/release macros already check this */
    MPIR_Assert( MPIR_Object_get_ref(comm_p->dev.vcrt->vcr_table[0]->pg));
    mpi_errno = MPIDI_PG_To_string(comm_p->dev.vcrt->vcr_table[0]->pg, &pg_list->str,
				   &pg_list->lenStr );
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }
    MPL_DBG_STMT(MPIDI_CH3_DBG_CONNECT,VERBOSE,MPIDI_PrintConnStr(__FILE__,__LINE__,"PG as string is", pg_list->str ));
    local_translation[0].pg_index = 0;
    local_translation[0].pg_rank = comm_p->dev.vcrt->vcr_table[0]->pg_rank;
    pg_iter = pg_list;
    for (i=1; i<local_comm_size; i++) {
	pg_iter = pg_list;
	pg_trailer = pg_list;
	while (pg_iter != NULL) {
	    /* Check to ensure pg is (probably) valid */
            /* XXX DJG FIXME-MT should we be checking this?  the add/release macros already check this */
	    MPIR_Assert(MPIR_Object_get_ref(comm_p->dev.vcrt->vcr_table[i]->pg) != 0);
	    if (MPIDI_PG_Id_compare(comm_p->dev.vcrt->vcr_table[i]->pg->id, pg_iter->pg_id)) {
		local_translation[i].pg_index = pg_iter->index;
		local_translation[i].pg_rank  = comm_p->dev.vcrt->vcr_table[i]->pg_rank;
		break;
	    }
	    if (pg_trailer != pg_iter)
		pg_trailer = pg_trailer->next;
	    pg_iter = pg_iter->next;
	}
	if (pg_iter == NULL) {
	    /* We use MPL_malloc directly because we do not know in
	       advance how many nodes we may allocate */
	    pg_iter = (pg_node*)MPL_malloc(sizeof(pg_node), MPL_MEM_DYNAMIC);
	    if (!pg_iter) {
		MPIR_ERR_POP(mpi_errno);
	    }
	    pg_iter->pg_id = MPL_strdup(comm_p->dev.vcrt->vcr_table[i]->pg->id);
	    pg_iter->index = cur_index++;
	    pg_iter->next = NULL;
	    mpi_errno = MPIDI_PG_To_string(comm_p->dev.vcrt->vcr_table[i]->pg, &pg_iter->str,
					   &pg_iter->lenStr );
	    if (mpi_errno != MPI_SUCCESS) {
		MPIR_ERR_POP(mpi_errno);
	    }
	    local_translation[i].pg_index = pg_iter->index;
	    local_translation[i].pg_rank = comm_p->dev.vcrt->vcr_table[i]->pg_rank;
	    pg_trailer->next = pg_iter;
	}
    }

    *n_local_pgs_p = cur_index;
    *pg_list_p     = pg_list;
    
#ifdef MPICH_DBG_OUTPUT
    pg_iter = pg_list;
    while (pg_iter != NULL) {
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST,"connect:PG: '%s'\n<%s>\n", pg_iter->pg_id, pg_iter->str));
	pg_iter = pg_iter->next;
    }
#endif


 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_EXTRACTLOCALPGINFO);
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}


/* The root process in comm_ptr receives strings describing the
   process groups and then distributes them to the other processes
   in comm_ptr.
   See SendPGToPeer for the routine that sends the descriptions */
#undef FUNCNAME
#define FUNCNAME ReceivePGAndDistribute
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int ReceivePGAndDistribute( MPIR_Comm *tmp_comm, MPIR_Comm *comm_ptr,
				   int root, int *recvtag_p, 
				   int n_remote_pgs, MPIDI_PG_t *remote_pg[] )
{
    char *pg_str = 0;
    int  i, j, flag;
    int  rank = comm_ptr->rank;
    int  mpi_errno = 0;
    int  recvtag = *recvtag_p;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_RECEIVEPGANDDISTRIBUTE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_RECEIVEPGANDDISTRIBUTE);

    for (i=0; i<n_remote_pgs; i++) {

	if (rank == root) {
	    /* First, receive the pg description from the partner */
	    mpi_errno = MPIC_Recv(&j, 1, MPI_INT, 0, recvtag++,
				  tmp_comm, MPI_STATUS_IGNORE, &errflag);
	    *recvtag_p = recvtag;
	    if (mpi_errno != MPI_SUCCESS) {
		MPIR_ERR_POP(mpi_errno);
	    }
	    pg_str = (char*)MPL_malloc(j, MPL_MEM_DYNAMIC);
	    if (pg_str == NULL) {
		MPIR_ERR_POP(mpi_errno);
	    }
	    mpi_errno = MPIC_Recv(pg_str, j, MPI_CHAR, 0, recvtag++,
				  tmp_comm, MPI_STATUS_IGNORE, &errflag);
	    *recvtag_p = recvtag;
	    if (mpi_errno != MPI_SUCCESS) {
		MPIR_ERR_POP(mpi_errno);
	    }
	}

	/* Broadcast the size and data to the local communicator */
	/*printf("accept:broadcasting 1 int\n");fflush(stdout);*/
	mpi_errno = MPIR_Bcast_intra_auto(&j, 1, MPI_INT, root, comm_ptr, &errflag);
	if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

	if (rank != root) {
	    /* The root has already allocated this string */
	    pg_str = (char*)MPL_malloc(j, MPL_MEM_DYNAMIC);
	    if (pg_str == NULL) {
		MPIR_ERR_POP(mpi_errno);
	    }
	}
	/*printf("accept:broadcasting string of length %d\n", j);fflush(stdout);*/
	mpi_errno = MPIR_Bcast_intra_auto(pg_str, j, MPI_CHAR, root, comm_ptr, &errflag);
	if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	/* Then reconstruct the received process group.  This step
	   also initializes the created process group */

	MPL_DBG_STMT(MPIDI_CH3_DBG_CONNECT,VERBOSE,MPIDI_PrintConnStr(__FILE__,__LINE__,"Creating pg from string", pg_str ));
	mpi_errno = MPIDI_PG_Create_from_string(pg_str, &remote_pg[i], &flag);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_POP(mpi_errno);
	}
	
	MPL_free(pg_str);
    }
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_RECEIVEPGANDDISTRIBUTE);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_PG_BCast( MPIR_Comm *peercomm_p, MPIR_Comm *comm_p, int root )
{
    int n_local_pgs=0, mpi_errno = MPI_SUCCESS;
    pg_translation *local_translation = 0;
    pg_node *pg_list, *pg_next, *pg_head = 0;
    int rank, i, peer_comm_size;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(1);

    peer_comm_size = comm_p->local_size;
    rank            = comm_p->rank;

    MPIR_CHKLMEM_MALLOC(local_translation,pg_translation*,
			peer_comm_size*sizeof(pg_translation),
			mpi_errno,"local_translation", MPL_MEM_DYNAMIC);
    
    if (rank == root) {
	/* Get the process groups known to the *peercomm* */
	ExtractLocalPGInfo( peercomm_p, local_translation, &pg_head, 
			    &n_local_pgs );
    }

    /* Now, broadcast the number of local pgs */
    mpi_errno = MPIR_Bcast( &n_local_pgs, 1, MPI_INT, root, comm_p, &errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

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
	mpi_errno = MPIR_Bcast( &len, 1, MPI_INT, root, comm_p, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	if (rank != root) {
	    pg_str = (char *)MPL_malloc(len, MPL_MEM_DYNAMIC);
            if (!pg_str) {
                MPIR_CHKMEM_SETERR(mpi_errno, len, "pg_str");
                goto fn_exit;
            }
	}
	mpi_errno = MPIR_Bcast( pg_str, len, MPI_CHAR, root, comm_p, &errflag);
        if (mpi_errno) {
            if (rank != root)
                MPL_free( pg_str );
            MPIR_ERR_POP(mpi_errno);
        }
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

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
	    MPL_free( pg_str );
	}
    }

    /* Free pg_list */
    pg_list = pg_head;

    /* FIXME: We should use the PG destroy function for this, and ensure that
       the PG fields are valid for that function */
    while (pg_list) {
	pg_next = pg_list->next;
	MPL_free( pg_list->str );
	if (pg_list->pg_id ) {
	    MPL_free( pg_list->pg_id );
	}
	MPL_free( pg_list );
	pg_list = pg_next;
    }

 fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Sends the process group information to the peer and frees the 
   pg_list */
#undef FUNCNAME
#define FUNCNAME SendPGtoPeerAndFree
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int SendPGtoPeerAndFree( MPIR_Comm *tmp_comm, int *sendtag_p,
				pg_node *pg_list )
{
    int mpi_errno = 0;
    int sendtag = *sendtag_p, i;
    pg_node *pg_iter;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SENDPGTOPEERANDFREE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SENDPGTOPEERANDFREE);

    while (pg_list != NULL) {
	pg_iter = pg_list;
	i = pg_iter->lenStr;
	/*printf("connect:sending 1 int: %d\n", i);fflush(stdout);*/
	mpi_errno = MPIC_Send(&i, 1, MPI_INT, 0, sendtag++, tmp_comm, &errflag);
	*sendtag_p = sendtag;
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_POP(mpi_errno);
	}
	
	/* printf("connect:sending string length %d\n", i);fflush(stdout); */
	mpi_errno = MPIC_Send(pg_iter->str, i, MPI_CHAR, 0, sendtag++,
			      tmp_comm, &errflag);
	*sendtag_p = sendtag;
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_POP(mpi_errno);
	}
	
	pg_list = pg_list->next;
	MPL_free(pg_iter->str);
	MPL_free(pg_iter->pg_id);
	MPL_free(pg_iter);
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SENDPGTOPEERANDFREE);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Comm_accept(const char *port_name, MPIR_Info *info, int root,
		      MPIR_Comm *comm_ptr, MPIR_Comm **newcomm)
{
    int mpi_errno=MPI_SUCCESS;
    int i, j, rank, recv_ints[3], send_ints[3], context_id;
    int remote_comm_size=0;
    MPIR_Comm *tmp_comm = NULL, *intercomm;
    MPIDI_VC_t *new_vc = NULL;
    int sendtag=100, recvtag=100, local_comm_size;
    int n_local_pgs=1, n_remote_pgs;
    pg_translation *local_translation = NULL, *remote_translation = NULL;
    pg_node *pg_list = NULL;
    MPIDI_PG_t **remote_pg = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(3);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMM_ACCEPT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_ACCEPT);

    /* Create the new intercommunicator here. We need to send the
       context id to the other side. */
    mpi_errno = MPIR_Comm_create(newcomm);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }
    mpi_errno = MPIR_Get_contextid_sparse( comm_ptr, &(*newcomm)->recvcontext_id, FALSE );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
	    MPIR_ERR_POP(mpi_errno);
	}

	/* Make an array to translate local ranks to process group index and 
	   rank */
	MPIR_CHKLMEM_MALLOC(local_translation,pg_translation*,
			    local_comm_size*sizeof(pg_translation),
			    mpi_errno,"local_translation", MPL_MEM_DYNAMIC);

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
                                     0, recvtag++, tmp_comm,
                                     MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_POP(mpi_errno);
	}
    }

    /* broadcast the received info to local processes */
    /*printf("accept:broadcasting 2 ints - %d and %d\n", recv_ints[0], recv_ints[1]);fflush(stdout);*/
    mpi_errno = MPIR_Bcast_intra_auto(recv_ints, 3, MPI_INT, root, comm_ptr, &errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");


    n_remote_pgs     = recv_ints[0];
    remote_comm_size = recv_ints[1];
    context_id       = recv_ints[2];
    MPIR_CHKLMEM_MALLOC(remote_pg,MPIDI_PG_t**,
			n_remote_pgs * sizeof(MPIDI_PG_t*),
			mpi_errno,"remote_pg", MPL_MEM_DYNAMIC);
    MPIR_CHKLMEM_MALLOC(remote_translation,pg_translation*,
			remote_comm_size * sizeof(pg_translation),
			mpi_errno, "remote_translation", MPL_MEM_DYNAMIC);
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST,"[%d]accept:remote process groups: %d\nremote comm size: %d\n", rank, n_remote_pgs, remote_comm_size));

    /* Exchange the process groups and their corresponding KVSes */
    if (rank == root)
    {
	/* The root receives the PG from the peer (in tmp_comm) and
	   distributes them to the processes in comm_ptr */
	mpi_errno = ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					n_remote_pgs, remote_pg );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	
	mpi_errno = SendPGtoPeerAndFree( tmp_comm, &sendtag, pg_list );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

	/* Receive the translations from remote process rank to process group index */
	/*printf("accept:sending %d ints and receiving %d ints\n", local_comm_size * 2, remote_comm_size * 2);fflush(stdout);*/
	mpi_errno = MPIC_Sendrecv(local_translation, local_comm_size * 2,
				  MPI_INT, 0, sendtag++,
				  remote_translation, remote_comm_size * 2, 
				  MPI_INT, 0, recvtag++, tmp_comm,
				  MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

#ifdef MPICH_DBG_OUTPUT
	MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE,"[%d]accept:Received remote_translation:\n", rank);
	for (i=0; i<remote_comm_size; i++)
	{
	    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
		i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank));
	}
#endif
    }
    else
    {
	mpi_errno = ReceivePGAndDistribute( tmp_comm, comm_ptr, root, &recvtag,
					    n_remote_pgs, remote_pg );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* Broadcast out the remote rank translation array */
    MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"Broadcast remote_translation");
    mpi_errno = MPIR_Bcast_intra_auto(remote_translation, remote_comm_size * 2, MPI_INT,
                                 root, comm_ptr, &errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
#ifdef MPICH_DBG_OUTPUT
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE,"[%d]accept:Received remote_translation after broadcast:\n", rank);
    for (i=0; i<remote_comm_size; i++)
    {
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
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
	MPIR_ERR_POP(mpi_errno);
    }

    /* synchronize with remote root */
    if (rank == root)
    {
	MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"sync with peer");
        mpi_errno = MPIC_Sendrecv(&i, 0, MPI_INT, 0,
                                     sendtag++, &j, 0, MPI_INT,
                                     0, recvtag++, tmp_comm,
                                     MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_POP(mpi_errno);
        }

        /* All communication with remote root done. Release the communicator. */
        MPIR_Comm_release(tmp_comm);
    }

    MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"Barrier");
    mpi_errno = MPIR_Barrier_intra_auto(comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }

    /* Free new_vc once the connection is completed. It was explicitly 
       allocated in ch3_progress.c and returned by 
       MPIDI_CH3I_Acceptq_dequeue. */
    if (rank == root) {
	FreeNewVC( new_vc );
    }

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_ACCEPT);
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

Input/Output Parameters:
.  intercomm - New intercommunicator.  The intercommunicator must already
   have been allocated; this routine initializes many of the fields

   Note:
   This routine performance a barrier over 'comm_ptr'.  Why?
*/
#undef FUNCNAME
#define FUNCNAME SetupNewIntercomm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int SetupNewIntercomm( MPIR_Comm *comm_ptr, int remote_comm_size,
			      pg_translation remote_translation[],
			      MPIDI_PG_t **remote_pg, 
			      MPIR_Comm *intercomm )
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    /* FIXME: How much of this could/should be common with the
       upper level (src/mpi/comm/ *.c) code? For best robustness, 
       this should use the same routine (not copy/paste code) as
       in the upper level code. */
    intercomm->attributes   = NULL;
    intercomm->remote_size  = remote_comm_size;
    intercomm->local_size   = comm_ptr->local_size;
    intercomm->pof2         = MPL_pof2(intercomm->local_size);
    intercomm->rank         = comm_ptr->rank;
    intercomm->local_group  = NULL;
    intercomm->remote_group = NULL;
    intercomm->comm_kind    = MPIR_COMM_KIND__INTERCOMM;
    intercomm->local_comm   = NULL;

    /* Point local vcrt at those of incoming intracommunicator */
    intercomm->dev.local_vcrt = comm_ptr->dev.vcrt;
    MPIDI_VCRT_Add_ref(comm_ptr->dev.vcrt);

    /* Set up VC reference table */
    mpi_errno = MPIDI_VCRT_Create(intercomm->remote_size, &intercomm->dev.vcrt);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_vcrt");
    }
    for (i=0; i < intercomm->remote_size; i++) {
	MPIDI_PG_Dup_vcr(remote_pg[remote_translation[i].pg_index], 
			 remote_translation[i].pg_rank, &intercomm->dev.vcrt->vcr_table[i]);
    }

    mpi_errno = MPIR_Comm_commit(intercomm);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    MPL_DBG_MSG(MPIDI_CH3_DBG_CONNECT,VERBOSE,"Barrier");
    mpi_errno = MPIR_Barrier_intra_auto(comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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
		    MPIR_ERR_POP(mpi_errno);
                }
	    /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&progress_state);
    }

    MPIDI_CH3_VC_Destroy(new_vc);
    MPL_free(new_vc);

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

/* Enqueue a connection request from client. This routine is called from netmod
 * (i.e., TCP) if received dynamic connection from others. If port exists, we
 * enqueue the request to that port's accept queue to wait for an accept call to
 * serve it; otherwise, such request should be discarded, thus we immediately send
 * nack back to client and start closing. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Acceptq_enqueue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Acceptq_enqueue(MPIDI_VC_t * vc, int port_name_tag )
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_CH3I_Port_connreq_t *connreq = NULL;
    MPIDI_CH3I_Port_t *port = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);

    LL_SEARCH_SCALAR(active_portq.head, port, port_name_tag, port_name_tag);

    /* Find port object by using port_name_tag. */
    mpi_errno = MPIDI_CH3I_Port_connreq_create(vc, &connreq);
    MPIR_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Can't create communicator connection object.");

    /* No port exists if port is not opened or already closed (incorrect user code).
     * Thus we just start closing VC here. */
    if (port == NULL) {
        /* Notify connecting client. */
        mpi_errno = MPIDI_CH3I_Port_issue_conn_ack(connreq->vc, FALSE /* closed port */);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        /* Start VC closing protocol. */
        mpi_errno = MPIDI_CH3I_Port_local_close_vc(connreq->vc);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, FREE);

        /* Enqueue unexpected VC to avoid waiting progress recursively.
         * these VCs will be freed in finalize. */
        MPIDI_CH3I_Port_connreq_q_enqueue(&unexpt_connreq_q, connreq);
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                        (MPL_DBG_FDEST, "Enqueued conn %p to unexpected queue with tag %d, vc=%p",
                         connreq, port_name_tag, vc));
    }
    else {
        /* Enqueue to accept queue, thus next accept call can serve it. */
        MPIDI_CH3I_Port_connreq_q_enqueue(&port->accept_connreq_q, connreq);
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                        (MPL_DBG_FDEST, "Enqueued conn %p to accept queue with tag %d, vc=%p",
                         connreq, port_name_tag, vc));

        /* signal for new enqueued VC, thus progress wait can return in accept. */
        MPIDI_CH3_Progress_signal_completion();
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);
    return mpi_errno;
  fn_fail:
    if (connreq)
        MPIDI_CH3I_Port_connreq_free(connreq);
    goto fn_exit;
}


/* Attempt to dequeue a connection request from the accept queue. If the queue
 * is empty return a NULL object. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Acceptq_dequeue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Acceptq_dequeue(MPIDI_CH3I_Port_connreq_t ** connreq_ptr, int port_name_tag)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_CH3I_Port_t *port = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);

    /* Find port object by using port_name_tag. */
    LL_SEARCH_SCALAR(active_portq.head, port, port_name_tag, port_name_tag);
    MPIR_Assert(port != NULL);  /* Port is always initialized in open_port. */

    MPIDI_CH3I_Port_connreq_q_dequeue(&port->accept_connreq_q, connreq_ptr);
    if ((*connreq_ptr) != NULL) {
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                        (MPL_DBG_FDEST, "conn=%p:Dequeued accept connection with tag %d, vc=%p",
                         (*connreq_ptr), port_name_tag, (*connreq_ptr)->vc));
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);
    return mpi_errno;
}

/* Clean up received new VCs that are not accepted or closed in accept
 * calls (e.g., mismatching accept and connect).This routine is called in
 * MPIDI_CH3I_Port_destroy(close_port) and MPIDI_Port_finalize (finalize).
 * Note that we already deleted port from active_port queue before cleaning up
 * its accept queue, thus no new VC can be enqueued concurrently. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Acceptq_cleanup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Acceptq_cleanup(MPIDI_CH3I_Port_connreq_q_t * accept_connreq_q)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Port_connreq_t *connreq = NULL, *connreq_tmp = NULL;

    LL_FOREACH_SAFE(accept_connreq_q->head, connreq, connreq_tmp) {
        MPIDI_CH3I_Port_connreq_q_delete(accept_connreq_q, connreq);

        /* Notify connecting client. */
        mpi_errno = MPIDI_CH3I_Port_issue_conn_ack(connreq->vc, FALSE /* closed port */);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        /* Start VC closing protocol. */
        mpi_errno = MPIDI_CH3I_Port_local_close_vc(connreq->vc);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, FREE);

        /* Free connection request (blocking wait till VC closed). */
        mpi_errno = MPIDI_CH3I_Port_connreq_free(connreq);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_Assert(accept_connreq_q->size == 0);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/*** Utility routines for revoked connection requests   ***/

/* Clean up revoked requests in connect (e.g., timed out connect).
 * We do not want to wait for these VCs being freed in timed out connect,
 * because it is blocked till the server calls a matching accept or close_port.
 * This routine is called in finalize on client process. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Revokeq_cleanup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Revokeq_cleanup(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Port_connreq_t *connreq = NULL, *connreq_tmp = NULL;

    LL_FOREACH_SAFE(revoked_connreq_q.head, connreq, connreq_tmp) {
        MPID_Progress_state progress_state;
        MPIDI_CH3I_Port_connreq_q_delete(&revoked_connreq_q, connreq);

        /* Blocking wait till the request is freed on server. */
        if (connreq->stat != MPIDI_CH3I_PORT_CONNREQ_FREE) {
            MPID_Progress_start(&progress_state);
            do {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_Progress_end(&progress_state);
                    MPIR_ERR_POP(mpi_errno);
                }
                /* --END ERROR HANDLING-- */
            } while (connreq->stat != MPIDI_CH3I_PORT_CONNREQ_FREE);
            MPID_Progress_end(&progress_state);
        }

        /* Release connection (blocking wait till VC closed). */
        MPIDI_CH3I_Port_connreq_free(connreq);
    }

    MPIR_Assert(revoked_connreq_q.size == 0);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*** Packet handlers exposed to progress engine  ***/

/* Packet handler to handle response (connection ACK) on client process. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_ConnAck
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_ConnAck(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                 void *data ATTRIBUTE((unused)),
                                 intptr_t * buflen, MPIR_Request ** rreqp)
{
    MPIDI_CH3_Pkt_conn_ack_t *ack_pkt = &pkt->conn_ack;
    MPIDI_CH3I_Port_connreq_t *connreq = (MPIDI_CH3I_Port_connreq_t *) (vc->connreq_obj);
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(connreq != NULL);

    /* Report unknown error, unexpectedly get response for remote
     * revoked connection. */
    if (connreq->stat != MPIDI_CH3I_PORT_CONNREQ_INITED &&
        connreq->stat != MPIDI_CH3I_PORT_CONNREQ_REVOKE)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_INTERN, "**unknown");

    if (ack_pkt->ack == TRUE) {
        if (connreq->stat == MPIDI_CH3I_PORT_CONNREQ_INITED) {
            MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                            (MPL_DBG_FDEST, "received ACK true for vc %p: inited->accepted", vc));

            /* Reply to server */
            mpi_errno = MPIDI_CH3I_Port_issue_accept_ack(connreq->vc, TRUE /* accept matched */);
            MPIR_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Cannot issue accept-matched packet");

            MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, ACCEPTED);
        }
        else if (connreq->stat == MPIDI_CH3I_PORT_CONNREQ_REVOKE) {
            MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                            (MPL_DBG_FDEST, "received ACK true for vc %p: revoke->free", vc));

            /* Reply to server */
            mpi_errno = MPIDI_CH3I_Port_issue_accept_ack(connreq->vc, FALSE /* locally revoked */);
            MPIR_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Cannot issue revoke packet");

            /* Start freeing connection request.
             * Note that we do not blocking close VC here, instead, we close VC
             * in MPIDI_CH3I_Revokeq_cleanup at finalize. This is because
             * VC close packets might be received following this packet, thus if
             * we blocked here we can never read that packet. */
            mpi_errno = MPIDI_CH3I_Port_local_close_vc(connreq->vc);
            MPIR_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Cannot locally close VC");

            MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, FREE);
        }
    }
    else {      /* ack == FALSE */
        if (connreq->stat == MPIDI_CH3I_PORT_CONNREQ_INITED) {
            MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                            (MPL_DBG_FDEST, "received ACK false for vc %p: inited->err_close", vc));

            /* Server closed port without issuing accept, client
             * connect call will catch this error and return MPI_ERR_PORT. */
            MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, ERR_CLOSE);
        }
        else if (connreq->stat == MPIDI_CH3I_PORT_CONNREQ_REVOKE) {
            MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                            (MPL_DBG_FDEST, "received ACK false for vc %p: revoke->free", vc));

            /* Start VC closing, and set connection ready-for-free. */
            mpi_errno = MPIDI_CH3I_Port_local_close_vc(connreq->vc);
            MPIR_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Cannot locally close VC");

            MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, FREE);
        }
    }

    *buflen = 0;
    *rreqp = NULL;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Packet handler to handle response (acceptance ACK) on server process. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_AcceptAck
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_AcceptAck(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                   void *data ATTRIBUTE((unused)),
                                   intptr_t * buflen, MPIR_Request ** rreqp)
{
    MPIDI_CH3_Pkt_accept_ack_t *ack_pkt = &pkt->accept_ack;
    MPIDI_CH3I_Port_connreq_t *connreq = (MPIDI_CH3I_Port_connreq_t *) (vc->connreq_obj);
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(connreq != NULL);

    /* Acceptance matched, finish accept. */
    if (ack_pkt->ack == TRUE) {
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                        (MPL_DBG_FDEST, "received (accept) ACK true for vc %p: accept->match", vc));

        MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, ACCEPTED);
    }
    else {
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                        (MPL_DBG_FDEST, "received (accept) ACK false for vc %p: accept->close",
                         vc));

        /* Client already left, close VC.
         * Note that accept call does not return when client timed out,
         * thus we only change the state and let accept call handle closing. */
        mpi_errno = MPIDI_CH3I_Port_local_close_vc(connreq->vc);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, FREE);
    }

    *buflen = 0;
    *rreqp = NULL;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*** Routines for connection request creation and freeing  ***/

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_connreq_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Port_connreq_create(MPIDI_VC_t * vc, MPIDI_CH3I_Port_connreq_t ** connreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Port_connreq_t *connreq = NULL;

    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKPMEM_MALLOC(connreq, MPIDI_CH3I_Port_connreq_t *, sizeof(MPIDI_CH3I_Port_connreq_t),
                        mpi_errno, "comm_conn", MPL_MEM_DYNAMIC);

    connreq->vc = vc;
    MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, INITED);

    /* Netmod may not change VC to active when connection established (i.e., sock).
     * Instead, it is changed in CH3 layer (e.g., isend, RMA).*/
    if (vc->state == MPIDI_VC_STATE_INACTIVE)
        MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    vc->connreq_obj = (void *) connreq; /* to get connection request in packet handlers */
    *connreq_ptr = connreq;

  fn_exit:
    MPIR_CHKPMEM_COMMIT();
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_connreq_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Port_connreq_free(MPIDI_CH3I_Port_connreq_t * connreq)
{
    int mpi_errno = MPI_SUCCESS;

    /* Skip if connection request is still in revoked state.
     * Because packet handler may be talking to server. */
    if (connreq->stat == MPIDI_CH3I_PORT_CONNREQ_REVOKE)
        return mpi_errno;

    /* Expected free, the remote side should also start closing too,
     * thus we blocking close VC here. */
    if (connreq->stat == MPIDI_CH3I_PORT_CONNREQ_FREE) {
        mpi_errno = FreeNewVC(connreq->vc);
    }
    else {
        /* Unexpected free, the remote side might not be able to close
         * VC at this point. Thus we cannot blocking close VC. */
        mpi_errno = MPIDI_CH3_VC_Destroy(connreq->vc);
    }

    /* Always free connection request.
     * Because it is only used in connect/accept routine. */
    MPL_free(connreq);

    return mpi_errno;
}


/*** Routines to initialize / destroy dynamic connection  ***/

/* Initialize port's accept queue. It is called in MPIDI_Open_port. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Port_init(int port_name_tag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Port_t *port = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PORT_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PORT_INIT);

    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKPMEM_MALLOC(port, MPIDI_CH3I_Port_t *, sizeof(MPIDI_CH3I_Port_t),
                        mpi_errno, "comm_port", MPL_MEM_DYNAMIC);

    port->port_name_tag = port_name_tag;
    port->accept_connreq_q.head = port->accept_connreq_q.tail = 0;
    port->accept_connreq_q.size = 0;
    port->next = NULL;

    LL_APPEND(active_portq.head, active_portq.tail, port);
    active_portq.size++;

  fn_exit:
    MPIR_CHKPMEM_COMMIT();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PORT_INIT);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

/* Destroy port's accept queue. It is called in MPIDI_Close_port. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Port_destroy(int port_name_tag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Port_t *port = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PORT_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PORT_DESTROY);

    LL_SEARCH_SCALAR(active_portq.head, port, port_name_tag, port_name_tag);
    if (port != NULL) {
        LL_DELETE(active_portq.head, active_portq.tail, port);

        mpi_errno = MPIDI_CH3I_Acceptq_cleanup(&port->accept_connreq_q);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        MPL_free(port);
        active_portq.size--;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PORT_DESTROY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This routine is called by MPID_Finalize to clean up dynamic connections. */
#undef FUNCNAME
#define FUNCNAME MPIDI_Port_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Port_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PORT_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PORT_FINALIZE);

    /* Server side clean up. */

    /* - Clean up all active ports.
     * Note that if a process is both server and client, we will never
     * deadlock here because only server cleanup issues closing packets.
     * All closing process on client is handled in progress. */
    {
        MPIDI_CH3I_Port_t *port = NULL, *port_tmp = NULL;

        LL_FOREACH_SAFE(active_portq.head, port, port_tmp) {
            /* destroy all opening ports. */
            LL_DELETE(active_portq.head, active_portq.tail, port);

            mpi_errno = MPIDI_CH3I_Acceptq_cleanup(&port->accept_connreq_q);
            MPL_free(port);
            active_portq.size--;
        }
        MPIR_Assert(active_portq.size == 0);
    }

    /* - Destroy all unexpected connection requests.
     * The closing protocol already started when we got them in acceptq_enqueue,
     * so just blocking wait for final release. */
    {
        MPIDI_CH3I_Port_connreq_t *connreq = NULL, *connreq_tmp = NULL;

        LL_FOREACH_SAFE(unexpt_connreq_q.head, connreq, connreq_tmp) {
            MPIDI_CH3I_Port_connreq_q_delete(&unexpt_connreq_q, connreq);
            mpi_errno = MPIDI_CH3I_Port_connreq_free(connreq);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        MPIR_Assert(unexpt_connreq_q.size == 0);
    }

    /* Client side clean up. */

    /* - Destroy all revoked connection requests. */
    mpi_errno = MPIDI_CH3I_Revokeq_cleanup();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PORT_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#else  /* MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS is defined */

#endif /* MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS */

