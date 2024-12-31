/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

int MPIDI_OFI_dynamic_send(uint64_t remote_gpid, int tag, const void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIDI_OFI_ENABLE_TAGGED);

    int nic = 0;                /* dynamic process only use nic 0 */
    int vci = 0;                /* dynamic process only use vci 0 */
    int ctx_idx = 0;
    int avtid = MPIDIU_GPID_GET_AVTID(remote_gpid);
    int lpid = MPIDIU_GPID_GET_LPID(remote_gpid);
    fi_addr_t remote_addr = MPIDI_OFI_av_to_phys(&MPIDIU_get_av(avtid, lpid), nic, vci);

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

/* the following functions are "proc" functions, but because they are only used during dynamic
 * process spawning, having them here provides better context */

int MPIDI_OFI_upids_to_gpids(int size, int *remote_upid_size, char *remote_upids,
                             uint64_t * remote_gpids)
{
    int i, mpi_errno = MPI_SUCCESS;
    int *new_avt_procs;
    char **new_upids;
    int n_new_procs = 0;
    int n_avts;
    char *curr_upid;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);

    MPIR_CHKLMEM_DECL();

    MPIR_CHKLMEM_MALLOC(new_avt_procs, sizeof(int) * size);
    MPIR_CHKLMEM_MALLOC(new_upids, sizeof(char *) * size);

    n_avts = MPIDIU_get_n_avts();

    curr_upid = remote_upids;
    for (i = 0; i < size; i++) {
        int j, k;
        char tbladdr[FI_NAME_MAX];
        int found = 0;
        size_t sz = 0;

        char *hostname = curr_upid;
        int hostname_len = strlen(hostname);
        char *addrname = hostname + hostname_len + 1;
        int addrname_len = remote_upid_size[i] - hostname_len - 1;

        for (k = 0; k < n_avts; k++) {
            if (MPIDIU_get_av_table(k) == NULL) {
                continue;
            }
            for (j = 0; j < MPIDIU_get_av_table(k)->size; j++) {
                sz = MPIDI_OFI_global.addrnamelen;
                MPIDI_OFI_VCI_CALL(fi_av_lookup(MPIDI_OFI_global.ctx[ctx_idx].av,
                                                MPIDI_OFI_TO_PHYS(k, j, nic), &tbladdr, &sz), 0,
                                   avlookup);
                if (sz == addrname_len && !memcmp(tbladdr, addrname, addrname_len)) {
                    remote_gpids[i] = MPIDIU_GPID_CREATE(k, j);
                    found = 1;
                    break;
                }
            }
            if (found) {
                break;
            }
        }

        if (!found) {
            new_avt_procs[n_new_procs] = i;
            new_upids[n_new_procs] = curr_upid;
            n_new_procs++;
        }
        curr_upid += remote_upid_size[i];
    }

    /* create new av_table, insert processes */
    if (n_new_procs > 0) {
        int avtid;
        mpi_errno = MPIDIU_new_avt(n_new_procs, &avtid);
        MPIR_ERR_CHECK(mpi_errno);

        for (i = 0; i < n_new_procs; i++) {
            char *hostname = new_upids[i];
            char *addrname = hostname + strlen(hostname) + 1;

            fi_addr_t addr;
            MPIDI_OFI_VCI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[ctx_idx].av, addrname,
                                            1, &addr, 0ULL, NULL), 0, avmap);
            MPIR_Assert(addr != FI_ADDR_NOTAVAIL);
            MPIDI_OFI_AV(&MPIDIU_get_av(avtid, i)).dest[nic][0] = addr;

            int node_id;
            mpi_errno = MPIR_nodeid_lookup(hostname, &node_id);
            MPIR_ERR_CHECK(mpi_errno);
            MPIDIU_get_av(avtid, i).node_id = node_id;

            remote_gpids[new_avt_procs[i]] = MPIDIU_GPID_CREATE(avtid, i);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
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

    MPIR_CHKPMEM_DECL();

    MPIR_CHKPMEM_MALLOC((*local_upid_size), comm->local_size * sizeof(int), MPL_MEM_ADDRESS);
    MPIR_CHKPMEM_MALLOC(temp_buf, comm->local_size * MPIDI_OFI_global.addrnamelen, MPL_MEM_BUFFER);

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

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}
