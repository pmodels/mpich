/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SOCKETIMPL_H
#define SOCKETIMPL_H

#include "mm_socket.h"
#include "sock.h"

#define SOCKET_ACCEPT_CONNECTION 0
#define SOCKET_REJECT_CONNECTION 1

#define SOCKET_EAGER_LIMIT       (1024 * 120)
#define SOCKET_ERROR_MSG_LENGTH  256
#define SOCKET_LISTENER_POINTER  &SOCKET_Process

typedef struct SOCKET_PerProcess {
    MPID_Thread_lock_t lock;
            sock_set_t set;
          sock_event_t out;
                sock_t listener;
		   int port;
		  char host[100];
		   int nSOCKET_EAGER_LIMIT;
		   int error;
		  char err_msg[SOCKET_ERROR_MSG_LENGTH];
} SOCKET_PerProcess;

extern SOCKET_PerProcess SOCKET_Process;

int socket_handle_read(MPIDI_VC *vc_ptr, int num_bytes);
int socket_handle_read_data(MPIDI_VC *vc_ptr, int num_read);
int socket_handle_read_ack(MPIDI_VC *vc_ptr, int num_read);
int socket_handle_read_context_pkt(MPIDI_VC *vc_ptr, int num_read);
int socket_handle_written(MPIDI_VC *vc_ptr, int num_bytes);
int socket_handle_written_ack(MPIDI_VC *vc_ptr, int num_written);
int socket_handle_written_context_pkt(MPIDI_VC *vc_ptr, int num_written);
int socket_read_data(MPIDI_VC *vc_ptr);
int socket_write_aggressive(MPIDI_VC *vc_ptr);

#endif
