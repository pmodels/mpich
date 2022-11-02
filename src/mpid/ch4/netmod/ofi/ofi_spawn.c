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
    int vni = 0;                /* dynamic process only use vni 0 */
    int ctx_idx = 0;
    int avtid = MPIDIU_GPID_GET_AVTID(remote_gpid);
    int lpid = MPIDIU_GPID_GET_LPID(remote_gpid);
    fi_addr_t remote_addr = MPIDI_OFI_av_to_phys(&MPIDIU_get_av(avtid, lpid), nic, vni, vni);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);

    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
    uint64_t match_bits;
    match_bits = MPIDI_OFI_init_sendtag(0, tag, MPIDI_OFI_DYNPROC_SEND);

    MPL_time_t time_start, time_now;
    double time_gap;
    MPL_wtime(&time_start);
    MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                      buf, size, NULL /* desc */ , 0,
                                      remote_addr, match_bits, (void *) &req.context),
                         vni, tsenddata, FALSE /* eagain */);
    do {
        mpi_errno = MPIDI_OFI_progress_uninlined(vni);
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
            MPIR_ERR_CHKANDJUMP4(rc < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cancel",
                                 "**ofid_cancel %s %d %s %s", __SHORT_FILE__, __LINE__, __func__,
                                 fi_strerror(-rc));

        }
        while (!req.done) {
            mpi_errno = MPIDI_OFI_progress_uninlined(vni);
            MPIR_ERR_CHECK(mpi_errno);
        }

        mpi_errno = MPIX_ERR_TIMEOUT;
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dynamic_recv(int tag, void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIDI_OFI_ENABLE_TAGGED);

    int vni = 0;                /* dynamic process only use vni 0 */
    int ctx_idx = 0;
    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
    uint64_t match_bits = 0;
    uint64_t mask_bits = 0;
    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, 0, tag);
    match_bits |= MPIDI_OFI_DYNPROC_SEND;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);

    MPL_time_t time_start, time_now;
    double time_gap;
    MPL_wtime(&time_start);
    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                  buf, size, NULL,
                                  FI_ADDR_UNSPEC, match_bits, mask_bits, &req.context),
                         vni, trecv, FALSE);
    do {
        mpi_errno = MPIDI_OFI_progress_uninlined(vni);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_wtime(&time_now);
        MPL_wtime_diff(&time_start, &time_now, &time_gap);
    } while (!req.done && (timeout == 0 || time_gap < (double) timeout));

    if (!req.done) {
        /* time out, let's cancel the request */
        int rc;
        rc = fi_cancel((fid_t) MPIDI_OFI_global.ctx[ctx_idx].rx, (void *) &req.context);
        if (rc && rc != -FI_ENOENT) {
            MPIR_ERR_CHKANDJUMP4(rc < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cancel",
                                 "**ofid_cancel %s %d %s %s", __SHORT_FILE__, __LINE__, __func__,
                                 fi_strerror(-rc));

        }
        while (!req.done) {
            mpi_errno = MPIDI_OFI_progress_uninlined(vni);
            MPIR_ERR_CHECK(mpi_errno);
        }

        mpi_errno = MPIX_ERR_TIMEOUT;
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
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
    int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, 0, nic);

    MPIR_CHKLMEM_DECL(2);

    MPIR_CHKLMEM_MALLOC(new_avt_procs, int *, sizeof(int) * size, mpi_errno, "new_avt_procs",
                        MPL_MEM_ADDRESS);
    MPIR_CHKLMEM_MALLOC(new_upids, char **, sizeof(char *) * size, mpi_errno, "new_upids",
                        MPL_MEM_ADDRESS);

    n_avts = MPIDIU_get_n_avts();

    curr_upid = remote_upids;
    for (i = 0; i < size; i++) {
        int j, k;
        char tbladdr[FI_NAME_MAX];
        int found = 0;
        size_t sz = 0;

        for (k = 0; k < n_avts; k++) {
            if (MPIDIU_get_av_table(k) == NULL) {
                continue;
            }
            for (j = 0; j < MPIDIU_get_av_table(k)->size; j++) {
                sz = MPIDI_OFI_global.addrnamelen;
                MPIDI_OFI_VCI_CALL(fi_av_lookup(MPIDI_OFI_global.ctx[ctx_idx].av,
                                                MPIDI_OFI_TO_PHYS(k, j, nic), &tbladdr, &sz), 0,
                                   avlookup);
                if (sz == remote_upid_size[i]
                    && !memcmp(tbladdr, curr_upid, remote_upid_size[i])) {
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
            fi_addr_t addr;
            MPIDI_OFI_VCI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[ctx_idx].av, new_upids[i],
                                            1, &addr, 0ULL, NULL), 0, avmap);
            MPIR_Assert(addr != FI_ADDR_NOTAVAIL);
            MPIDI_OFI_AV(&MPIDIU_get_av(avtid, i)).dest[nic][0] = addr;
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
    int i, total_size = 0;
    char *temp_buf = NULL, *curr_ptr = NULL;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(comm, 0, nic);

    MPIR_CHKPMEM_DECL(2);
    MPIR_CHKLMEM_DECL(1);

    MPIR_CHKPMEM_MALLOC((*local_upid_size), int *, comm->local_size * sizeof(int),
                        mpi_errno, "local_upid_size", MPL_MEM_ADDRESS);
    MPIR_CHKLMEM_MALLOC(temp_buf, char *, comm->local_size * MPIDI_OFI_global.addrnamelen,
                        mpi_errno, "temp_buf", MPL_MEM_BUFFER);

    for (i = 0; i < comm->local_size; i++) {
        size_t sz = MPIDI_OFI_global.addrnamelen;;
        MPIDI_OFI_addr_t *av = &MPIDI_OFI_AV(MPIDIU_comm_rank_to_av(comm, i));
        MPIDI_OFI_VCI_CALL(fi_av_lookup(MPIDI_OFI_global.ctx[ctx_idx].av, av->dest[nic][0],
                                        &temp_buf[i * MPIDI_OFI_global.addrnamelen],
                                        &sz), 0, avlookup);
        (*local_upid_size)[i] = (int) sz;
        total_size += (*local_upid_size)[i];
    }

    MPIR_CHKPMEM_MALLOC((*local_upids), char *, total_size * sizeof(char),
                        mpi_errno, "local_upids", MPL_MEM_BUFFER);
    curr_ptr = (*local_upids);
    for (i = 0; i < comm->local_size; i++) {
        memcpy(curr_ptr, &temp_buf[i * MPIDI_OFI_global.addrnamelen], (*local_upid_size)[i]);
        curr_ptr += (*local_upid_size)[i];
    }

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}
