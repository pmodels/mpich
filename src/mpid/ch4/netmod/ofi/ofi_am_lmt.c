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

#include "mpidimpl.h"
#include "ofi_am_lmt.h"

static int ofi_handle_lmt_req(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req);
static int ofi_handle_lmt_ack(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req);

void MPIDI_OFI_am_lmt_init(void)
{
    MPIDIG_am_reg_cb(MPIDI_OFI_AM_LMT_REQ, NULL, &ofi_handle_lmt_req);
    MPIDIG_am_reg_cb(MPIDI_OFI_AM_LMT_ACK, NULL, &ofi_handle_lmt_ack);
}

/* MPIDI_OFI_AM_LMT_REQ */

int MPIDI_OFI_am_lmt_send(int rank, MPIR_Comm * comm, int handler_id,
                          const void *am_hdr, size_t am_hdr_sz,
                          const void *data, MPI_Aint data_sz, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ofi_handle_lmt_req(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_lmt_read_event(void *context)
{
    int mpi_errno = MPI_SUCCESS;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDI_OFI_AM_LMT_ACK */

static int ofi_do_lmt_ack(int rank, int context_id, MPIR_Request * sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static int ofi_handle_lmt_ack(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
