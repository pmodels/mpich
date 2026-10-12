/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_IMPL_H_INCLUDED
#define SOCK_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "sock_types.h"
#include "mpidch4r.h"
#include "ch4_impl.h"

#define MPIDI_SOCK_AV(av)     ((av)->netmod.sock)

int MPIDI_SOCK_comm_addr_exchange(MPIR_Comm * comm);

/* sock_conn.c */
int MPIDI_SOCK_conn_init(void);
int MPIDI_SOCK_conn_finalize(void);
int MPIDI_SOCK_conn_connect(MPIDI_av_entry_t * av, MPIDI_SOCK_conn_t ** conn_p);
int MPIDI_SOCK_conn_accept(void);
void MPIDI_SOCK_conn_close(MPIDI_SOCK_conn_t * conn);
int MPIDI_SOCK_set_nonblock(int fd);

/* sock_io.c */
int MPIDI_SOCK_send(MPIDI_av_entry_t * av, int handler_id, const void *am_hdr, MPI_Aint am_hdr_sz,
                    const void *data, MPI_Aint data_sz, void *pack_buf,
                    int src_vci, int dst_vci, MPIR_Request * sreq);
int MPIDI_SOCK_flush(void);

/* sock_spawn.c */
int MPIDI_SOCK_dynamic_enqueue(MPIDI_SOCK_am_header_t * hdr, char *buf);
void MPIDI_SOCK_dynamic_finalize(void);
int MPIDI_SOCK_progress(int *made_progress);

#endif /* SOCK_IMPL_H_INCLUDED */
