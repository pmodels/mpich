/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2019 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "shm_control.h"
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include "../xpmem/xpmem_control.h"
#endif

int MPIDI_SHM_ctrl_dispatch(int ctrl_id, void *ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ctrl_id) {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
        case MPIDI_SHM_XPMEM_SEND_LMT_RTS:
            mpi_errno = MPIDI_XPMEM_ctrl_send_lmt_rts_cb((MPIDI_SHM_ctrl_hdr_t *) ctrl_hdr);
            break;
        case MPIDI_SHM_XPMEM_SEND_LMT_CTS:
            mpi_errno = MPIDI_XPMEM_ctrl_send_lmt_cts_cb((MPIDI_SHM_ctrl_hdr_t *) ctrl_hdr);
            break;
        case MPIDI_SHM_XPMEM_SEND_LMT_SEND_FIN:
            mpi_errno = MPIDI_XPMEM_ctrl_send_lmt_send_fin_cb((MPIDI_SHM_ctrl_hdr_t *) ctrl_hdr);
            break;
        case MPIDI_SHM_XPMEM_SEND_LMT_RECV_FIN:
            mpi_errno = MPIDI_XPMEM_ctrl_send_lmt_recv_fin_cb((MPIDI_SHM_ctrl_hdr_t *) ctrl_hdr);
            break;
        case MPIDI_SHM_XPMEM_SEND_LMT_CNT_FREE:
            mpi_errno = MPIDI_XPMEM_ctrl_send_lmt_cnt_free_cb((MPIDI_SHM_ctrl_hdr_t *) ctrl_hdr);
            break;
#endif
        default:
            /* Unknown SHM control header */
            MPIR_Assert(0);
    }

    return mpi_errno;
}
