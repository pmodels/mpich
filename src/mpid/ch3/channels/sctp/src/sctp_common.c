/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include "sctp_common.h"
#include "mpidi_ch3_impl.h"

/* 
 * sctp_open_dgm_socket
 */

/* 
 * sctp_open_dgn_socket()
 * Opens a generic datagram (UDP) SCTP socket
 */
#undef FUNCNAME
#define FUNCNAME sctp_open_dgm_socket
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int sctp_open_dgm_socket() {
  return socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
}

#undef FUNCNAME
#define FUNCNAME buf_size
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int buf_size(int fd) {
  int bufsz;
  socklen_t bufsz_len = sizeof(bufsz);

  getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, &bufsz_len);

  return bufsz;
}

/*
 * bind_from_file (For multi-homing)
 * This function tries to bind additional IPs from a file
 */
#undef FUNCNAME
#define FUNCNAME bind_from_file
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int bind_from_file(char* fName, int sockfd, int port) {
  int rc = MPI_SUCCESS;
  FILE* fptr = NULL;
  char emName[100];
  char emIP[16];
  char hostName[100];
  struct sockaddr_in addr;

  if((fptr = fopen(fName, "r")) == NULL) 
    goto fn_error; 

  if(gethostname(hostName, 100) < 0)
    goto fn_error;

  while(fscanf(fptr, "%s %s", emName, emIP) != EOF) {

    if(strcmp(hostName, emName) == 0) {
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr(emIP);
      addr.sin_port = htons((unsigned short) port);

#ifdef FREEBSD
      addr.sin_len = sizeof(struct sockaddr);
#endif      

      if(sctp_bindx(sockfd, (struct sockaddr*) &addr,
		    1, SCTP_BINDX_ADD_ADDR) < 0)
	goto fn_error;
    }
  }

 fn_exit:
  fclose(fptr);
  return rc;

 fn_error:
  perror("bind_from_file ERROR");
  rc = -1;
  goto fn_exit;

}


/*
 * sctp_open_dgm_socket2
 * Parameterized constructor of SCTP UDP style socket
 */
#undef FUNCNAME
#define FUNCNAME sctp_open_dgm_socket2
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int sctp_open_dgm_socket2(int num_stream, int block_mode,
			 int listen_back_log, int port, int nagle,
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
  memset(&initm, 0, sizeof(initm));
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

  char* env;
  env = getenv("MPICH_SCTP_BINDALL");
  if (env != NULL) {
    /* let SCTP handle the multihoming */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    env = getenv("MPICH_INTERFACE_HOSTNAME");
    if (env != NULL && *env != '\0') {
      addr.sin_addr.s_addr = inet_addr(env);
    } else {
      /* MPICH_INTERFACE_HOSTNAME should be set by mpd, so this shouldn't happen.. */
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
  }
  
  addr.sin_port = htons((unsigned short) port);
  rc = bind(*fd, (struct sockaddr *) &addr, sizeof(addr));
  
  if(rc == -1) {
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
    port = *real_port;
  } else 
    *real_port = port;

  env = getenv("MPICH_SCTP_MULTIHOME_FILE");
  if (env != NULL && *env != '\0') {
    rc = bind_from_file(env, *fd, port);
    if(rc == -1)
      goto fn_fail;
  } 
 
  /* set up listen back log */
  rc = listen(*fd, listen_back_log);
  if (rc == -1) {
    goto fn_fail;
  }

  /* register events */
  if (setsockopt(*fd, IPPROTO_SCTP, SCTP_EVENTS, evnts, sizeof(*evnts))) {
    perror("setsockopt error SCTP_EVENTS");
    goto fn_fail;
  }

  /* set nagle */
  if (setsockopt(*fd, IPPROTO_SCTP,
		 SCTP_NODELAY, (char *) &nagle, sizeof(nagle))){
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
  return SOCK_ERROR;

}

/* sctp_writev, implementation got from lksctp src */
#undef FUNCNAME
#define FUNCNAME sctp_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ssize_t sctp_writev(int s, struct iovec *data, int iovcnt,const
		    struct sockaddr *to,
		    socklen_t tolen __attribute__((unused)),
		    unsigned long ppid,
		    unsigned long flags,
		    unsigned short stream_no,
		    unsigned long timetolive,
		    unsigned long context,
                    int total ){

#ifdef MPICH_SCTP_CONCATENATES_IOVS
    /* required for Solaris since struct msghdr has no msg_control field */
    
    int current_cnt=0 , total_offset=0, byte_sent;
    char *send_buf = (char *) malloc(total);  /* FIXME assumes success */
    
    /* combine all iovcnt's into one message.  write all or nothing */
    while(current_cnt < iovcnt) {
        memcpy(send_buf+total_offset, data[current_cnt].iov_base, data[current_cnt].iov_len);
        total_offset += data[current_cnt].iov_len;
        current_cnt++;
    }

    byte_sent = sctp_sendmsg(s, send_buf, total, (struct sockaddr *) to, sizeof(*to), ppid,
                 flags, stream_no, timetolive, context);
    free(send_buf);
    
    return byte_sent;
}

#else
/* avoids copying.  uses iovec directly */
  struct msghdr outmsg;
  struct iovec iov;
  char outcmsg[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
  struct cmsghdr *cmsg;
  struct sctp_sndrcvinfo *sinfo;
  
  outmsg.msg_name = (caddr_t) to;
  outmsg.msg_namelen = tolen;
  outmsg.msg_iov = data;
  outmsg.msg_iovlen = iovcnt;
  
  outmsg.msg_control = outcmsg;
  outmsg.msg_controllen = sizeof(outcmsg);
  outmsg.msg_flags = 0;
  
  cmsg = CMSG_FIRSTHDR(&outmsg);
  cmsg->cmsg_level = IPPROTO_SCTP;
  cmsg->cmsg_type = SCTP_SNDRCV;
  cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
  
  outmsg.msg_controllen = cmsg->cmsg_len;
  sinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
  memset(sinfo, 0, sizeof(struct sctp_sndrcvinfo));
  sinfo->sinfo_ppid = ppid;
  sinfo->sinfo_flags = flags;
  sinfo->sinfo_stream = stream_no;
  sinfo->sinfo_timetolive = timetolive;
  sinfo->sinfo_context = context;
  
  /* pass in flag? */
  return sendmsg(s, &outmsg, 0);
   
}

#undef FUNCNAME
#define FUNCNAME sctp_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int sctp_readv(int s, struct iovec *data, int iovcnt, struct sockaddr *from,
		 socklen_t *fromlen, struct sctp_sndrcvinfo *sinfo,
		 int *msg_flags)
{
  int error;
  struct msghdr inmsg;
  char incmsg[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
  struct cmsghdr *cmsg = NULL;
  
  memset(&inmsg, 0, sizeof (inmsg));

  inmsg.msg_name = from;
  inmsg.msg_namelen = *fromlen;
  inmsg.msg_iov = data;
  inmsg.msg_iovlen = iovcnt;
  inmsg.msg_control = incmsg;
  inmsg.msg_controllen = sizeof(incmsg);
  
  error = recvmsg(s, &inmsg, 0);
  if (error < 0)
    return error;
  
  *fromlen = inmsg.msg_namelen;
  *msg_flags = inmsg.msg_flags;
  
  if (!sinfo)
    return error;
  
  for (cmsg = CMSG_FIRSTHDR(&inmsg); cmsg != NULL;
       cmsg = CMSG_NXTHDR(&inmsg, cmsg)){
    if ((IPPROTO_SCTP == cmsg->cmsg_level) &&
	(SCTP_SNDRCV == cmsg->cmsg_type))
      break;
  }
  
  /* Copy sinfo. */
  if (cmsg)
    memcpy(sinfo, CMSG_DATA(cmsg), sizeof(struct sctp_sndrcvinfo));
  
  return (error);
}
#endif /* MPICH_SCTP_CONCATENATES_IOVS */



#undef FUNCNAME
#define FUNCNAME giveMeSockAddr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int giveMeSockAddr(unsigned char ifaddr[], int port, struct sockaddr_in* addr) {

  memset(addr, 0, sizeof(addr));
  addr->sin_family = AF_INET;
  
  memcpy(&(addr->sin_addr.s_addr), ifaddr, sizeof(addr->sin_addr.s_addr));
  addr->sin_port = htons(port);
  
  return 0;
}

/*
 * sctp_recvmsg wrapper
 */
#undef FUNCNAME
#define FUNCNAME sctp_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int sctp_recv(int fd, char* buffer, int cnt, sctp_rcvinfo* sri, 
	      int* msg_flags, int* read) {

  struct sockaddr_storage from;
  socklen_t fromlen = sizeof(from);
  int flags, offset = 0;
  int total_read = 0;
  int temp_read = 0;

  sctp_rcvinfo prev_info;
  int loop = 0;

  while(TRUE) {
    flags = 0;
    temp_read = 0;
    errno = 0;
    fromlen = sizeof(from);

    prev_info = *sri;

    temp_read = sctp_recvmsg(fd,
	       buffer+offset, cnt, (struct sockaddr*) &from,
	       &fromlen, sri, &flags);

    if(offset == 0 && errno == EAGAIN) {
      total_read = 0;
      break;
    }

    if(temp_read > 0) {

      /* ASSERT time */
      if(loop) {
	MPIU_Assert(prev_info.sinfo_stream == sri->sinfo_stream);
	MPIU_Assert(prev_info.sinfo_assoc_id == sri->sinfo_assoc_id);
      }

      offset += temp_read;
      total_read += temp_read;
    }

    if(MSG_EOR & flags) {
      MPIU_Assert(flags & MSG_EOR);
      break;
    }

    loop++;
    break; /* looping assumes that partial delivery continues on the same stream
            *   and association until MSG_EOR is found.  This isn't the case for
            *   lksctp.  This break can be commented out as an optimization for
            *   other stacks.
            */
  }
  
  *msg_flags = flags;
  *read = total_read;

  return errno;
}

/* 
 * sctp_sendmsg wrapper
 * preliminary implementation: CHUNK is defined in sctp_common.h
 */
#undef FUNCNAME
#define FUNCNAME my_sctp_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int my_sctp_send(int fd, char* buffer, int cnt, struct sockaddr *to, uint16_t stream, int ppid) {

  int byte_sent = 0;
  int error;
  int offset = 0;
  int send_cnt = 0;
  int temp_cnt = cnt;

  while(temp_cnt) {

    send_cnt = (temp_cnt > CHUNK)? CHUNK : temp_cnt;

    errno = 0;
    error = sctp_sendmsg(fd, buffer+offset, send_cnt, to, sizeof(*to), ppid, 0, stream, 0, 0);

    byte_sent += (error > 0)? error : 0;

    if(error <= 0) {
        if(errno == EAGAIN)
            error = 0;
        break;
    }

    offset += byte_sent;
    temp_cnt -= byte_sent;
  }
  
  return byte_sent;
}


/*
 * sctp_fd_block
 * change block mode of a fd
 */
#undef FUNCNAME
#define FUNCNAME sctp_setblock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int sctp_setblock(int fd, int mode) {

  int result = 0;

  if(mode) {
    result = fcntl(fd, F_SETFL, 0); 
  } else {
    result = fcntl(fd, F_SETFL, O_NONBLOCK);
  }

  return result;
}
