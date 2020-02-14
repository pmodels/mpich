/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef OFI_AM_LMT_H_INCLUDED
#define OFI_AM_LMT_H_INCLUDED

/* called from ofi_init.c */
void MPIDI_OFI_am_lmt_init(void);

/* called from ofi_am_impl.h */
int MPIDI_OFI_am_lmt_send(int rank, MPIR_Comm * comm, int handler_id,
                          const void *am_hdr, size_t am_hdr_sz,
                          const void *data, MPI_Aint data_sz, MPIR_Request * sreq);

/* called from ofi_events.c */
int MPIDI_OFI_am_lmt_read_event(void *context);

#endif /* OFI_AM_LMT_H_INCLUDED */
