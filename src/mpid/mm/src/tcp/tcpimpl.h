/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef TCPIMPL_H
#define TCPIMPL_H

#include "mm_tcp.h"
#include "bsocket.h"

#define TCP_ACCEPT_CONNECTION 0
#define TCP_REJECT_CONNECTION 1

#define TCP_EAGER_LIMIT (1024 * 20)

#define TCP_ERROR_MSG_LENGTH 256

typedef struct TCP_PerProcess {
    MPID_Thread_lock_t lock;
                   int listener;
		   int port;
		  char host[100];
               bfd_set readset;
               bfd_set writeset;
	    MPIDI_VC * read_list;
	    MPIDI_VC * write_list;
	           int max_bfd;
		   int error;
		  char err_msg[TCP_ERROR_MSG_LENGTH];
		   int num_readers;
		   int num_writers;
		   int nTCP_EAGER_LIMIT;
} TCP_PerProcess;

extern TCP_PerProcess TCP_Process;

int tcp_read(MPIDI_VC *vc_ptr);
int tcp_read_header(MPIDI_VC *vc_ptr);
int tcp_read_data(MPIDI_VC *vc_ptr);
int tcp_write(MPIDI_VC *vc_ptr);
int tcp_write_aggressive(MPIDI_VC *vc_ptr);

#endif
