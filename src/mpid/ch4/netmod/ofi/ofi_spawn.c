/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"
#include "uthash.h"

/* NOTE: all these functions assume the caller to enter VCI-0 critical section */

/* NOTE: MPIDI_OFI_dynamic_{send,recv} is used for initial MPI_Comm_{connect/accept}, and
 *       MPIDI_OFI_dynamic_sendrecv is used for additional data exchange. The latter require
 *       exact source matching while MPIDI_OFI_dynamic_recv is an anysource receive.
 */

static int cancel_dynamic_request(MPIDI_OFI_dynamic_process_request_t * dynamic_req, bool is_send);
static uint64_t get_dynamic_connection_match_bits(int tag);
static uint64_t get_dynamic_match_bits(MPIR_Lpid lpid, int tag);

int MPIDI_OFI_dynamic_send(MPIR_Lpid remote_lpid, int tag, const void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIDI_OFI_ENABLE_TAGGED);
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif

    int vci = 0;                /* dynamic process only use vci 0 */
    int ctx_idx = 0;
    fi_addr_t remote_addr = MPIDI_OFI_av_to_phys_root(MPIDIU_lpid_to_av_slow(remote_lpid));

    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    uint64_t match_bits = get_dynamic_connection_match_bits(tag);

    MPL_time_t time_start, time_now;
    double time_gap;
    MPL_wtime(&time_start);
    MPIDI_OFI_CALL_RETRY(fi_tsend(MPIDI_OFI_global.ctx[ctx_idx].tx, buf, size, NULL /* desc */ ,
                                  remote_addr, match_bits, (void *) &req.context), vci, tsend);
    do {
        mpi_errno = MPIDI_OFI_progress_uninlined(vci);
        // mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_wtime(&time_now);
        MPL_wtime_diff(&time_start, &time_now, &time_gap);
    } while (!req.done && (timeout == 0 || time_gap < (double) timeout));

    if (!req.done) {
        /* time out, let's cancel the request */
        mpi_errno = cancel_dynamic_request(&req, true);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIX_ERR_TIMEOUT;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dynamic_recv(int tag, void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIDI_OFI_ENABLE_TAGGED);
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif

    int vci = 0;                /* dynamic process only use vci 0 */
    int ctx_idx = 0;
    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
    uint64_t match_bits = get_dynamic_connection_match_bits(tag);
    uint64_t mask_bits = 0;

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
        mpi_errno = cancel_dynamic_request(&req, false);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIX_ERR_TIMEOUT;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dynamic_sendrecv(MPIR_Lpid remote_lpid, int tag,
                               const void *send_buf, int send_size, void *recv_buf, int recv_size,
                               int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    /* NOTE: dynamic_sendrecv is always called inside CS of vci 0 */
    int vci = 0;
    int ctx_idx = 0;
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(vci));
#endif

    MPIDI_av_entry_t *av = MPIDIU_lpid_to_av_slow(remote_lpid);
    fi_addr_t remote_addr = MPIDI_OFI_av_to_phys_root(av);

    MPIDI_OFI_dynamic_process_request_t send_req;
    send_req.done = 0;
    send_req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    if (send_size > 0) {
        uint64_t match_bits = get_dynamic_match_bits(MPIR_Process.rank, tag);
        MPIDI_OFI_CALL_RETRY(fi_tsend(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                      send_buf, send_size, NULL,
                                      remote_addr, match_bits, (void *) &send_req.context),
                             vci, tsend);
    } else {
        send_req.done = 1;
    }

    MPIDI_OFI_dynamic_process_request_t recv_req;
    recv_req.done = 0;
    recv_req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    if (recv_size > 0) {
        uint64_t mask_bits = 0;
        uint64_t match_bits = get_dynamic_match_bits(remote_lpid, tag);
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                      recv_buf, recv_size, NULL,
                                      remote_addr, match_bits, mask_bits, &recv_req.context),
                             vci, trecv);
    } else {
        recv_req.done = 1;
    }

    MPL_time_t time_start;
    MPL_wtime(&time_start);
    while (!send_req.done || !recv_req.done) {
        mpi_errno = MPIDI_OFI_progress_uninlined(vci);
        MPIR_ERR_CHECK(mpi_errno);

        if (timeout > 0) {
            MPL_time_t time_now;
            double time_gap;
            MPL_wtime(&time_now);
            MPL_wtime_diff(&time_start, &time_now, &time_gap);
            if (time_gap > (double) timeout) {
                /* timed out, cancel the operations */
                if (!send_req.done) {
                    mpi_errno = cancel_dynamic_request(&send_req, true);
                    MPIR_ERR_CHECK(mpi_errno);
                }
                if (!recv_req.done) {
                    mpi_errno = cancel_dynamic_request(&recv_req, false);
                    MPIR_ERR_CHECK(mpi_errno);
                }

                mpi_errno = MPIX_ERR_TIMEOUT;
                break;
            }
        }
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
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, 0);

    MPIR_CHKPMEM_DECL();
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif

    MPIR_CHKPMEM_MALLOC((*local_upid_size), comm->local_size * sizeof(int), MPL_MEM_ADDRESS);
    int upid_size_estimate = MPIDI_OFI_global.addrnamelen + 20; /* 20 bytes as estimate for hostname */
    MPIR_CHKPMEM_MALLOC(temp_buf, comm->local_size * upid_size_estimate, MPL_MEM_BUFFER);

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
        if (idx + hostname_len + 1 + FI_NAME_MAX > buf_size) {
            buf_size += 1024;
            temp_buf = MPL_realloc(temp_buf, buf_size, MPL_MEM_OTHER);
            MPIR_Assert(temp_buf);
        }

        strcpy(temp_buf + idx, hostname);
        idx += hostname_len + 1;

        size_t sz = FI_NAME_MAX;
        MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, i);
        MPIDI_OFI_CALL(fi_av_lookup(MPIDI_OFI_global.ctx[ctx_idx].av,
                                    MPIDI_OFI_AV_ADDR_ROOT(av), temp_buf + idx, &sz), avlookup);
        idx += (int) sz;

        (*local_upid_size)[i] = hostname_len + 1 + sz;
    }

    *local_upids = temp_buf;

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_OFI_insert_upid(MPIR_Lpid lpid, const char *upid, int upid_len)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
    const char *hostname = upid;
    MPIDI_av_entry_t *av = MPIDIU_lpid_to_av_slow(lpid);

    bool do_insert = false;
    if (lpid & MPIR_LPID_DYNAMIC_MASK) {
        /* dynamic entry */
        do_insert = true;
    } else if (MPIDI_OFI_AV_IS_UNSET(av, lpid)) {
        /* new av entry */
        MPIDI_av_entry_t *dynamic_av = MPIDIU_find_dynamic_av(upid, upid_len);
        if (dynamic_av) {
            /* just copy it over */
            MPIDI_OFI_AV_ADDR_ROOT(av) = MPIDI_OFI_AV_ADDR_ROOT(dynamic_av);
        } else {
            do_insert = true;
        }

        /* set node_id */
        int node_id;
        mpi_errno = MPIR_nodeid_lookup(hostname, &node_id);
        MPIR_ERR_CHECK(mpi_errno);
        av->node_id = node_id;
    } else {
        /* A known entry, nothing to do */
        goto fn_exit;
    }

    if (do_insert) {
        const char *addrname = hostname + strlen(hostname) + 1;
        /* new entry */
        MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[0].av, addrname,
                                    1, &MPIDI_OFI_AV_ADDR_ROOT(av), 0ULL, NULL), avmap);
        MPIR_Assert(MPIDI_OFI_AV_ADDR_ROOT(av) != FI_ADDR_NOTAVAIL);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- internal static routines */

/* NOTE: used by MPIDI_OFI_dynamic_sendrecv, exact source match */
static uint64_t get_dynamic_match_bits(MPIR_Lpid lpid, int tag)
{
    /* normalize tag within (MPIDI_OFI_TAG_BITS - 1) bits, reserve 1 bit for dynamic connect/accept */
    tag &= (1 << (MPIDI_OFI_TAG_BITS - 1)) - 1;

    uint64_t match_bits = MPIDI_OFI_DYNPROC_SEND | tag;

    if (!MPIDI_OFI_ENABLE_DATA) {
        /* FI_DIRECTED_RECV is not enabled, we have to embed source in the match_bits */
        MPIDI_av_entry_t *av = MPIDIU_lpid_to_av_slow(lpid);

        char upid[FI_NAME_MAX];
        size_t sz = FI_NAME_MAX;
        fi_av_lookup(MPIDI_OFI_global.ctx[0].av, MPIDI_OFI_AV_ADDR_ROOT(av), upid, &sz);

        unsigned upid_hash;
        HASH_VALUE(upid, sz, upid_hash);
        upid_hash &= (1 << MPIDI_OFI_SOURCE_BITS) - 1;

        match_bits |= (upid_hash << MPIDI_OFI_TAG_BITS);
    }

    return match_bits;
}

/* NOTE: used by MPIDI_OFI_dynamic_{send/recv}, separate tag space from MPIDI_OFI_dynamic_sendrecv,
 *       do not set source bits since it is an any source match.  */
static uint64_t get_dynamic_connection_match_bits(int tag)
{
    /* normalize tag within (MPIDI_OFI_TAG_BITS - 1) bits */
    tag &= (1 << (MPIDI_OFI_TAG_BITS - 1)) - 1;
    /* set the high bit for dynamic connect/accept */
    tag |= (1 << (MPIDI_OFI_TAG_BITS - 1));

    return MPIDI_OFI_DYNPROC_SEND | tag;
}

static int cancel_dynamic_request(MPIDI_OFI_dynamic_process_request_t * dynamic_req, bool is_send)
{
    int mpi_errno = MPI_SUCCESS;

    struct fid_ep *ep;
    if (is_send) {
        ep = MPIDI_OFI_global.ctx[0].tx;
    } else {
        ep = MPIDI_OFI_global.ctx[0].rx;
    }
    int rc;
    rc = fi_cancel((fid_t) ep, (void *) &dynamic_req->context);
    if (rc && rc != -FI_ENOENT) {
        MPIR_ERR_CHKANDJUMP2(rc < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cancel",
                             "**ofid_cancel %s %s", MPIDI_OFI_DEFAULT_NIC_NAME, fi_strerror(-rc));

    }
    while (!dynamic_req->done) {
        mpi_errno = MPIDI_OFI_progress_uninlined(0);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
