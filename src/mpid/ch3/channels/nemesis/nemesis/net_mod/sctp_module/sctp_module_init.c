/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sctp_module_impl.h"

MPID_nem_netmod_funcs_t MPIDI_nem_sctp_module_funcs = {
    MPID_nem_sctp_module_init,
    MPID_nem_sctp_module_finalize,
    MPID_nem_sctp_module_ckpt_shutdown,
    MPID_nem_sctp_module_poll,
    MPID_nem_sctp_module_send,
    MPID_nem_sctp_module_get_business_card,
    MPID_nem_sctp_module_connect_to_root,
    MPID_nem_sctp_module_vc_init,
    MPID_nem_sctp_module_vc_destroy,
    MPID_nem_sctp_module_vc_terminate
};


MPID_nem_queue_ptr_t MPID_nem_sctp_module_free_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;

static MPID_nem_queue_t _free_queue;


static MPID_nem_sctp_socket_bufsz = 0;
int MPID_nem_sctp_onetomany_fd = -1;
int MPID_nem_sctp_port = 0;
HASH* MPID_nem_sctp_assocID_table;
MPIDI_VC_t * MPIDI_CH3I_dynamic_tmp_vc;
int MPIDI_CH3I_dynamic_tmp_fd;

#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_ADDR_KEY "addr"


/*
 * sctp_open_dgm_socket
 * Parameterized constructor of SCTP UDP style socket
 *
 * num_stream       = number of streams per association
 * block_mode       = boolean for if the socket should be blocking
 * listen_back_log
 * port             = port it will attempt to locally bind to
 * nagle            = boolean for if Nagle algorith should be off
 * buffer_size      = socket send/recv buffers sizes
 * evnts            = which events the should be assigned to this socket
 * fd               = the resulting file descriptor
 * real_port        = the port actually binded on (if port is occupied)
 */
#undef FUNCNAME
#define FUNCNAME sctp_open_dgm_socket
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int sctp_open_dgm_socket(int num_stream, int block_mode,
			 int listen_back_log, int port, int no_nagle,
			 int* buffer_size, struct sctp_event_subscribe* evnts,
			 int* fd, int* real_port) {

  struct sctp_initmsg initm;
  struct sockaddr_in addr;
  int flags, rc;
  socklen_t len;

  /* init fd */
  *fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
  if(*fd == -1)
    goto fn_fail;

  /* init num streams */
  initm.sinit_num_ostreams = num_stream;
  initm.sinit_max_instreams = num_stream;
  if (setsockopt(*fd, IPPROTO_SCTP, SCTP_INITMSG, &initm, sizeof(initm))) {
    perror("setsockopt error SCTP_INITMSG");
    goto fn_fail;
  }
  
  /* set blocking mode, default block */
  flags = fcntl(*fd, F_GETFL, 0);
  if (flags == -1) {
    goto fn_fail;
  }

  if(!block_mode) {
    rc = fcntl(*fd, F_SETFL, flags | O_NONBLOCK);
    if(rc == -1)
      goto fn_fail;
  }

  /* bind addr & port */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;

  /* if you want to bind to a particular interface */
  char* env;
  env = getenv("MPICH_INTERFACE_HOSTNAME");
  if (env != NULL && *env != '\0') {
    addr.sin_addr.s_addr = inet_addr(env);
  } else 
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* TODO add support for binding to particular port */
  addr.sin_port = htons((unsigned short) port);
  rc = bind(*fd, (struct sockaddr *) &addr, sizeof(addr));
  
  if(rc == -1) {
    goto fn_fail;
  }

/*   /\* selectively bind interfaces from a file? *\/ */
/*   env = getenv("MPICH_SCTP_MULTIHOME_FILE"); */
/*   if (env != NULL && *env != '\0') { */
/*     rc = bind_from_file(env, *fd, port); */
/*     if(rc == -1) */
/*       goto fn_fail; */
/*   }  */
 
  /* set up listen back log */
  rc = listen(*fd, listen_back_log);
  if (rc == -1) {
    goto fn_fail;
  }

  /* retrieve port value if wasn't specified */
  if(!port){
    /* Can reuse addr since bind already succeeded. */
    len = sizeof(addr);
    if (getsockname(*fd, (struct sockaddr *) &addr, &len)) {
	goto fn_fail;
    }
    
    *real_port = (int) ntohs(addr.sin_port);
  } else 
    *real_port = port;

  /* register events */
  if (setsockopt(*fd, IPPROTO_SCTP, SCTP_EVENTS, evnts, sizeof(*evnts))) {
    perror("setsockopt error SCTP_EVENTS");
    goto fn_fail;
  }

  /* set nagle */
  if (setsockopt(*fd, IPPROTO_SCTP,
		 SCTP_NODELAY, (char *) &no_nagle, sizeof(no_nagle))){
    goto fn_fail;
  }

  /* set buffer size if specified */
  if(*buffer_size > 0) {
    int bufsz;
    socklen_t bufsz_len;
    
    bufsz = *buffer_size;
    bufsz_len = sizeof(bufsz);
    rc = setsockopt(*fd, SOL_SOCKET, SO_SNDBUF, &bufsz, bufsz_len);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1) {
      goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    
    bufsz = *buffer_size;
    bufsz_len = sizeof(bufsz);
    rc = setsockopt(*fd, SOL_SOCKET, SO_RCVBUF, &bufsz, bufsz_len);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1) {
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    
    bufsz_len = sizeof(bufsz);
    rc = getsockopt(*fd, SOL_SOCKET, SO_SNDBUF, &bufsz, &bufsz_len);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == 0) {
      if (bufsz < *buffer_size * 0.9 || bufsz < *buffer_size * 1.0){
	  MPIDI_DBG_PRINTF((50, FCNAME,"WARNING: send socket buffer size differs from requested size (requested=%d, actual=%d)\n",
			  buffer_size, bufsz));
      }
    }
    /* --END ERROR HANDLING-- */
    
    bufsz_len = sizeof(bufsz);
    rc = getsockopt(*fd, SOL_SOCKET, SO_RCVBUF, &bufsz, &bufsz_len);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == 0) {
      if (bufsz < *buffer_size * 0.9 || bufsz < *buffer_size * 1.0) {
	MPIDI_DBG_PRINTF((50, FCNAME,"WARNING: receive socket buffer size differs from requested size (requested=%d, actual=%d)\n",
	       buffer_size, bufsz));
      }
      *buffer_size = bufsz;
    }
    /* --END ERROR HANDLING-- */
  }

 fn_exit:
  return 1;
  
 fn_fail:
  return -1;
}


#undef FUNCNAME
#define FUNCNAME init_sctp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int init_sctp (MPIDI_PG_t *pg_p) 
{
    int           mpi_errno = MPI_SUCCESS;
    int           stream, rank;
    int           no_nagle = 1;
    int           port = 0;
    struct sctp_event_subscribe evnts;
    char          *env;

    /* turn off all event except send/recv info */
    bzero(&evnts, sizeof(evnts));
    evnts.sctp_data_io_event=1;

    /* get socket buffers sizes */
    env = getenv("MPICH_SCTP_BUFFER_SIZE");
    if (env)
    {
        int tmp;
        
        /* FIXME: atoi doesn't detect errors (e.g., non-digits) */
        tmp = atoi(env);
        if (tmp > 0)
	{
            MPID_nem_sctp_socket_bufsz = tmp;
	}
    } else {
        MPID_nem_sctp_socket_bufsz = _MPICH_SCTP_SOCKET_BUFSZ;
    }

    /* setup one-to-many socket */
    if(sctp_open_dgm_socket(MPICH_SCTP_NUM_STREAMS,
                            0, 5, port, no_nagle,
                            &MPID_nem_sctp_socket_bufsz,                            
                            &evnts, &MPID_nem_sctp_onetomany_fd,
                            &MPID_nem_sctp_port) == -1) {
        MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_INTERN, "**internrc"); /* FIXME define error code */
    }

    /* setup association ID -> VC hash table */
    MPID_nem_sctp_assocID_table = hash_init(MPIDI_PG_Get_size(pg_p),
                                            sizeof(MPID_nem_sctp_hash_entry),
                                            INT4_MAX, 0);
    if(MPID_nem_sctp_assocID_table == 0)
    {
        MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_INTERN, "**nomem") ;  /* FIXME define error code */
    }    

    /* initialize variables for dynamic processes */
    MPIDI_CH3I_dynamic_tmp_vc = NULL;
    MPIDI_CH3I_dynamic_tmp_fd = -1;
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    if (MPID_nem_sctp_onetomany_fd != -1)
    {
        printf("closing sctp fd!\n");
	close(MPID_nem_sctp_onetomany_fd);
    }

    goto fn_exit;
}


/*
   int  
   MPID_nem_sctp_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
                            MPID_nem_cell_ptr_t module_elements, int num_module_elements, MPID_nem_queue_ptr_t *module_recv_queue,
                            MPID_nem_queue_ptr_t *module_free_queue)

   IN
       proc_recv_queue -- main recv queue for the process
       proc_free_queue -- main free queueu for the process
       proc_elements -- pointer to the process' queue elements
       num_proc_elements -- number of process' queue elements
       module_elements -- pointer to queue elements to be used by this module
       num_module_elements -- number of queue elements for this module
       ckpt_restart -- true if this is a restart from a checkpoint.  In a restart, the network needs to be brought up again, but
                       we want to keep things like sequence numbers.
   OUT
   
       recv_queue -- pointer to the recv queue for this module.  The process will add elements to this
                     queue for the module to send
       free_queue -- pointer to the free queue for this module.  The process will return elements to
                     this queue
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_sctp_module_init (MPID_nem_queue_ptr_t  proc_recv_queue, 
			  MPID_nem_queue_ptr_t  proc_free_queue, 
			  MPID_nem_cell_ptr_t    proc_elements,   
			  int num_proc_elements,
			  MPID_nem_cell_ptr_t    module_elements, 
			  int num_module_elements, 
			  MPID_nem_queue_ptr_t *module_free_queue,
			  int ckpt_restart, MPIDI_PG_t *pg_p, int pg_rank,
			  char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int index;

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_sctp_module_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);
    
    mpi_errno = init_sctp (pg_p);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* create business card. published in the layer above? */
    mpi_errno = MPID_nem_sctp_module_get_business_card (pg_rank, bc_val_p, val_max_sz_p);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);


    /* save references to queues */
    MPID_nem_process_recv_queue = proc_recv_queue;
    MPID_nem_process_free_queue = proc_free_queue;

    MPID_nem_sctp_module_free_queue = &_free_queue;

    /* set up network module queues */
    MPID_nem_queue_init (MPID_nem_sctp_module_free_queue);

    for (index = 0; index < num_module_elements; ++index)
    {
	MPID_nem_queue_enqueue (MPID_nem_sctp_module_free_queue, &module_elements[index]);
    }

    *module_free_queue = MPID_nem_sctp_module_free_queue;


 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_sctp_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;

    mpi_errno = MPIU_Str_add_string_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_ADDR_KEY, MPID_nem_hostname);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }

    
    mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, MPID_nem_sctp_port);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_sctp_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);

    /* TODO fill in connect_to_root body based on this pseudocode */
    
    /* assert that new_vc->pg == NULL */
    /* assert that VC_FIELD(new_vc, fd) == -1 */
    /* create new socket for VC_FIELD(new_vc, fd) */
    /* MPIDI_CH3I_dynamic_tmp_vc = new_vc; */
    /* MPIDI_CH3I_dynamic_tmp_fd = VC_FIELD(new_vc, fd); */
    /* obtain to_address from business_card contents */
    /* create business_card for new socket */
    /* prepare sending the business card to the server */
    /* write business card to the server on a control stream specific for accept */
    /* block on tmp_fd until server ACKs */
    /* set to_address based on contents of the ACK */
    
    return mpi_errno;
}

/* MPIDI_CH3_CHANNEL_AVOIDS_SELECT is defined so we need this for dynamic processes */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Complete_Acceptq_dequeue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPIDI_CH3_Complete_Acceptq_dequeue (MPIDI_VC_t *vc) {

    if(vc != NULL) {
        MPIU_Assert(MPIDI_CH3I_dynamic_tmp_vc == NULL);
        
        MPIDI_CH3I_dynamic_tmp_vc = vc;
        MPIDI_CH3I_dynamic_tmp_fd = VC_FIELD(vc, fd);
    }
    
    return MPI_SUCCESS;
}


/* build sockaddr_in based on hostname and port in the bizcard */
#undef FUNCNAME
#define FUNCNAME get_sockaddr_in_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int get_sockaddr_in_from_bc (const char *business_card, struct sockaddr_in *addr)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int len;
    int port;
    int MAX_HOST_NAME=1024;
    char host[MAX_HOST_NAME];

    
    /* for getting ifname from DNS rather than business card */
    struct hostent *info;
    char ifname[256];
    unsigned char *p;
    unsigned char ifaddr[4];

   
    ret = MPIU_Str_get_string_arg (business_card, MPIDI_CH3I_ADDR_KEY, host, MAX_HOST_NAME);
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");

    mpi_errno = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_PORT_KEY, &port);
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingport");

    /* create sockaddr_in */
    
    /* lookup hostname and IP */
    info = gethostbyname( host ); 
    /* extract string of IP from hostent */
    if (info && info->h_addr_list) {        
        p = (unsigned char *)(info->h_addr_list[0]);
        MPIU_Snprintf( ifname, sizeof(ifname), "%u.%u.%u.%u", 
                       p[0], p[1], p[2], p[3] );
    } else { 
       MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_INTERN, "**internrc"); /* FIXME create error code */
    }
    
    /* create ifaddr */
    int rc = inet_pton( AF_INET, (const char *)ifname, ifaddr );        
    if (rc == 0) {
        /* ifname was not valid */
        MPIU_ERR_SETFATALANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**internrc", "inet_pton = %d", rc); /* FIXME create error code */
    }
    else if (rc < 0) {
        /* af_inet not supported */
        MPIU_ERR_SETFATALANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**internrc", "inet_pton < 0 (%d)", rc); /* FIXME create error code */
    }
    
    /* setup final sockaddr_in */
    memset(addr, 0, sizeof(addr));
    addr->sin_family = AF_INET;
    memcpy(&(addr->sin_addr.s_addr), ifaddr, sizeof(addr->sin_addr.s_addr));
    addr->sin_port = htons(port);
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_sctp_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    int i;
    int mpi_errno = MPI_SUCCESS;
    
    /* initially disconnected (doing lazy connect).  "connected" means has
     *  received on any stream (NOT sent because you don't get the assocID
     *  until you receive).
     */
    ((MPIDI_CH3I_VC *)vc->channel_private)->state = MPID_NEM_VC_STATE_DISCONNECTED;
    
    /* set all streams to initial stage (i.e. before the connection
     *  packet containing the PG has arrived)
     */
    for(i = 0; i < MPICH_SCTP_NUM_STREAMS; i++) {
        VC_FIELD(vc, stream_table)[i].have_sent_pg_id = HAVE_NOT_SENT_PG_ID;
        VC_FIELD(vc, stream_table)[i].have_recv_pg_id = HAVE_NOT_RECV_PG_ID;
    }

    /* use single one-to-many socket for non-temporary VCs (as in connect/accept) */
    if(vc->pg)
        VC_FIELD(vc, fd) = MPID_nem_sctp_onetomany_fd;
    else
        VC_FIELD(vc, fd) = -1;
    
    /* populate the sockaddr_in based on the business card */
    mpi_errno = get_sockaddr_in_from_bc (business_card, &(VC_FIELD(vc, to_address)));
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;    
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_sctp_module_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;   

    /* free any resources associated with this VC here */

 fn_exit:   
       return mpi_errno;
 fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_sctp_module_vc_terminate (MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}
