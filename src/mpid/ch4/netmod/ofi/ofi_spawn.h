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
#ifndef OFI_SPAWN_H_INCLUDED
#define OFI_SPAWN_H_INCLUDED

#include "ofi_impl.h"
#include "pmi.h"

#define MPIDI_OFI_PORT_NAME_TAG_KEY "tag"
#define MPIDI_OFI_CONNENTRY_TAG_KEY "connentry"

#define MPIDI_OFI_DYNPROC_RECEIVER 0
#define MPIDI_OFI_DYNPROC_SENDER 1

static inline void MPIDI_OFI_free_port_name_tag(int tag)
{
    int index, rem_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_FREE_PORT_NAME_TAG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_FREE_PORT_NAME_TAG);

    index = tag / (sizeof(int) * 8);
    rem_tag = tag - (index * sizeof(int) * 8);

    MPIDI_Global.port_name_tag_mask[index] &= ~(1 << ((8 * sizeof(int)) - 1 - rem_tag));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_FREE_PORT_NAME_TAG);
}


static inline int MPIDI_OFI_get_port_name_tag(int *port_name_tag)
{
    unsigned i, j;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_GET_PORT_NAME_TAG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_GET_PORT_NAME_TAG);

    for (i = 0; i < MPIR_MAX_CONTEXT_MASK; i++)
        if (MPIDI_Global.port_name_tag_mask[i] != ~0)
            break;

    if (i < MPIR_MAX_CONTEXT_MASK)
        for (j = 0; j < (8 * sizeof(int)); j++) {
            if ((MPIDI_Global.port_name_tag_mask[i] | (1 << ((8 * sizeof(int)) - j - 1))) !=
                MPIDI_Global.port_name_tag_mask[i]) {
                MPIDI_Global.port_name_tag_mask[i] |= (1 << ((8 * sizeof(int)) - j - 1));
                *port_name_tag = ((i * 8 * sizeof(int)) + j);
                goto fn_exit;
            }
        }
    else
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_GET_PORT_NAME_TAG);
    return mpi_errno;

  fn_fail:
    *port_name_tag = -1;
    mpi_errno = MPI_ERR_OTHER;
    goto fn_exit;
}

static inline int MPIDI_OFI_get_tag_from_port(const char *port_name, int *port_name_tag)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPL_STR_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_GET_TAG_FROM_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_GET_TAG_FROM_PORT);

    if (strlen(port_name) == 0)
        goto fn_exit;

    str_errno = MPL_str_get_int_arg(port_name, MPIDI_OFI_PORT_NAME_TAG_KEY, port_name_tag);
    MPIR_ERR_CHKANDJUMP(str_errno, mpi_errno, MPI_ERR_OTHER, "**argstr_no_port_name_tag");
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_GET_TAG_FROM_PORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int MPIDI_OFI_get_conn_name_from_port(const char *port_name, char *connname)
{
    int mpi_errno = MPI_SUCCESS;
    int maxlen = MPIDI_KVSAPPSTRLEN;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_GET_CONN_NAME_FROM_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_GET_CONN_NAME_FROM_PORT);

    MPL_str_get_binary_arg(port_name,
                           MPIDI_OFI_CONNENTRY_TAG_KEY,
                           connname, MPIDI_Global.addrnamelen, &maxlen);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_GET_CONN_NAME_FROM_PORT);
    return mpi_errno;
}

static inline int MPIDI_OFI_dynproc_create_intercomm(const char *port_name,
                                                     int remote_size,
                                                     int *remote_lupids,
                                                     MPIR_Comm * comm_ptr,
                                                     MPIR_Comm ** newcomm,
                                                     int is_low_group, char *api)
{
    int context_id_offset, mpi_errno = MPI_SUCCESS;
    MPIR_Comm *tmp_comm_ptr = NULL;
    int i = 0;
    MPIDI_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DYNPROC_CREATE_INTERCOMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DYNPROC_CREATE_INTERCOMM);

    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_get_tag_from_port(port_name, &context_id_offset));
    MPIDI_OFI_MPI_CALL_POP(MPIR_Comm_create(&tmp_comm_ptr));

    tmp_comm_ptr->context_id = MPIR_CONTEXT_SET_FIELD(DYNAMIC_PROC, context_id_offset, 1);
    tmp_comm_ptr->recvcontext_id = tmp_comm_ptr->context_id;
    tmp_comm_ptr->remote_size = remote_size;
    tmp_comm_ptr->local_size = comm_ptr->local_size;
    tmp_comm_ptr->rank = comm_ptr->rank;
    tmp_comm_ptr->comm_kind = MPIR_COMM_KIND__INTERCOMM;
    tmp_comm_ptr->local_comm = comm_ptr;
    tmp_comm_ptr->is_low_group = is_low_group;

    /* handle local group */
    /* No ref changes to LUT/MLUT in this step because the localcomm will not
     * be released in the normal way */
    MPIDI_COMM(tmp_comm_ptr, local_map).mode = MPIDI_COMM(comm_ptr, map).mode;
    MPIDI_COMM(tmp_comm_ptr, local_map).size = MPIDI_COMM(comm_ptr, map).size;
    MPIDI_COMM(tmp_comm_ptr, local_map).avtid = MPIDI_COMM(comm_ptr, map).avtid;
    switch (MPIDI_COMM(comm_ptr, map).mode) {
    case MPIDI_RANK_MAP_DIRECT:
    case MPIDI_RANK_MAP_DIRECT_INTRA:
        break;
    case MPIDI_RANK_MAP_OFFSET:
    case MPIDI_RANK_MAP_OFFSET_INTRA:
        MPIDI_COMM(tmp_comm_ptr, local_map).reg.offset = MPIDI_COMM(comm_ptr, map).reg.offset;
        break;
    case MPIDI_RANK_MAP_STRIDE:
    case MPIDI_RANK_MAP_STRIDE_INTRA:
    case MPIDI_RANK_MAP_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
        MPIDI_COMM(tmp_comm_ptr, local_map).reg.stride.stride =
            MPIDI_COMM(comm_ptr, map).reg.stride.stride;
        MPIDI_COMM(tmp_comm_ptr, local_map).reg.stride.blocksize =
            MPIDI_COMM(comm_ptr, map).reg.stride.blocksize;
        MPIDI_COMM(tmp_comm_ptr, local_map).reg.stride.offset =
            MPIDI_COMM(comm_ptr, map).reg.stride.offset;
        break;
    case MPIDI_RANK_MAP_LUT:
    case MPIDI_RANK_MAP_LUT_INTRA:
        MPIDI_COMM(tmp_comm_ptr, local_map).irreg.lut.t = MPIDI_COMM(comm_ptr, map).irreg.lut.t;
        MPIDI_COMM(tmp_comm_ptr, local_map).irreg.lut.lpid =
            MPIDI_COMM(comm_ptr, map).irreg.lut.lpid;
        break;
    case MPIDI_RANK_MAP_MLUT:
        MPIDI_COMM(tmp_comm_ptr, local_map).irreg.mlut.t = MPIDI_COMM(comm_ptr, map).irreg.mlut.t;
        MPIDI_COMM(tmp_comm_ptr, local_map).irreg.mlut.gpid =
            MPIDI_COMM(comm_ptr, map).irreg.mlut.gpid;
        break;
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }

    /* set mapping for remote group */
    mpi_errno = MPIDIU_alloc_mlut(&mlut, remote_size);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }
    MPIDI_COMM(tmp_comm_ptr, map).mode = MPIDI_RANK_MAP_MLUT;
    MPIDI_COMM(tmp_comm_ptr, map).size = remote_size;
    MPIDI_COMM(tmp_comm_ptr, map).avtid = -1;
    MPIDI_COMM(tmp_comm_ptr, map).irreg.mlut.t = mlut;
    MPIDI_COMM(tmp_comm_ptr, map).irreg.mlut.gpid = mlut->gpid;
    for (i = 0; i < remote_size; ++i) {
        MPIDI_COMM(tmp_comm_ptr, map).irreg.mlut.gpid[i].avtid =
            MPIDIU_LUPID_GET_AVTID(remote_lupids[i]);
        MPIDI_COMM(tmp_comm_ptr, map).irreg.mlut.gpid[i].lpid =
            MPIDIU_LUPID_GET_LPID(remote_lupids[i]);
    }

    MPIR_Comm_commit(tmp_comm_ptr);

    MPIDI_OFI_MPI_CALL_POP(MPIR_Comm_dup_impl(tmp_comm_ptr, newcomm));

    tmp_comm_ptr->local_comm = NULL;    /* avoid freeing local comm with comm_release */
    MPIR_Comm_release(tmp_comm_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DYNPROC_CREATE_INTERCOMM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_OFI_dynproc_handshake(int root,
                                              int phase,
                                              int timeout,
                                              int port_id,
                                              fi_addr_t * conn,
                                              MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_dynamic_process_request_t req;
    uint64_t match_bits = 0;
    uint64_t mask_bits = 0;
    struct fi_msg_tagged msg;
    int buf = 0;
    MPID_Time_t time_sta, time_now;
    double time_gap;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DYNPROC_HANDSHAKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DYNPROC_HANDSHAKE);

    /* connector */
    if (phase == 0) {
        req.done = MPIDI_OFI_PEEK_START;
        req.event_id = MPIDI_OFI_EVENT_ACCEPT_PROBE;
        match_bits = MPIDI_OFI_init_recvtag(&mask_bits, port_id,
                                            MPI_ANY_SOURCE, MPI_ANY_TAG);
        match_bits |= MPIDI_OFI_DYNPROC_SEND;

        msg.msg_iov = NULL;
        msg.desc = NULL;
        msg.iov_count = 0;
        msg.addr = FI_ADDR_UNSPEC;
        msg.tag = match_bits;
        msg.ignore = mask_bits;
        msg.context = (void *) &req.context;
        msg.data = 0;

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL,VERBOSE,
                        (MPL_DBG_FDEST, "connecting port_id %d, conn %lu, waiting for MPIDI_OFI_dynproc_handshake",
                         port_id, *conn));
        time_gap = 0.0;
        MPID_Wtime(&time_sta);
        while (req.done != MPIDI_OFI_PEEK_FOUND) {
            req.done = MPIDI_OFI_PEEK_START;
            MPIDI_OFI_CALL(fi_trecvmsg
                           (MPIDI_OFI_EP_RX_TAG(0), &msg,
                            FI_PEEK | FI_COMPLETION | (MPIDI_OFI_ENABLE_DATA ? FI_REMOTE_CQ_DATA : 0)), trecv);
            do {
                mpi_errno = MPID_Progress_test();
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);

                MPID_Wtime(&time_now);
                MPID_Wtime_diff(&time_sta, &time_now, &time_gap);
            } while(req.done == MPIDI_OFI_PEEK_START && (int) time_gap < timeout);
            if ((int) time_gap >= timeout) {
                /* connection is timed out */
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL,VERBOSE,
                                (MPL_DBG_FDEST, "connection to port_id %d, conn %lu ack timed out",
                                 port_id, *conn));
                mpi_errno = MPI_ERR_PORT;
                goto fn_fail;
            }
        }

        req.done = 0;
        req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_EP_RX_TAG(0),
                                      &buf,
                                      sizeof(int),
                                      NULL,
                                      *conn,
                                      match_bits,
                                      mask_bits, &req.context), trecv, MPIDI_OFI_CALL_LOCK);
        time_gap = 0.0;
        MPID_Wtime(&time_sta);
        do {
            mpi_errno = MPID_Progress_test();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);

            MPID_Wtime(&time_now);
            MPID_Wtime_diff(&time_sta, &time_now, &time_gap);
        } while(!req.done && (int) time_gap < timeout);
        if ((int) time_gap >= timeout) {
            /* connection is mismatched */
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL,VERBOSE,
                            (MPL_DBG_FDEST, "connection to port_id %d, conn %lu ack mismatched",
                             port_id, *conn));
            mpi_errno = MPI_ERR_PORT;
            goto fn_fail;
        }
    }

    /* acceptor */
    if (phase == 1) {
        int tag = root;

        match_bits = MPIDI_OFI_init_sendtag(port_id,
                                            comm_ptr->rank,
                                            tag, MPIDI_OFI_DYNPROC_SEND);

        req.done = 0;
        req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        mpi_errno = MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(0),
                                           &buf,
                                           sizeof(int),
                                           NULL,
                                           comm_ptr->rank,
                                           *conn,
                                           match_bits,
                                           (void *) &req.context,
                                           MPIDI_OFI_DO_SEND,
                                           MPIDI_OFI_CALL_LOCK);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        MPIDI_OFI_PROGRESS_WHILE(!req.done);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DYNPROC_HANDSHAKE);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_OFI_dynproc_exchange_map(int root,
                                                 int phase,
                                                 int port_id,
                                                 fi_addr_t * conn,
                                                 char *conname,
                                                 MPIR_Comm * comm_ptr,
                                                 int *out_root,
                                                 int *remote_size,
                                                 size_t **remote_upid_size,
                                                 char **remote_upids,
                                                 MPID_Node_id_t **remote_node_ids)
{
    int i, mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_dynamic_process_request_t req[3];
    uint64_t match_bits = 0;
    uint64_t mask_bits = 0;
    struct fi_msg_tagged msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DYNPROC_EXCHANGE_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DYNPROC_EXCHANGE_MAP);

    MPIR_CHKPMEM_DECL(3);

    req[0].done = MPIDI_OFI_PEEK_START;
    req[0].event_id = MPIDI_OFI_EVENT_ACCEPT_PROBE;
    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, port_id,
                                        MPI_ANY_SOURCE, MPI_ANY_TAG);
    match_bits |= MPIDI_OFI_DYNPROC_SEND;

    if (phase == 0) {
        size_t remote_upid_recvsize = 0;

        /* Receive the addresses                           */
        /* We don't know the size, so probe for table size */
        /* Receive phase updates the connection            */
        /* With the probed address                         */
        msg.msg_iov = NULL;
        msg.desc = NULL;
        msg.iov_count = 0;
        msg.addr = FI_ADDR_UNSPEC;
        msg.tag = match_bits;
        msg.ignore = mask_bits;
        msg.context = (void *) &req[0].context;
        msg.data = 0;

        while (req[0].done != MPIDI_OFI_PEEK_FOUND) {
            req[0].done = MPIDI_OFI_PEEK_START;
            MPIDI_OFI_CALL(fi_trecvmsg
                           (MPIDI_OFI_EP_RX_TAG(0), &msg,
                            FI_PEEK | FI_COMPLETION | (MPIDI_OFI_ENABLE_DATA ? FI_REMOTE_CQ_DATA : 0)), trecv);
            MPIDI_OFI_PROGRESS_WHILE(req[0].done == MPIDI_OFI_PEEK_START);
        }

        *remote_size = req[0].msglen / sizeof(size_t);
        *out_root = req[0].tag;
        MPIR_CHKPMEM_MALLOC((*remote_upid_size), size_t*,
                            (*remote_size) * sizeof(size_t), mpi_errno, "remote_upid_size");
        MPIR_CHKPMEM_MALLOC((*remote_node_ids), MPID_Node_id_t*,
                            (*remote_size) * sizeof(MPID_Node_id_t), mpi_errno, "remote_node_ids");

        req[0].done = 0;
        req[0].event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        req[1].done = 0;
        req[1].event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        req[2].done = 0;
        req[2].event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_EP_RX_TAG(0),
                                      *remote_upid_size,
                                      (*remote_size) * sizeof(size_t),
                                      NULL,
                                      FI_ADDR_UNSPEC,
                                      match_bits,
                                      mask_bits, &req[0].context), trecv, MPIDI_OFI_CALL_LOCK);
        MPIDI_OFI_PROGRESS_WHILE(!req[0].done);

        for (i = 0; i < (*remote_size); i++) remote_upid_recvsize += (*remote_upid_size)[i];
        MPIR_CHKPMEM_MALLOC((*remote_upids), char*, remote_upid_recvsize,
                            mpi_errno, "remote_upids");

        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_EP_RX_TAG(0),
                                      *remote_upids,
                                      remote_upid_recvsize,
                                      NULL,
                                      FI_ADDR_UNSPEC,
                                      match_bits,
                                      mask_bits, &req[1].context), trecv, MPIDI_OFI_CALL_LOCK);

        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_EP_RX_TAG(0),
                                      *remote_node_ids,
                                      (*remote_size) * sizeof(MPID_Node_id_t),
                                      NULL,
                                      FI_ADDR_UNSPEC,
                                      match_bits,
                                      mask_bits, &req[2].context), trecv, MPIDI_OFI_CALL_LOCK);

        MPIDI_OFI_PROGRESS_WHILE(!req[1].done || !req[2].done);
        size_t disp = 0;
        for (i = 0; i < req[0].source; i++) disp += (*remote_upid_size)[i];
        memcpy(conname, *remote_upids + disp, (*remote_upid_size)[req[0].source]);
        MPIR_CHKPMEM_COMMIT();
    }

    if (phase == 1) {
        /* Send our table to the child */
        /* Send phase maps the entry   */
        int tag = root;
        int local_size = comm_ptr->local_size;
        size_t *local_upid_size = NULL;
        size_t local_upid_sendsize = 0;
        char *local_upids = NULL;
        MPID_Node_id_t *local_node_ids = NULL;

        /* Step 1: get local upids (with size) and node ids for sending */
        MPIDI_NM_get_local_upids(comm_ptr, &local_upid_size, &local_upids);
        for (i = 0; i < local_size; i++) local_upid_sendsize += local_upid_size[i];
        local_node_ids = (MPID_Node_id_t*) MPL_malloc(local_size * sizeof(MPID_Node_id_t));
        for (i = 0; i < comm_ptr->local_size; i++)
            MPIDI_CH4U_get_node_id(comm_ptr, i, &local_node_ids[i]);


        match_bits = MPIDI_OFI_init_sendtag(port_id,
                                            comm_ptr->rank,
                                            tag, MPIDI_OFI_DYNPROC_SEND);

        /* fi_av_map here is not quite right for some providers */
        /* we need to get this connection from the sockname     */
        req[0].done = 0;
        req[0].event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        req[1].done = 0;
        req[1].event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        req[2].done = 0;
        req[2].event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        mpi_errno = MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(0),
                                           local_upid_size,
                                           local_size * sizeof(size_t),
                                           NULL,
                                           comm_ptr->rank,
                                           *conn,
                                           match_bits,
                                           (void *) &req[0].context,
                                           MPIDI_OFI_DO_SEND,
                                           MPIDI_OFI_CALL_LOCK);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(0),
                               local_upids,
                               local_upid_sendsize,
                               NULL,
                               comm_ptr->rank,
                               *conn,
                               match_bits,
                               (void *) &req[1].context,
                               MPIDI_OFI_DO_SEND,
                               MPIDI_OFI_CALL_LOCK);
        MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(0),
                               local_node_ids,
                               local_size * sizeof(MPID_Node_id_t),
                               NULL,
                               comm_ptr->rank,
                               *conn,
                               match_bits,
                               (void *) &req[2].context,
                               MPIDI_OFI_DO_SEND,
                               MPIDI_OFI_CALL_LOCK);

        MPIDI_OFI_PROGRESS_WHILE(!req[0].done || !req[1].done || !req[2].done);

        MPL_free(local_upid_size);
        MPL_free(local_upids);
        MPL_free(local_node_ids);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DYNPROC_EXCHANGE_MAP);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_comm_connect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_comm_connect(const char *port_name,
                                            MPIR_Info * info,
                                            int root,
                                            int timeout,
                                            MPIR_Comm * comm_ptr,
                                            MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS, root_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int remote_size = 0;
    size_t *remote_upid_size;
    char *remote_upids = NULL;
    int *remote_lupids = NULL;
    MPID_Node_id_t *remote_node_ids = NULL;
    int is_low_group = -1;
    int parent_root = -1;
    int rank = comm_ptr->rank;
    int port_id;
    int conn_id;
    fi_addr_t conn;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_CONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_CONNECT);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        MPIR_Assert(0);
        goto fn_exit;
    }
    MPIR_CHKLMEM_DECL(1);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_SPAWN_MUTEX);
    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_get_tag_from_port(port_name, &port_id));

    if (rank == root) {
        char conname[FI_NAME_MAX];
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_get_conn_name_from_port(port_name, conname));
        MPIDI_OFI_CALL(fi_av_insert(MPIDI_Global.av, conname, 1, &conn, 0ULL, NULL), avmap);
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_dynproc_exchange_map
                               (root, MPIDI_OFI_DYNPROC_SENDER,
                                port_id, &conn, conname, comm_ptr, &parent_root,
                                &remote_size, &remote_upid_size, &remote_upids,
                                &remote_node_ids));
        mpi_errno = MPIDI_OFI_dynproc_handshake(root, MPIDI_OFI_DYNPROC_RECEIVER,
                                                timeout, port_id, &conn, comm_ptr);
        if (mpi_errno == MPI_ERR_PORT || mpi_errno == MPI_SUCCESS) {
            root_errno = mpi_errno;
            mpi_errno = MPIR_Bcast_intra(&root_errno, 1, MPI_INT, root, comm_ptr, &errflag);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            if (root_errno != MPI_SUCCESS) {
                mpi_errno = root_errno;
                MPIR_ERR_POP(mpi_errno);
            }
        }
        else {
            MPIR_ERR_POP(mpi_errno);
        }
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_dynproc_exchange_map
                               (root, MPIDI_OFI_DYNPROC_RECEIVER,
                                port_id, &conn, conname, comm_ptr, &parent_root,
                                &remote_size, &remote_upid_size, &remote_upids,
                                &remote_node_ids));
        MPIR_CHKLMEM_MALLOC(remote_lupids, int*, remote_size * sizeof(int),
                            mpi_errno, "remote_lupids");

        MPIDIU_upids_to_lupids(remote_size, remote_upid_size, remote_upids, &remote_lupids,
                               remote_node_ids);
        /* the child comm group is alawys the low group */
        is_low_group = 0;
    }

    if (rank != root) {
        mpi_errno = MPIR_Bcast_intra(&root_errno, 1, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
        if (root_errno != MPI_SUCCESS) {
            mpi_errno = root_errno;
            MPIR_ERR_POP(mpi_errno);
        }
    }

    /* broadcast the upids to local groups */
    MPIDI_OFI_MPI_CALL_POP(MPIDIU_Intercomm_map_bcast_intra(comm_ptr, root,
                                                            &remote_size, &is_low_group, 0,
                                                            remote_upid_size,
                                                            remote_upids,
                                                            &remote_lupids,
                                                            remote_node_ids));
    if (rank == root) {
        MPL_free(remote_upid_size);
        MPL_free(remote_upids);
        MPL_free(remote_node_ids);
    }

    /* Now Create the New Intercomm */
    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_dynproc_create_intercomm(port_name,
                                                              remote_size,
                                                              remote_lupids,
                                                              comm_ptr,
                                                              newcomm, is_low_group,
                                                              (char *) "Connect"));
    if (rank == root) {
        conn_id = MPIDI_OFI_conn_manager_insert_conn(conn, (*newcomm)->rank,
                                                     MPIDI_OFI_DYNPROC_CONNECTED_CHILD);
        MPIDI_OFI_COMM(*newcomm).conn_id = conn_id;
    }
    MPIDI_OFI_MPI_CALL_POP(MPIR_Barrier_intra(comm_ptr, &errflag));
  fn_exit:
    if (rank == root) {
        MPIR_CHKLMEM_FREEALL();
    }
    else {
        MPL_free(remote_lupids);
    }
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_SPAWN_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_CONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_comm_disconnect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_DISCONNECT);

    MPIDI_OFI_MPI_CALL_POP(MPIR_Comm_free_impl(comm_ptr));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_comm_open_port
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPL_STR_SUCCESS;
    int port_name_tag = 0;
    int len = MPI_MAX_PORT_NAME;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_OPEN_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_OPEN_PORT);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        MPIR_Assert(0);
        goto fn_exit;
    }

    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_get_port_name_tag(&port_name_tag));
    MPIDI_OFI_STR_CALL(MPL_str_add_int_arg(&port_name, &len, MPIDI_OFI_PORT_NAME_TAG_KEY,
                                           port_name_tag), port_str);
    MPIDI_OFI_STR_CALL(MPL_str_add_binary_arg(&port_name, &len, MPIDI_OFI_CONNENTRY_TAG_KEY,
                                              MPIDI_Global.addrname,
                                              MPIDI_Global.addrnamelen), port_str);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_OPEN_PORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_comm_close_port
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_close_port(const char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    int port_name_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_CLOSE_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_CLOSE_PORT);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        MPIR_Assert(0);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_get_tag_from_port(port_name, &port_name_tag);
    MPIDI_OFI_free_port_name_tag(port_name_tag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_CLOSE_PORT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_comm_close_port
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_comm_accept(const char *port_name,
                                           MPIR_Info * info,
                                           int root, MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int remote_size = 0;
    size_t *remote_upid_size;
    char *remote_upids = NULL;
    int *remote_lupids = NULL;
    MPID_Node_id_t *remote_node_ids = NULL;
    int child_root = -1;
    int is_low_group = -1;
    int conn_id;
    fi_addr_t conn;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_ACCEPT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_ACCEPT);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        MPIR_Assert(0);
        goto fn_exit;
    }
    MPIR_CHKLMEM_DECL(1);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_SPAWN_MUTEX);

    int rank = comm_ptr->rank;

    if (rank == root) {
        char conname[FI_NAME_MAX];
        int port_id;

        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_get_tag_from_port(port_name, &port_id));
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_dynproc_exchange_map
                               (root, MPIDI_OFI_DYNPROC_RECEIVER,
                                port_id, &conn, conname, comm_ptr, &child_root,
                                &remote_size, &remote_upid_size, &remote_upids,
                                &remote_node_ids));
        MPIDI_OFI_CALL(fi_av_insert(MPIDI_Global.av, conname, 1, &conn, 0ULL, NULL), avmap);
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_dynproc_handshake
                               (root, MPIDI_OFI_DYNPROC_SENDER, 0,
                                port_id, &conn, comm_ptr));
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_dynproc_exchange_map
                               (root, MPIDI_OFI_DYNPROC_SENDER,
                                port_id, &conn, conname, comm_ptr, &child_root,
                                &remote_size, &remote_upid_size, &remote_upids,
                                &remote_node_ids));
        MPIDI_OFI_CALL(fi_av_remove(MPIDI_Global.av, &conn, 1, 0ULL), avmap);
        MPIR_CHKLMEM_MALLOC(remote_lupids, int*, remote_size * sizeof(int),
                            mpi_errno, "remote_lupids");
        MPIDIU_upids_to_lupids(remote_size, remote_upid_size, remote_upids, &remote_lupids,
                               remote_node_ids);
        /* the parent comm group is alawys the low group */
        is_low_group = 1;
    }

    /* broadcast the upids to local groups */
    MPIDI_OFI_MPI_CALL_POP(MPIDIU_Intercomm_map_bcast_intra(comm_ptr, root,
                                                            &remote_size, &is_low_group, 0,
                                                            remote_upid_size,
                                                            remote_upids,
                                                            &remote_lupids,
                                                            remote_node_ids));
    if (rank == root) {
        MPL_free(remote_upid_size);
        MPL_free(remote_upids);
        MPL_free(remote_node_ids);
    }

    /* Now Create the New Intercomm */
    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_dynproc_create_intercomm(port_name,
                                                              remote_size,
                                                              remote_lupids,
                                                              comm_ptr,
                                                              newcomm, is_low_group,
                                                              (char *) "Connect"));
    if (rank == root) {
        conn_id = MPIDI_OFI_conn_manager_insert_conn(conn, (*newcomm)->rank,
                                                     MPIDI_OFI_DYNPROC_CONNECTED_PARENT);
        MPIDI_OFI_COMM(*newcomm).conn_id = conn_id;
    }
    MPIDI_OFI_MPI_CALL_POP(MPIR_Barrier_intra(comm_ptr, &errflag));
  fn_exit:
    if (rank == root) {
        MPIR_CHKLMEM_FREEALL();
    }
    else {
        MPL_free(remote_lupids);
    }
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_SPAWN_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_ACCEPT);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#endif /* OFI_SPAWN_H_INCLUDED */
