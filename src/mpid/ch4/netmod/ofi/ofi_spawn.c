/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

int MPIDI_OFI_dynamic_send(MPIR_Lpid remote_lpid, int tag, const void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIDI_OFI_ENABLE_TAGGED);

    int nic = 0;                /* dynamic process only use nic 0 */
    int vci = 0;                /* dynamic process only use vci 0 */
    int ctx_idx = 0;
    fi_addr_t remote_addr = MPIDI_OFI_av_to_phys(MPIDIU_lpid_to_av_slow(remote_lpid), nic, vci);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    uint64_t match_bits = MPIDI_OFI_init_sendtag(0, 0, tag);
    match_bits |= MPIDI_OFI_DYNPROC_SEND;

    MPL_time_t time_start, time_now;
    double time_gap;
    MPL_wtime(&time_start);
    if (MPIDI_OFI_ENABLE_DATA) {
        MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          buf, size, NULL /* desc */ , 0,
                                          remote_addr, match_bits, (void *) &req.context),
                             vci, tsenddata);
    } else {
        MPIDI_OFI_CALL_RETRY(fi_tsend(MPIDI_OFI_global.ctx[ctx_idx].tx, buf, size, NULL /* desc */ ,
                                      remote_addr, match_bits, (void *) &req.context), vci, tsend);
    }
    do {
        mpi_errno = MPIDI_OFI_progress_uninlined(vci);
        // mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_wtime(&time_now);
        MPL_wtime_diff(&time_start, &time_now, &time_gap);
    } while (!req.done && (timeout == 0 || time_gap < (double) timeout));

    if (!req.done) {
        /* time out, let's cancel the request */
        int rc;
        rc = fi_cancel((fid_t) MPIDI_OFI_global.ctx[ctx_idx].tx, (void *) &req.context);
        if (rc && rc != -FI_ENOENT) {
            MPIR_ERR_CHKANDJUMP2(rc < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cancel",
                                 "**ofid_cancel %s %s", MPIDI_OFI_DEFAULT_NIC_NAME,
                                 fi_strerror(-rc));

        }
        while (!req.done) {
            mpi_errno = MPIDI_OFI_progress_uninlined(vci);
            MPIR_ERR_CHECK(mpi_errno);
        }

        mpi_errno = MPIX_ERR_TIMEOUT;
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dynamic_recv(int tag, void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIDI_OFI_ENABLE_TAGGED);

    int vci = 0;                /* dynamic process only use vci 0 */
    int ctx_idx = 0;
    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
    uint64_t match_bits = 0;
    uint64_t mask_bits = 0;
    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, 0, MPI_ANY_SOURCE, tag);
    match_bits |= MPIDI_OFI_DYNPROC_SEND;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPL_time_t time_start, time_now;
    double time_gap;
    MPL_wtime(&time_start);
    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                  buf, size, NULL,
                                  FI_ADDR_UNSPEC, match_bits, mask_bits, &req.context), vci, trecv);
    do {
        mpi_errno = MPIDI_OFI_progress_uninlined(vci);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_wtime(&time_now);
        MPL_wtime_diff(&time_start, &time_now, &time_gap);
    } while (!req.done && (timeout == 0 || time_gap < (double) timeout));

    if (!req.done) {
        /* time out, let's cancel the request */
        int rc;
        rc = fi_cancel((fid_t) MPIDI_OFI_global.ctx[ctx_idx].rx, (void *) &req.context);
        if (rc && rc != -FI_ENOENT) {
            MPIR_ERR_CHKANDJUMP2(rc < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cancel",
                                 "**ofid_cancel %s %s", MPIDI_OFI_DEFAULT_NIC_NAME,
                                 fi_strerror(-rc));

        }
        while (!req.done) {
            mpi_errno = MPIDI_OFI_progress_uninlined(vci);
            MPIR_ERR_CHECK(mpi_errno);
        }

        mpi_errno = MPIX_ERR_TIMEOUT;
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dynamic_sendrecv(MPIR_Lpid remote_lpid, int tag,
                               const void *send_buf, int send_size, void *recv_buf, int recv_size)
{
    int mpi_errno = MPI_SUCCESS;

    /* NOTE: dynamic_sendrecv is always called inside CS of vci 0 */
    int vci = 0;
    int nic = 0;
    int ctx_idx = 0;
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[vci].lock));
#endif

    MPIDI_av_entry_t *av = MPIDIU_lpid_to_av_slow(remote_lpid);
    fi_addr_t remote_addr = MPIDI_OFI_av_to_phys(av, nic, vci);

    MPIDI_OFI_dynamic_process_request_t send_req;
    send_req.done = 0;
    send_req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    if (send_size > 0) {
        uint64_t match_bits = MPIDI_OFI_DYNPROC_SEND | tag;
        if (MPIDI_OFI_ENABLE_DATA) {
            MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                              send_buf, send_size, NULL, 0,
                                              remote_addr, match_bits, (void *) &send_req.context),
                                 vci, tsenddata);
        } else {
            MPIDI_OFI_CALL_RETRY(fi_tsend(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          send_buf, send_size, NULL,
                                          remote_addr, match_bits, (void *) &send_req.context),
                                 vci, tsend);
        }
    } else {
        send_req.done = 1;
    }

    MPIDI_OFI_dynamic_process_request_t recv_req;
    recv_req.done = 0;
    recv_req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    if (recv_size > 0) {
        uint64_t mask_bits = 0;
        uint64_t match_bits = MPIDI_OFI_DYNPROC_SEND | tag;
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                      recv_buf, recv_size, NULL,
                                      remote_addr, match_bits, mask_bits, &recv_req.context),
                             vci, trecv);
    } else {
        recv_req.done = 1;
    }

    while (!send_req.done || !recv_req.done) {
        mpi_errno = MPIDI_OFI_progress_uninlined(vci);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_get_local_upids(MPIR_Comm * comm, int **local_upid_size, char **local_upids)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    char *temp_buf = NULL;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);

    MPIR_CHKPMEM_DECL(2);

    MPIR_CHKPMEM_MALLOC((*local_upid_size), int *, comm->local_size * sizeof(int),
                        mpi_errno, "local_upid_size", MPL_MEM_ADDRESS);
    MPIR_CHKPMEM_MALLOC(temp_buf, char *, comm->local_size * MPIDI_OFI_global.addrnamelen,
                        mpi_errno, "temp_buf", MPL_MEM_BUFFER);

    int buf_size = 0;
    int idx = 0;
    for (i = 0; i < comm->local_size; i++) {
        /* upid format: hostname + '\0' + addrname */
        int hostname_len = 0;
        char *hostname = (char *) "";
        int node_id = MPIDIU_comm_rank_to_av(comm, i)->node_id;
        if (MPIR_Process.node_hostnames && node_id >= 0) {
            hostname = utarray_eltptr(MPIR_Process.node_hostnames, node_id);
            hostname_len = strlen(hostname);
        }
        int upid_len = hostname_len + 1 + MPIDI_OFI_global.addrnamelen;
        if (idx + upid_len > buf_size) {
            buf_size += 1024;
            temp_buf = MPL_realloc(temp_buf, buf_size, MPL_MEM_OTHER);
            MPIR_Assert(temp_buf);
        }

        strcpy(temp_buf + idx, hostname);
        idx += hostname_len + 1;

        size_t sz = MPIDI_OFI_global.addrnamelen;;
        MPIDI_OFI_addr_t *av = &MPIDI_OFI_AV(MPIDIU_comm_rank_to_av(comm, i));
        MPIDI_OFI_VCI_CALL(fi_av_lookup(MPIDI_OFI_global.ctx[ctx_idx].av, av->dest[nic][0],
                                        temp_buf + idx, &sz), 0, avlookup);
        idx += (int) sz;

        (*local_upid_size)[i] = upid_len;
    }

    *local_upids = temp_buf;

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_OFI_insert_upid(MPIR_Lpid lpid, const char *upid, int upid_len)
{
    int mpi_errno = MPI_SUCCESS;

    const char *hostname = upid;
    MPIDI_av_entry_t *av = MPIDIU_lpid_to_av_slow(lpid);

    bool is_dynamic = (lpid & MPIR_LPID_DYNAMIC_MASK);
    bool do_insert = false;
    if (is_dynamic) {
        do_insert = true;
    } else if (MPIDI_OFI_AV(av).dest[0][0] == FI_ADDR_NOTAVAIL) {
        MPIDI_av_entry_t *dynamic_av = MPIDIU_find_dynamic_av(upid, upid_len);
        if (dynamic_av) {
            /* just copy it over */
            MPIDI_OFI_AV(av).dest[0][0] = MPIDI_OFI_AV(dynamic_av).dest[0][0];
        } else {
            do_insert = true;
        }

        /* set node_id */
        int node_id;
        mpi_errno = MPIR_nodeid_lookup(hostname, &node_id);
        MPIR_ERR_CHECK(mpi_errno);
        av->node_id = node_id;
    }

    if (do_insert) {
        const char *addrname = hostname + strlen(hostname) + 1;
        /* new entry */
        MPIDI_OFI_VCI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[0].av, addrname,
                                        1, &MPIDI_OFI_AV(av).dest[0][0], 0ULL, NULL), 0, avmap);
        MPIR_Assert(MPIDI_OFI_AV(av).dest[0][0] != FI_ADDR_NOTAVAIL);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
