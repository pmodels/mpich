/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifndef SHM_INLINE
#define SHM_DISABLE_INLINES
#include <mpidimpl.h>
#include "shm_inline.h"
MPIDI_SHM_funcs_t MPIDI_SHM_src_funcs = {
    .mpi_init = MPIDI_SHM_mpi_init_hook,
    .mpi_finalize = MPIDI_SHM_mpi_finalize_hook,
    .get_vni_attr = MPIDI_SHM_get_vni_attr,
    .progress = MPIDI_SHM_progress,
    .mpi_comm_connect = MPIDI_SHM_mpi_comm_connect,
    .mpi_comm_disconnect = MPIDI_SHM_mpi_comm_disconnect,
    .mpi_open_port = MPIDI_SHM_mpi_open_port,
    .mpi_close_port = MPIDI_SHM_mpi_close_port,
    .mpi_comm_accept = MPIDI_SHM_mpi_comm_accept,
    /* Routines that handle addressing */
    .comm_get_lpid = MPIDI_SHM_comm_get_lpid,
    .get_local_upids = MPIDI_SHM_get_local_upids,
    .upids_to_lupids = MPIDI_SHM_upids_to_lupids,
    .create_intercomm_from_lpids = MPIDI_SHM_create_intercomm_from_lpids,
    .mpi_comm_create_hook = MPIDI_SHM_mpi_comm_create_hook,
    .mpi_comm_free_hook = MPIDI_SHM_mpi_comm_free_hook,
    /* Window initialization/cleanup routines */
    .mpi_win_create_hook = MPIDI_SHM_mpi_win_create_hook,
    .mpi_win_allocate_hook = MPIDI_SHM_mpi_win_allocate_hook,
    .mpi_win_allocate_shared_hook = MPIDI_SHM_mpi_win_allocate_shared_hook,
    .mpi_win_create_dynamic_hook = MPIDI_SHM_mpi_win_create_dynamic_hook,
    .mpi_win_attach_hook = MPIDI_SHM_mpi_win_attach_hook,
    .mpi_win_detach_hook = MPIDI_SHM_mpi_win_detach_hook,
    .mpi_win_free_hook = MPIDI_SHM_mpi_win_free_hook,
    /* RMA synchronization routines */
    .rma_win_cmpl_hook = MPIDI_SHM_rma_win_cmpl_hook,
    .rma_win_local_cmpl_hook = MPIDI_SHM_rma_win_local_cmpl_hook,
    .rma_target_cmpl_hook = MPIDI_SHM_rma_target_cmpl_hook,
    .rma_target_local_cmpl_hook = MPIDI_SHM_rma_target_local_cmpl_hook,
    .rma_op_cs_enter_hook = MPIDI_SHM_rma_op_cs_enter_hook,
    .rma_op_cs_exit_hook = MPIDI_SHM_rma_op_cs_exit_hook,
    /* Request initialization/cleanup routines */
    .am_request_init = MPIDI_SHM_am_request_init,
    .am_request_finalize = MPIDI_SHM_am_request_finalize,
    /* Active Message Routines */
    .am_send_hdr = MPIDI_SHM_am_send_hdr,
    .am_isend = MPIDI_SHM_am_isend,
    .am_isendv = MPIDI_SHM_am_isendv,
    .am_send_hdr_reply = MPIDI_SHM_am_send_hdr_reply,
    .am_isend_reply = MPIDI_SHM_am_isend_reply,
    .am_hdr_max_sz = MPIDI_SHM_am_hdr_max_sz,
    .am_recv = MPIDI_SHM_am_recv
};

MPIDI_SHM_native_funcs_t MPIDI_SHM_native_src_funcs = {
    .mpi_send = MPIDI_SHM_mpi_send,
    .mpi_ssend = MPIDI_SHM_mpi_ssend,
    .mpi_startall = MPIDI_SHM_mpi_startall,
    .mpi_send_init = MPIDI_SHM_mpi_send_init,
    .mpi_ssend_init = MPIDI_SHM_mpi_ssend_init,
    .mpi_rsend_init = MPIDI_SHM_mpi_rsend_init,
    .mpi_bsend_init = MPIDI_SHM_mpi_bsend_init,
    .mpi_isend = MPIDI_SHM_mpi_isend,
    .mpi_issend = MPIDI_SHM_mpi_issend,
    .mpi_cancel_send = MPIDI_SHM_mpi_cancel_send,
    .mpi_recv_init = MPIDI_SHM_mpi_recv_init,
    .mpi_recv = MPIDI_SHM_mpi_recv,
    .mpi_irecv = MPIDI_SHM_mpi_irecv,
    .mpi_imrecv = MPIDI_SHM_mpi_imrecv,
    .mpi_cancel_recv = MPIDI_SHM_mpi_cancel_recv,
    .mpi_alloc_mem = MPIDI_SHM_mpi_alloc_mem,
    .mpi_free_mem = MPIDI_SHM_mpi_free_mem,
    .mpi_improbe = MPIDI_SHM_mpi_improbe,
    .mpi_iprobe = MPIDI_SHM_mpi_iprobe,
    .mpi_win_set_info = MPIDI_SHM_mpi_win_set_info,
    .mpi_win_shared_query = MPIDI_SHM_mpi_win_shared_query,
    .mpi_put = MPIDI_SHM_mpi_put,
    .mpi_win_start = MPIDI_SHM_mpi_win_start,
    .mpi_win_complete = MPIDI_SHM_mpi_win_complete,
    .mpi_win_post = MPIDI_SHM_mpi_win_post,
    .mpi_win_wait = MPIDI_SHM_mpi_win_wait,
    .mpi_win_test = MPIDI_SHM_mpi_win_test,
    .mpi_win_lock = MPIDI_SHM_mpi_win_lock,
    .mpi_win_unlock = MPIDI_SHM_mpi_win_unlock,
    .mpi_win_get_info = MPIDI_SHM_mpi_win_get_info,
    .mpi_get = MPIDI_SHM_mpi_get,
    .mpi_win_free = MPIDI_SHM_mpi_win_free,
    .mpi_win_fence = MPIDI_SHM_mpi_win_fence,
    .mpi_win_create = MPIDI_SHM_mpi_win_create,
    .mpi_accumulate = MPIDI_SHM_mpi_accumulate,
    .mpi_win_attach = MPIDI_SHM_mpi_win_attach,
    .mpi_win_allocate_shared = MPIDI_SHM_mpi_win_allocate_shared,
    .mpi_rput = MPIDI_SHM_mpi_rput,
    .mpi_win_flush_local = MPIDI_SHM_mpi_win_flush_local,
    .mpi_win_detach = MPIDI_SHM_mpi_win_detach,
    .mpi_compare_and_swap = MPIDI_SHM_mpi_compare_and_swap,
    .mpi_raccumulate = MPIDI_SHM_mpi_raccumulate,
    .mpi_rget_accumulate = MPIDI_SHM_mpi_rget_accumulate,
    .mpi_fetch_and_op = MPIDI_SHM_mpi_fetch_and_op,
    .mpi_win_allocate = MPIDI_SHM_mpi_win_allocate,
    .mpi_win_flush = MPIDI_SHM_mpi_win_flush,
    .mpi_win_flush_local_all = MPIDI_SHM_mpi_win_flush_local_all,
    .mpi_win_unlock_all = MPIDI_SHM_mpi_win_unlock_all,
    .mpi_win_create_dynamic = MPIDI_SHM_mpi_win_create_dynamic,
    .mpi_rget = MPIDI_SHM_mpi_rget,
    .mpi_win_sync = MPIDI_SHM_mpi_win_sync,
    .mpi_win_flush_all = MPIDI_SHM_mpi_win_flush_all,
    .mpi_get_accumulate = MPIDI_SHM_mpi_get_accumulate,
    .mpi_win_lock_all = MPIDI_SHM_mpi_win_lock_all,
    /* Collectives */
    .mpi_barrier = MPIDI_SHM_mpi_barrier,
    .mpi_bcast = MPIDI_SHM_mpi_bcast,
    .mpi_allreduce = MPIDI_SHM_mpi_allreduce,
    .mpi_allgather = MPIDI_SHM_mpi_allgather,
    .mpi_allgatherv = MPIDI_SHM_mpi_allgatherv,
    .mpi_scatter = MPIDI_SHM_mpi_scatter,
    .mpi_scatterv = MPIDI_SHM_mpi_scatterv,
    .mpi_gather = MPIDI_SHM_mpi_gather,
    .mpi_gatherv = MPIDI_SHM_mpi_gatherv,
    .mpi_alltoall = MPIDI_SHM_mpi_alltoall,
    .mpi_alltoallv = MPIDI_SHM_mpi_alltoallv,
    .mpi_alltoallw = MPIDI_SHM_mpi_alltoallw,
    .mpi_reduce = MPIDI_SHM_mpi_reduce,
    .mpi_reduce_scatter = MPIDI_SHM_mpi_reduce_scatter,
    .mpi_reduce_scatter_block = MPIDI_SHM_mpi_reduce_scatter_block,
    .mpi_scan = MPIDI_SHM_mpi_scan,
    .mpi_exscan = MPIDI_SHM_mpi_exscan,
    .mpi_neighbor_allgather = MPIDI_SHM_mpi_neighbor_allgather,
    .mpi_neighbor_allgatherv = MPIDI_SHM_mpi_neighbor_allgatherv,
    .mpi_neighbor_alltoall = MPIDI_SHM_mpi_neighbor_alltoall,
    .mpi_neighbor_alltoallv = MPIDI_SHM_mpi_neighbor_alltoallv,
    .mpi_neighbor_alltoallw = MPIDI_SHM_mpi_neighbor_alltoallw,
    .mpi_ineighbor_allgather = MPIDI_SHM_mpi_ineighbor_allgather,
    .mpi_ineighbor_allgatherv = MPIDI_SHM_mpi_ineighbor_allgatherv,
    .mpi_ineighbor_alltoall = MPIDI_SHM_mpi_ineighbor_alltoall,
    .mpi_ineighbor_alltoallv = MPIDI_SHM_mpi_ineighbor_alltoallv,
    .mpi_ineighbor_alltoallw = MPIDI_SHM_mpi_ineighbor_alltoallw,
    .mpi_ibarrier = MPIDI_SHM_mpi_ibarrier,
    .mpi_ibcast = MPIDI_SHM_mpi_ibcast,
    .mpi_iallgather = MPIDI_SHM_mpi_iallgather,
    .mpi_iallgatherv = MPIDI_SHM_mpi_iallgatherv,
    .mpi_iallreduce = MPIDI_SHM_mpi_iallreduce,
    .mpi_ialltoall = MPIDI_SHM_mpi_ialltoall,
    .mpi_ialltoallv = MPIDI_SHM_mpi_ialltoallv,
    .mpi_ialltoallw = MPIDI_SHM_mpi_ialltoallw,
    .mpi_iexscan = MPIDI_SHM_mpi_iexscan,
    .mpi_igather = MPIDI_SHM_mpi_igather,
    .mpi_igatherv = MPIDI_SHM_mpi_igatherv,
    .mpi_ireduce_scatter_block = MPIDI_SHM_mpi_ireduce_scatter_block,
    .mpi_ireduce_scatter = MPIDI_SHM_mpi_ireduce_scatter,
    .mpi_ireduce = MPIDI_SHM_mpi_ireduce,
    .mpi_iscan = MPIDI_SHM_mpi_iscan,
    .mpi_iscatter = MPIDI_SHM_mpi_iscatter,
    .mpi_iscatterv = MPIDI_SHM_mpi_iscatterv,
    /* Datatype hooks */
    .mpi_type_commit_hook = MPIDI_SHM_mpi_type_commit_hook,
    .mpi_type_free_hook = MPIDI_SHM_mpi_type_free_hook,
    /* Op hooks */
    .mpi_op_commit_hook = MPIDI_SHM_mpi_op_commit_hook,
    .mpi_op_free_hook = MPIDI_SHM_mpi_op_free_hook,
};
#endif
