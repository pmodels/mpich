/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sctp_module_impl.h"

#define NUM_PREALLOC_SENDQ 10
/* int errno_save; */

struct {MPIDI_VC_t *head;} send_list = {0};
struct {MPID_nem_sctp_module_send_q_element_t *head, *tail;} free_buffers = {0};

#define ALLOC_Q_ELEMENT(e) do {                                                                                                         \
        if (Q_EMPTY (free_buffers))                                                                                                     \
        {                                                                                                                               \
            MPIU_CHKPMEM_MALLOC (*(e), MPID_nem_sctp_module_send_q_element_t *, sizeof(MPID_nem_sctp_module_send_q_element_t),      \
                                 mpi_errno, "send queue element");                                                                      \
        }                                                                                                                               \
        else                                                                                                                            \
        {                                                                                                                               \
            Q_DEQUEUE (&free_buffers, e);                                                                                               \
        }                                                                                                                               \
    } while (0)

/* FREE_Q_ELEMENTS() frees a list if elements starting at e0 through e1 */
#define FREE_Q_ELEMENTS(e0, e1) Q_ENQUEUE_MULTIPLE (&free_buffers, e0, e1)
#define FREE_Q_ELEMENT(e) Q_ENQUEUE (&free_buffers, e) 


static MPID_nem_pkt_t conn_pkt;
static MPID_nem_pkt_t *conn_pkt_p = &conn_pkt;
static int conn_pkt_is_set = 0;

/* common sctp send code */
#undef FUNCNAME
#define FUNCNAME send_sctp_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_sctp_pkt(MPIDI_VC_t *vc, MPID_nem_pkt_t *pkt, int stream, int *sent)
{
    int mpi_errno = MPI_SUCCESS;
    int len;
    int ret;
/*     int errno_save; */
    
    *sent = 0; /* glass half empty */

    /* need to find out if we have to send the connection packet for this stream */
    
    if(VC_FIELD(vc, stream_table)[stream].have_sent_pg_id == HAVE_NOT_SENT_PG_ID) {        

        /* send connection pkt */

        conn_pkt_p->mpich2.dest = pkt->mpich2.dest;

        if(!conn_pkt_is_set)
        {
            /* initialize connection pkt */

            len = (int) strlen(MPIDI_Process.my_pg->id);
            conn_pkt_p->mpich2.datalen = len + 1;
            conn_pkt_p->mpich2.source  = MPID_nem_mem_region.rank;

            /* TODO make sure MPID_NEM_MPICH2_DATA_LEN > strlen(pg_id) */
            /*    FYI  as I type this, this is 64K bytes... */

            memset(MPID_NEM_PACKET_PAYLOAD(conn_pkt_p), 0, MPID_NEM_MPICH2_DATA_LEN);
            memcpy(MPID_NEM_PACKET_PAYLOAD(conn_pkt_p), MPIDI_Process.my_pg->id, len );

            conn_pkt_is_set++;                
        }

        ret = sctp_sendmsg(VC_FIELD(vc, fd), conn_pkt_p, 
                           MPID_NEM_PACKET_LEN (conn_pkt_p),
                           (struct sockaddr *) &(VC_FIELD(vc, to_address)), sizeof(struct sockaddr_in),
                           0, 0, stream, 0, 0);        

        if(ret == -1) {
/*             errno_save = errno; */
            if(errno != EAGAIN) {  /* TODO might need to modify this */
/*                 printf("conn_pkt sctp_sendmsg problem (errno %d)\n", errno_save); */
                MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_INTERN, "**internrc"); /* FIXME define error code */
            }
            goto fn_exit;
        }

        /* update that the connection packet was sent */
        VC_FIELD(vc, stream_table)[stream].have_sent_pg_id = HAVE_SENT_PG_ID;        
    }

    /* send pkt */
    ret = sctp_sendmsg(VC_FIELD(vc, fd), pkt, MPID_NEM_PACKET_LEN (pkt),
                       (struct sockaddr *)&(VC_FIELD(vc, to_address)), sizeof(struct sockaddr_in),
                       0, 0, stream, 0, 0);
    if(ret == -1) {
/*         errno_save = errno; */
        if(errno != EAGAIN) {
/*             printf("data_pkt sctp_sendmsg problem (errno %d)\n", errno_save); */
            MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_INTERN, "**internrc"); /* FIXME define error code */
        }
        goto fn_exit;
    } else {
        *sent = 1;
    }
    
 fn_exit:
 fn_fail:
    return mpi_errno;    
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_send_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_sctp_module_send_finalize()
{
    int mpi_errno = MPI_SUCCESS;

    /* TODO free_buffers sendq elements */
    
 fn_exit:
 fn_fail:
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_send_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_sctp_module_send_init()
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPID_nem_sctp_module_send_q_element_t *sendq_e;
    MPIU_CHKPMEM_DECL (1);
    
    /* preallocate sendq elements */
    MPIU_CHKPMEM_MALLOC (sendq_e, MPID_nem_sctp_module_send_q_element_t *, NUM_PREALLOC_SENDQ * sizeof(MPID_nem_sctp_module_send_q_element_t), mpi_errno,
                         "send queue element");
    for (i = 0; i < NUM_PREALLOC_SENDQ; ++i)
    {
        Q_ENQUEUE (&free_buffers, &sendq_e[i]);
    }

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_sctp_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    int mpi_errno = MPI_SUCCESS;
    int sent;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    int stream = 0; /* single stream implementation! need MPID_NEM_CELL_xxx
                     *  to get stream (and maybe context and maybe tag)
                     */
    int ret;
    int len;
/*     int errno_save; */
    MPID_nem_pkt_t *pkt;
    MPID_nem_sctp_module_send_q_element_t *e;
    MPIU_CHKPMEM_DECL(1);
    
    pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */

    /* update pkt fields */
    pkt->mpich2.datalen = datalen;
    pkt->mpich2.source  = MPID_nem_mem_region.rank;

    /* determine what to send.  this could be either:
     *       - a connection packet - if a stream hasn't sent it'd PG yet
     *       - a previous packet - if a previous attempt caused EAGAIN
     *       - this pkt - if nothing else was sent prior
     */

    if (!Q_EMPTY (vc_ch->send_queue))
    {
        /* will send previous packet or connection pkt (interface hides this) */
        MPID_nem_sctp_module_send_queue (vc); /* try to empty the queue */
        if (!Q_EMPTY (vc_ch->send_queue))  /* didn't empty Q so enqueue cell & exit */
        {
            goto enqueue_cell_and_exit;
        }
    }

    
    /* will send either this pkt or connection pkt (interface hides this)  */
    mpi_errno = send_sctp_pkt(vc, pkt, stream, &sent);
    if(mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    if(!sent) {
        VC_L_ADD (&send_list, vc);
        goto enqueue_cell_and_exit; /* didn't send so add to Q to send later */
    }


    /* pre-sendQ code*/
    
/*     if(VC_FIELD(vc, stream_table)[stream].have_sent_pg_id == HAVE_NOT_SENT_PG_ID) {         */

/*         /\* send connection pkt *\/ */

/*         if(!conn_pkt_is_set) */
/*         { */
/*             /\* initialize connection pkt *\/ */

/*             len = (int) strlen(MPIDI_Process.my_pg->id); */
/*             conn_pkt_p->mpich2.datalen = len + 1; */
/*             conn_pkt_p->mpich2.source  = MPID_nem_mem_region.rank; */
/*             conn_pkt_p->mpich2.dest = pkt->mpich2.dest;  /\* needs to be set more than once! *\/ */

/*             /\* TODO make sure MPID_NEM_MPICH2_DATA_LEN > strlen(pg_id) *\/ */
/*             /\*    FYI  as I type this, this is 64K bytes... *\/ */

/*             memset(MPID_NEM_PACKET_PAYLOAD(conn_pkt_p), 0, MPID_NEM_MPICH2_DATA_LEN); */
/*             memcpy(MPID_NEM_PACKET_PAYLOAD(conn_pkt_p), MPIDI_Process.my_pg->id, len ); */

/*             conn_pkt_is_set++;                 */
/*         } */

/*         ret = sctp_sendmsg(VC_FIELD(vc, fd), conn_pkt_p,  */
/*                            MPID_NEM_PACKET_LEN (conn_pkt_p), */
/*                            (struct sockaddr *) &(VC_FIELD(vc, to_address)), sizeof(struct sockaddr_in), */
/*                            0, 0, stream, 0, 0);         */

/*         errno_save = errno; */
/*         if(ret == -1) { */
/*             /\*  if unsuccessful, need to maintain a send Q.  */
/*              *   connection packet is  */
/*              *\/ */
/*             perror("sctp_sendmsg conn_pkt"); /\* TODO check error codes *\/ */
/*             printf("errno_save is %d\n", errno_save); */
/*         } */

        
/*         VC_FIELD(vc, stream_table)[stream].have_sent_pg_id = HAVE_SENT_PG_ID;         */
/*     } */

/*     /\* send cell *\/ */
/*     ret = sctp_sendmsg(VC_FIELD(vc, fd), pkt, MPID_NEM_PACKET_LEN (pkt), */
/*                        (struct sockaddr *)&(VC_FIELD(vc, to_address)), sizeof(struct sockaddr_in), */
/*                        0, 0, stream, 0, 0); */
/*     errno_save = errno; */
/*     if(ret == -1) { */
/*         /\* TODO if unsuccessful, need to maintain a send Q *\/ */
/*         perror("sctp_sendmsg data"); /\* TODO check error codes *\/ */
/*         printf("errno_save is %d\n", errno_save); */
/*     } else { */
/*         /\* it was sent successfully so enqueue the cell back to the free_queue *\/ */
/*         MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell); */
/*     } */
    
    
 fn_exit:
    MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell);    
    MPIU_CHKPMEM_COMMIT();    
    return mpi_errno;
  enqueue_cell_and_exit:
    /* enqueue cell on VC's send queue and exit */
    ALLOC_Q_ELEMENT (&e);
    MPID_NEM_MEMCPY (&e->buf, pkt, MPID_NEM_PACKET_LEN (pkt));
    e->len = MPID_NEM_PACKET_LEN (pkt);
    e->start = e->buf;
    e->stream = stream;
    Q_ENQUEUE (&vc_ch->send_queue, e);
    goto fn_exit;   
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell);    /* tmp fix */
/*     if(errno_save != ECHRNG && errno_save != EPERM)   printf("errno is %d\n", errno_save); */
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_send_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_sctp_module_send_queue (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    int sent;
    MPID_nem_pkt_t *pkt;
    int count;
    MPID_nem_sctp_module_send_q_element_t *e, *e_first, *e_last;

    if (Q_EMPTY (vc_ch->send_queue))
        goto fn_exit;


    /* walk through VC's send_queue calling sctp_sendmsg */
    

    count = 0;
    e_last = NULL;
    e = Q_HEAD (vc_ch->send_queue);
    e_first = e;
    do
    {
        pkt = (MPID_nem_pkt_t *) e->start;
        mpi_errno = send_sctp_pkt(vc, pkt, e->stream, &sent);
        if(mpi_errno != MPI_SUCCESS)
            goto fn_fail;
        if(!sent)
            break;

        count++;
        e_last = e;
        e = e->next;
    }
    while (e);


    /* remove pending sends that were sent */

    if (count == 0) /* nothing was sent.  e_last should be NULL. e_first should equal e */
        goto fn_exit;

    Q_REMOVE_ELEMENTS (&vc_ch->send_queue, e_first, e_last);
    FREE_Q_ELEMENTS (e_first, e_last);

    if (Q_EMPTY (vc_ch->send_queue)) /* everything was sent. e should be NULL*/
    {
        VC_L_REMOVE (&send_list, vc);
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_sctp_module_send_progress()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    
    for (vc = send_list.head; vc; vc = ((MPIDI_CH3I_VC *)vc->channel_private)->next)
    {
        mpi_errno = MPID_nem_sctp_module_send_queue (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
