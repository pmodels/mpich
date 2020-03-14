/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef XPMEM_CONTROL_H_INCLUDED
#define XPMEM_CONTROL_H_INCLUDED

#include "shm_types.h"

int MPIDI_XPMEM_rts_cb(int handler_id, void *am_hdr, void **data, size_t * data_sz, int is_local,
                       int *is_contig, MPIR_Request ** req);
int MPIDI_XPMEM_cts_cb(int handler_id, void *am_hdr, void **data, size_t * data_sz, int is_local,
                       int *is_contig, MPIR_Request ** req);
int MPIDI_XPMEM_send_fin_cb(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                            int is_local, int *is_contig, MPIR_Request ** req);
int MPIDI_XPMEM_recv_fin_cb(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                            int is_local, int *is_contig, MPIR_Request ** req);
int MPIDI_XPMEM_cnt_free_cb(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                            int is_local, int *is_contig, MPIR_Request ** req);

#endif /* XPMEM_CONTROL_H_INCLUDED */
