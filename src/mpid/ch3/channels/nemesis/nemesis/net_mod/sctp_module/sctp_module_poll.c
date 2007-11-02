/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sctp_module_impl.h"


/* design assumes only one partial delivery happens at once, as is the case
 *  in most stacks (and defined in the standard, I think)
 */
static char partial_msg_buf[MPID_NEM_MAX_PACKET_LEN];
static char partial_msg_tmpbuf[MPID_NEM_MAX_PACKET_LEN];
static int partial_msg_sz = 0;
static int partial_msg_in_progress = 0;

#undef FUNCNAME
#define FUNCNAME send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int send_progress()
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =  MPID_nem_sctp_module_send_progress();

/*     MPIDI_VC_t *vc; */

    /* TODO need a send Q because can't assume success on first call to
     *   the net module's send */
    
/*     for (vc = MPID_nem_sctp_module_send_list.head; vc; vc = vc->ch.next) */
/*     { */
/*         mpi_errno = MPID_nem_sctp_module_send_queue (vc); */
/*         if (mpi_errno) MPIU_ERR_POP (mpi_errno); */
/*     } */

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME recv_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int recv_progress()
{
    struct sctp_sndrcvinfo sri;
    struct sockaddr from;
    socklen_t fromlen = sizeof(from);
    int msg_flags;
    int sz;
    MPID_nem_sctp_hash_entry *result;
    MPID_nem_sctp_hash_entry  lresult;
    char *buf_ptr;
    
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    MPID_nem_cell_ptr_t v_cell;
    MPID_nem_cell_t *cell; /* non-volatile cell */


    /* can't control VC or stream you are reading from so read until you
     * would block or there are no more cells available in the free queue
     */
    while (!MPID_nem_queue_empty (MPID_nem_sctp_module_free_queue))
    {
        MPID_nem_queue_dequeue (MPID_nem_sctp_module_free_queue, &v_cell);
        cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
        
        if((sz = sctp_recvmsg(MPID_nem_sctp_onetomany_fd,
                        MPID_NEM_CELL_TO_PACKET (cell),
                        MPID_NEM_MAX_PACKET_LEN, &from,
                        &fromlen, &sri, &msg_flags))  == -1)
        {
            if(errno == EAGAIN) {
                /* don't want to waste the cell so enqueue it on the module free Q */
                MPID_nem_queue_enqueue (MPID_nem_sctp_module_free_queue, v_cell);
                break;
            } else { 
                MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_INTERN, "**internrc"); /* FIXME define error code */
            }
        }
        else
        {
            /* received some data */
            
            if(msg_flags != MSG_EOR)
            {
                if(!msg_flags) {
                    /* partial delivery */

                    buf_ptr = partial_msg_buf;
                    if(!partial_msg_in_progress) {
                        /* first time seeing partial read */
                        partial_msg_sz = sz;                        
                        
                    } else {
                        /* the last sctp_recvmsg was a partial read too */
                        buf_ptr += partial_msg_sz;
                        partial_msg_sz += sz;
                    }
                    /* cp to partial message buffer */
                    memcpy(buf_ptr, MPID_NEM_CELL_TO_PACKET (cell), sz);
                    buf_ptr += sz; /* for the case when MSG_EOR is found */
                    partial_msg_in_progress++;

                    /* don't want to waste the cell so enqueue it on the module free Q */
                    MPID_nem_queue_enqueue (MPID_nem_sctp_module_free_queue, v_cell);
                    
                } else {
                    /* unknown msg_flags */
                    MPIU_ERR_SETFATALANDJUMP1(mpi_errno, MPI_ERR_INTERN,
                                               "**internrc",
                                               "msg_flags for recv is %d",
                                               msg_flags); /* FIXME create error code */
                }
            }
            else
            {
                /* standard case (w/ MSG_EOR) */

                if(partial_msg_in_progress) {
                    /* MSG_EOR finally found */
                    /* need to assemble full msg in buf then cp back to cell */
                    memcpy(buf_ptr, MPID_NEM_CELL_TO_PACKET (cell), sz);
                    memcpy(MPID_NEM_CELL_TO_PACKET (cell), partial_msg_buf,
                           partial_msg_sz + sz);
                    partial_msg_in_progress = 0;
                    partial_msg_sz = 0;
                }
                
                /* Look at association ID to see if this is
                 *  a new association.
                 */
                if((result = hash_find(MPID_nem_sctp_assocID_table, 
                                       (int4) sri.sinfo_assoc_id)) == NULL)
                {
                    /* new association  */
                    
                    result = &lresult;
                    result->assoc_id = sri.sinfo_assoc_id;

                    /* only one stream in initial design */

                    /* get PG from the connection packet */
                    MPIDI_PG_t * pg;
                    MPIDI_VC_t * vc;
                    char * data_ptr = (char *) MPID_NEM_CELL_TO_PACKET (cell);
                        
                    /* a new association so the first message must contain the pg_id, or
                     *  our protocol has been broken...
                     */

                    data_ptr += MPID_NEM_MPICH2_HEAD_LEN;
                        
                        
                    /* read pg_id from data_ptr and lookup VC using pg_id */
                    mpi_errno = MPIDI_PG_Find(data_ptr, &pg);
                    if (pg == NULL) {
                        MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
                                             "**pglookup",
                                             "**pglookup %s", data_ptr);
                    }
		
                    MPIDI_PG_Get_vc(pg, MPID_NEM_CELL_SOURCE(cell), &vc);
                    result->vc = vc;
                    ((MPIDI_CH3I_VC *)vc->channel_private)->state = MPID_NEM_VC_STATE_CONNECTED;
                    
                    /* Record that connection pkt has arrived.  */
                    VC_FIELD(vc, stream_table)[sri.sinfo_stream].have_recv_pg_id = HAVE_RECV_PG_ID;
                    
                    /* insert fully populated entry */
                    hash_insert(MPID_nem_sctp_assocID_table, result);

                    /* upper layer doesn't need to handle this since it is
                     *  only for SCTP net module to figure out the VC
                     */
                    MPID_nem_queue_enqueue (MPID_nem_sctp_module_free_queue, v_cell);
                }
                else
                {
                    /* seen this assocID before */

                    /* TODO In a multi-stream design, data may be a connection
                     *  pkt arriving on an existing association but a new stream.
                     */

                    /* TODO Implement accept side of dynamic processes based on this
                     *        pseudo-code.
                     */
                    /* if recv'd from (accept) control stream
                     *     malloc MPIDI_VC_t
                     *     MPIDI_VC_Init(vc, NULL, 0) and other init...
                     *     extract port_name_tag from msg
                     *     extract business_card from msg, set VC_FIELD(vc, to_address)
                     *     open new socket, set to VC_FIELD(vc, fd)
                     *     construct new business_card based on VC_FIELD(vc, fd)
                     *     prepare to send new business_card
                     *     ACK new business_card
                     *     MPIDI_CH3I_Acceptq_enqueue(vc, port_name_tag);
                     *     MPIDI_CH3_Progress_signal_completion();
                     */
                    
                    /* else
                     *     put cell on the process recv queue */
                    MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, v_cell);                    
                }                
            }
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_sctp_module_poll (MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;

/*     mpi_errno = send_progress(); */
    mpi_errno = MPID_nem_sctp_module_send_progress();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = recv_progress();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Channel_close
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Channel_close( void )
{
    /* When called, Outstanding_close_ops in ch3u_handle_connection should be zero */
    /* WARNING! : Outstanding_close_ops can be zero prematurely if MPI_Comm_disconnect
     *              is called.
     */
    int mpi_errno = MPI_SUCCESS;
    
    if(MPIDI_CH3I_dynamic_tmp_vc) {
        /*  be sure to not close prematurely when a close pkt is received from
         *  a recently disconnected VC on the main onetomany socket
         */
        if(MPIDI_CH3I_dynamic_tmp_vc->state == MPIDI_VC_STATE_INACTIVE) {
            
        /* when using a tmp VC for dynamic processes, close the socket
         *  and reset variables since we only do one connect/accept pair at a time.
         */
            close(MPIDI_CH3I_dynamic_tmp_fd); /* FIXME check for errors */
            MPIDI_CH3I_dynamic_tmp_vc = NULL;
            MPIDI_CH3I_dynamic_tmp_fd = -1;
        }
    }
    
}
