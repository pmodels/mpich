/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "sctp_common.h"




void print_SCTP_event(struct MPIDU_Sctp_event * eventp);

int inline MPIDU_Sctp_post_writev(MPIDI_VC_t* vc, MPID_Request* sreq, int offset,
				  MPIDU_Sock_progress_update_func_t fn, int stream_no);


#undef FUNCNAME
#define FUNCNAME adjust_req
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int adjust_req(MPID_Request* rreq, MPIU_Size_t nb) {

  MPID_IOV * iov = rreq-> dev.iov;
  const int count = rreq->dev.iov_count;
  int offset = 0;
  int temp;
  
  return adjust_iov(&iov, &rreq->dev.iov_count, nb);
 
}

#undef FUNCNAME
#define FUNCNAME adjust_iov
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb)
{
    MPID_IOV * const iov = *iovp;
    const int count = *countp;
    int offset = 0;
    int temp;
    
    while (offset < count)
    {
	if (iov[offset].MPID_IOV_LEN <= nb)
	{
	  nb -= iov[offset].MPID_IOV_LEN;
	  iov[offset].MPID_IOV_LEN = 0;
  
	  offset++;
	}
	else
	{
	    iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) 
							   iov[offset].MPID_IOV_BUF + nb);
	    iov[offset].MPID_IOV_LEN -= nb;
	    break;
	}
    }

    *iovp += offset;
    *countp -= offset;

    return (*countp == 0);
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_event_enqueue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sctp_event_enqueue(MPIDU_Sctp_op_t op, MPIU_Size_t num_bytes, 
				    sctp_rcvinfo* sri, int fd, void * user_ptr, void* user_ptr2, int value, int error)
{
  struct MPIDU_Sctp_eventq_elem * eventq_elem;
  struct MPIDU_Sctp_event* new_event;
  int index;
  sctp_rcvinfo bogus;

  int mpi_errno = MPI_SUCCESS;
 
  /*  eventq_head hasn't been initialized yet */
  if(eventq_head == NULL) {
    eventq_head = MPIU_Malloc(sizeof(struct MPIDU_Sctp_eventq_elem));

    if(eventq_head == NULL) {
      MPIDI_DBG_PRINTF((50, FCNAME, "Malloc Failed!\n"));
      mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
                                             __LINE__, MPI_ERR_OTHER,
                                             "**nomem", NULL);
      goto fn_fail;
    }

    eventq_head-> size = 0;
    eventq_head-> head = 0;
    eventq_head-> tail = 0;
    eventq_head-> next = NULL;
    eventq_tail = eventq_head;
  }

  /*  tail is full, need to allocate a new one */
  if(eventq_tail-> size == MPIDU_SCTP_EVENTQ_POOL_SIZE){
    eventq_elem = MPIU_Malloc(sizeof(struct MPIDU_Sctp_eventq_elem));

    if(eventq_elem == NULL) {
      MPIDI_DBG_PRINTF((50, FCNAME, "Malloc Failed!\n"));
      mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
                                             __LINE__, MPI_ERR_OTHER,
                                             "**nomem", NULL);
      goto fn_fail;
    }

    eventq_elem-> size = 0;
    eventq_elem-> head = 0;
    eventq_elem-> tail = 0;
    eventq_elem-> next = NULL;
    eventq_tail->next = eventq_elem;
    eventq_tail = eventq_elem;
  }

  /* put the event in */
  index = eventq_tail->tail;
  new_event = &eventq_tail->event[index];
  new_event-> op_type = op;
  new_event-> num_bytes = num_bytes;
  new_event-> fd = fd;
  new_event-> sri = (sri == NULL)? bogus : *sri;
  new_event-> user_ptr = user_ptr;
  new_event-> user_ptr2 = user_ptr2;
  new_event-> user_value = value;
  new_event-> error = error;

  eventq_tail->size++;
  eventq_tail->tail = (index+1) % MPIDU_SCTP_EVENTQ_POOL_SIZE;

 fn_exit:
 fn_fail:
  return mpi_errno;
}
/* end MPIDU_Socki_event_enqueue() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_event_dequeue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sctp_event_dequeue(struct MPIDU_Sctp_event * eventp)
{
    struct MPIDU_Sctp_eventq_elem* eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    
    /* check if queue is empty */
    if(eventq_head == NULL || eventq_head->size <= 0) {
        mpi_errno--; /* as long as it isn't MPI_SUCCESS (not interpretted otherwise at call) */
        eventp->op_type = 100;
        return mpi_errno;
    }

    *eventp = eventq_head->event[eventq_head->head];
    eventq_head->size--;
    eventq_head->head = (eventq_head->head + 1) % MPIDU_SCTP_EVENTQ_POOL_SIZE;

    /*  free eventq_head if it's empty, however, always maintain at least one */
    if(eventq_head->size == 0 && eventq_tail != eventq_head) {
      eventq_elem = eventq_head;
      
      eventq_head = eventq_head-> next;
      MPIU_Free(eventq_elem);
    }
    
    return mpi_errno;
}
/* end MPIDU_Socki_event_dequeue() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_free_eventq_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void inline MPIDU_Sctp_free_eventq_mem(void)
{
  struct MPIDU_Sctp_eventq_elem* eventq_elem;
  
  while (eventq_head) {
    eventq_elem = eventq_head;
    eventq_head = eventq_head->next;
    MPIU_Free(eventq_elem);
  }

  eventq_head = NULL;
  eventq_tail = NULL;

}

#undef FUNCNAME
#define FUNCNAME create_request
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static MPID_Request* create_request(MPID_IOV * iov, int iov_count, int iov_offset, MPIU_Size_t nb)
{
    MPID_Request * sreq;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST);

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST);
    
    sreq = MPID_Request_create();
    /* --BEGIN ERROR HANDLING-- */
    if (sreq == NULL)
	return NULL;
    /* --END ERROR HANDLING-- */

    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    
    for (i = 0; i < iov_count; i++)
    {
	sreq->dev.iov[i] = iov[i];
    }
    if (iov_offset == 0)
    {
	MPIU_Assert(iov[0].MPID_IOV_LEN == sizeof(MPIDI_CH3_Pkt_t));
	sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) iov[0].MPID_IOV_BUF;
	sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &sreq->dev.pending_pkt;
    }
    sreq->dev.iov[iov_offset].MPID_IOV_BUF = 
	(MPID_IOV_BUF_CAST)((char *) sreq->dev.iov[iov_offset].MPID_IOV_BUF + nb);
    sreq->dev.iov[iov_offset].MPID_IOV_LEN -= nb;
    sreq->dev.iov_count = iov_count;
    sreq->dev.OnDataAvail = 0;

    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST);
    return sreq;
}


/*  
 * PRE-CONDITION: stream state = MPIDI_CH3I_VC_STATE_UNCONNECTED
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_enqueue_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void inline MPIDU_Sctp_stream_init(MPIDI_VC_t* vc, MPID_Request* req, int stream){

  MPID_Request* conn_req = NULL;
  int mpi_errno;

  /* stream hasn't been used yet, need to enqueue a connection packet */
  if(SEND_CONNECTED(vc, stream) != MPIDI_CH3I_VC_STATE_CONNECTING ){
    conn_req = create_request(VC_IOV(vc, stream), 2, 0, 0);

    if(conn_req)
    {
      /*  don't put it in sendQ, but directly in Global_SendQ */
      SEND_CONNECTED(vc, stream) = MPIDI_CH3I_VC_STATE_CONNECTING;

      /* should be the connection request */
      MPIDU_Sctp_post_writev(vc, conn_req, 0, NULL, stream);
      
      
    } else {
      mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);

      /* goto fn_exit; FIXME should return int so error can be passed up */
    }
    
  } 

  /* need to update state for upcalls to close protocol to fully work, and barriers */
  if(vc->state == MPIDI_VC_STATE_INACTIVE)
      MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

  if(req) {
    MPIDI_CH3I_SendQ_enqueue_x(vc, req, stream);
  }
}


/* returns number of bytes written. chunks long messages. writes until
 *  done or it comes across an EAGAIN 
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sctp_writev(MPIDI_VC_t* vc, struct iovec* ldata,int iovcnt, int stream, int ppid, 
		      MPIU_Size_t* nb) {

    return MPIDU_Sctp_writev_fd(vc->ch.fd, &(vc->ch.to_address), ldata, iovcnt, stream, ppid, nb);
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_writev_fd
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sctp_writev_fd(int fd, struct sockaddr_in * to, struct iovec* ldata,
                         int iovcnt, int stream, int ppid, MPIU_Size_t* nb)
{
  int byte_sent, r, nwritten =0, i, sz = 0;
  int mpi_errno = MPI_SUCCESS;
  static struct iovec cdata[MPID_IOV_LIMIT]; /* not thread-safe */
  struct iovec *data = cdata;

  MPIU_Assert(iovcnt > 0);

  MEMCPY(cdata, ldata, sizeof(struct iovec) *iovcnt);

  /* calculate total size */
  for(i = 0; i < iovcnt; i++) sz += ldata[i].iov_len;
  
  do {
    r = 0;
    errno = 0;
    if(sz <= CHUNK) /* vs. MPIDI_CH3_EAGER_MAX_MSG_SIZE? */ {
      /* a short/eager message */
      byte_sent = sz;
      r = sctp_writev(fd, data, iovcnt,
		      (struct sockaddr *) to, sizeof(*to), ppid, 0, stream, 0, 0, sz);
    } else {
      byte_sent = MPIR_MIN(CHUNK,data->iov_len);
      
      MPIU_Assert(byte_sent != 0);

      r = sctp_sendmsg(fd, data->iov_base, byte_sent, (struct sockaddr *) to,
		       sizeof(*to), ppid, 0, stream, 0, 0);

    }
    
    /* update iov's (adjust_iov is static and does not handle errors and "errors" (EAGAIN)) */
    
    if (r <= 0) {  /* error */

      if(errno == EAGAIN || errno == ENOMEM || r == 0)
	break;
      else
	goto fn_fail;

    } else {  /* r > 0 */

      MPIU_Assert(r == byte_sent);
      nwritten += r;
      adjust_iov(&data, &iovcnt, r);
    }
    
  } while(iovcnt > 0);
  
  *nb = nwritten;
  
 fn_exit:
  return (nwritten >= 0)? MPI_SUCCESS : -1;
 fn_fail:
  *nb = nwritten;
  nwritten = -1;
  perror("MPIDU_Sctp_writev");
  goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sctp_write(MPIDI_VC_t* vc, void* buf, MPIU_Size_t len, 
		     int stream_no, int ppid, MPIU_Size_t* num_written){

  int err = MPI_SUCCESS;
  ssize_t nb;
  
  nb = my_sctp_send(vc->ch.fd, buf, len,
                      (struct sockaddr *) &(vc->ch.to_address), stream_no, ppid);    

  *num_written = 0;
  if(nb >= 0)
    *num_written = nb;
  else
    perror("MPIDU_Sctp_write");

  return (nb >= 0)? MPI_SUCCESS: -1;
}

#undef FUNCNAME
#define FUNCNAME print_SCTP_event
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void print_SCTP_event(struct MPIDU_Sctp_event * eventp){
}


/*  readv from advance buffer  */
#undef FUNCNAME
#define FUNCNAME readv_from_advbuf
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int readv_from_advbuf(MPID_Request* req, char* from, int bytes_read) {

  MPID_IOV* iovp = req-> dev.iov;
  int iov_cnt = req-> dev.iov_count;

  int total_size, i, j, read_size, rc;
  total_size = i = j = read_size = rc = 0;

  for(i=0, j=0; j< iov_cnt; i++) {
    if(iovp[i].MPID_IOV_LEN > 0) {
      total_size += iovp[i].MPID_IOV_LEN;
      j++;
    }
  }

  rc = bytes_read = MPIR_MIN(bytes_read, total_size);

  for(;bytes_read > 0;iovp++) {
    
    if(iovp-> MPID_IOV_LEN == 0)
      continue;

    read_size = MPIR_MIN(bytes_read, iovp-> MPID_IOV_LEN);

    MEMCPY(iovp-> MPID_IOV_BUF, from, read_size);

    from += read_size;
    bytes_read -= read_size;
  }

  return rc;
}

#undef FUNCNAME
#define FUNCNAME Req_Stream_from_pkt_and_req
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int Req_Stream_from_pkt_and_req(MPIDI_CH3_Pkt_t * pkt, MPID_Request * sreq)
{
    int stream;
    
    switch(pkt->type)
    {
        /* account for each MPIDI_CH3_Pkt_type */
    case MPIDI_CH3_PKT_EAGERSHORT_SEND:

        /* these types are internally identical */
        case MPIDI_CH3_PKT_EAGER_SEND :
        case MPIDI_CH3_PKT_EAGER_SYNC_SEND :
        case MPIDI_CH3_PKT_READY_SEND :
        case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
        case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND :
        {
	  stream = Req_Stream_from_match(pkt->eager_send.match);
        }
        break;
        
        case MPIDI_CH3_PKT_RNDV_SEND :
        { 
	  stream = Req_Stream_from_match(sreq->dev.match);
        }
        break;
        
        default :
        case MPIDI_CH3_PKT_CLOSE :
        case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND :    /*  CTS has unset values here */
                                                 /*  FIXME : need to be smart about assigning the CTS stream
                                                  *    so it doesn't overlap an already active stream
                                                  */
        {
	  stream = MPICH_SCTP_CTL_STREAM;
        }
        break;
    }
    MPIU_DBG_MSG_D(CH3_CONNECT,VERBOSE,"stream %d returned",stream );

    return stream;        
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_post_writev
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
inline int MPIDU_Sctp_post_writev(MPIDI_VC_t* vc, MPID_Request* sreq, int offset,
				  MPIDU_Sock_progress_update_func_t fn, int stream_no)
{
  MPIU_Assert(offset >= 0);

  int mpi_errno = MPI_SUCCESS;
  MPID_IOV* iov = sreq->dev.iov + offset;
  int iov_n = sreq->dev.iov_count - offset;
  
  Global_SendQ_enqueue(vc, sreq, stream_no);
  
  SCTP_IOV* iov_ptr = &(vc->ch.posted_iov[stream_no]);
  
  POST_IOV(iov_ptr) = iov;
  POST_IOV_CNT(iov_ptr) = iov_n;
  POST_IOV_OFFSET(iov_ptr) = 0;
  POST_IOV_FLAG(iov_ptr) = TRUE;
  
  /* POST_UPDATE_FN(iov_ptr) = fn; */
  
 fn_exit:
  return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_post_write
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
inline int MPIDU_Sctp_post_write(MPIDI_VC_t* vc, MPID_Request* sreq, MPIU_Size_t minlen, 
			  MPIU_Size_t maxlen, MPIDU_Sock_progress_update_func_t fn, int stream_no) {
  
  int mpi_errno = MPI_SUCCESS;
  void* buf = sreq->dev.iov[0].MPID_IOV_BUF;
  
  Global_SendQ_enqueue(vc, sreq, stream_no);

  SCTP_IOV* iov_ptr = &(vc->ch.posted_iov[stream_no]);

  POST_IOV_FLAG(iov_ptr) = FALSE;
  POST_BUF(iov_ptr) = buf;
  POST_BUF_MIN(iov_ptr) = minlen;
  POST_BUF_MAX(iov_ptr) = maxlen;
  
  /* POST_UPDATE_FN(iov_ptr) = fn; */

 fn_exit:
  return mpi_errno;
}


/* BUFFER Management routines */

static BufferNode_t* BufferList = NULL;

#undef FUNCNAME
#define FUNCNAME BufferList_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
inline void BufferList_init(BufferNode_t* node) {
  BufferList = node;
  buf_init(BufferList);
}

#undef FUNCNAME
#define FUNCNAME buf_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
inline int buf_init(BufferNode_t* node) {
  node-> free_space = READ_AMOUNT;
  node-> buf_ptr = node-> buffer;

  node-> next = NULL;
  node-> dynamic = FALSE;

  return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME buf_clean
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
inline int buf_clean(BufferNode_t* node) {
  return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME request_buffer
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
inline char* request_buffer(int size, BufferNode_t** bf_node) {
  BufferNode_t* node = BufferList;
  if(node-> free_space >= size) {
    *bf_node = node;
    return node-> buf_ptr;
  } else { 
    *bf_node = NULL;
    return NULL;
  }
}

#undef FUNCNAME
#define FUNCNAME update_size
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
inline void update_size(BufferNode_t* node, int size) {
  node-> free_space -= size;
  node-> buf_ptr += size;
}

/* END Buffer routines */


/* got rid of sock util so need sctp versions of these definitions */


#define MPIDI_CH3I_HOST_DESCRIPTION_KEY  "description"
#define MPIDI_CH3I_PORT_KEY              "port"
#define MPIDI_CH3I_IFNAME_KEY            "ifname"


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_get_host_description
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Sctp_get_host_description(char * host_description, int len)
{
    char * env_hostname;
    int rc;
    int mpi_errno = MPI_SUCCESS;
   
    /* --BEGIN ERROR HANDLING-- */
    if (len < 0)
    {
      mpi_errno = -1;
      goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* FIXME: Is this documented?  How does it work if the process manager
       cannot give each process a different value for an environment
       name?  What if a different interface is needed? */
    /* Use hostname supplied in environment variable, if it exists */
    env_hostname = getenv("MPICH_INTERFACE_HOSTNAME");
    if (env_hostname != NULL)
    {
	rc = MPIU_Strncpy(host_description, env_hostname, len);
	/* --BEGIN ERROR HANDLING-- */
	if (rc != 0)
	{
	  mpi_errno = -1;
	}
	/* --END ERROR HANDLING-- */
    }
    else {
	rc = gethostname(host_description, len);
	/* --BEGIN ERROR HANDLING-- */
	if (rc == -1)
	{
	  mpi_errno = -1;
	}
	/* --END ERROR HANDLING-- */
    }

  fn_exit:
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Get_business_card_sctp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Get_business_card_sctp(char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;
    int port;
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    
    mpi_errno = MPIDU_Sctp_get_host_description(host_description, MAX_HOST_DESCRIPTION_LEN);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_description");
    }

    port = MPIDI_CH3I_listener_port;
    str_errno = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, port);
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }
    
    str_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description);
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

    /* Look up the interface address cooresponding to this host description */
    /* FIXME: We should start switching to getaddrinfo instead of 
       gethostbyname */
    /* FIXME: We don't make use of the ifname in Windows in order to 
       provide backward compatibility with the (undocumented) host
       description string used by the socket connection routine 
       MPIDU_Sock_post_connect.  We need to change to an interface-address
       (already resolved) based description for better scalability and
       to eliminate reliance on fragile DNS services. Note that this is
       also more scalable, since the DNS server may serialize address 
       requests.  On most systems, asking for the host info of yourself
       is resolved locally (i.e., perfectly parallel).
    */
#ifndef HAVE_WINDOWS_H
    {
	struct hostent *info;
	char ifname[256];
	unsigned char *p;
	info = gethostbyname( host_description );
	if (info && info->h_addr_list) {
	    p = (unsigned char *)(info->h_addr_list[0]);
	    MPIU_Snprintf( ifname, sizeof(ifname), "%u.%u.%u.%u", 
			   p[0], p[1], p[2], p[3] );
	    MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"ifname = %s",ifname );
	    str_errno = MPIU_Str_add_string_arg( bc_val_p, 
						 val_max_sz_p, 
						 MPIDI_CH3I_IFNAME_KEY,
						 ifname );
            if (str_errno) {
                MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
            }
	}
    }
#endif
 fn_exit:
    
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_get_conninfo_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sctp_get_conninfo_from_bc( const char *bc, 
				     char *host_description, int maxlen,
				     int *port, MPIDU_Sock_ifaddr_t *ifaddr, 
				     int *hasIfaddr )
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno;
#if !defined(HAVE_WINDOWS_H) && defined(HAVE_INET_PTON)
    char ifname[256];
#endif
    
    str_errno = MPIU_Str_get_string_arg(bc, MPIDI_CH3I_HOST_DESCRIPTION_KEY, 
				 host_description, maxlen);
    if (str_errno != MPIU_STR_SUCCESS) {
	/* --BEGIN ERROR HANDLING */
	if (str_errno == MPIU_STR_FAIL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**argstr_missinghost");
	}
	else {
	    /* MPIU_STR_TRUNCATED or MPIU_STR_NONEM */
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
	}
	/* --END ERROR HANDLING-- */
    }
    str_errno = MPIU_Str_get_int_arg(bc, MPIDI_CH3I_PORT_KEY, port);
    if (str_errno != MPIU_STR_SUCCESS) {
	/* --BEGIN ERROR HANDLING */
	if (str_errno == MPIU_STR_FAIL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_missingport");
	}
	else {
	    /* MPIU_STR_TRUNCATED or MPIU_STR_NONEM */
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port");
	}
	/* --END ERROR HANDLING-- */
    }
    /* ifname is optional */
    /* FIXME: This is a hack to allow Windows to continue to use
       the host description string instead of the interface address
       bytes when posting a socket connection.  This should be fixed 
       by changing the Sock_post_connect to only accept interface
       address.  Note also that Windows does not have the inet_pton 
       routine; the Windows version of this routine will need to 
       be identified or written.  See also channels/sock/ch3_progress.c */
    *hasIfaddr = 0;
#if !defined(HAVE_WINDOWS_H) && defined(HAVE_INET_PTON)
    str_errno = MPIU_Str_get_string_arg(bc, MPIDI_CH3I_IFNAME_KEY, 
					ifname, sizeof(ifname) );
    if (str_errno == MPIU_STR_SUCCESS) {
	/* Convert ifname into 4-byte ip address */
	/* Use AF_INET6 for IPv6 (inet_pton may still be used).
	   An address with more than 3 :'s is an IPv6 address */
	
	int rc = inet_pton( AF_INET, (const char *)ifname, ifaddr->ifaddr );
	if (rc == 0) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ifnameinvalid");
	}
	else if (rc < 0) {
	    /* af_inet not supported */
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**afinetinvalid");
	}
	else {
	    /* Success */
	    *hasIfaddr = 1;
	    ifaddr->len = 4;  /* IPv4 address */
	    ifaddr->type = AF_INET;
	}
    }
#endif
    
 fn_exit:
    
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
