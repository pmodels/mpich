/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

#define PORT_NAME_TAG_KEY "tag"
#define CONNENTR_TAG_KEY "connentry"

#define DYNPROC_RECEIVER 0
#define DYNPROC_SENDER 1

/* NOTE: dynamics process support is limited to single VCI for now.
 * TODO: assert MPIDI_global.n_vcis == 1.
 */

static void free_port_name_tag(int tag);
static int get_port_name_tag(int *port_name_tag);
static int get_tag_from_port(const char *port_name, int *port_name_tag);
static int get_conn_name_from_port(const char *port_name, char *connname);
static int peer_intercomm_create(char *remote_addrname, int len, int tag, int timeout,
                                 bool is_sender, MPIR_Comm ** newcomm);
static int get_my_addrname(char *addrname_buf, int *len);
static int dynamic_send(uint64_t remote_gpid, int tag, const void *buf, int size, int timeout);
static int dynamic_recv(int tag, void *buf, int size, int timeout);
static int dynamic_intercomm_create(const char *port_name, MPIR_Info * info, int root,
                                    MPIR_Comm * comm_ptr, int timeout, bool is_sender,
                                    MPIR_Comm ** newcomm);

/* NOTE: port_name_tag, context_id_offset, and port_id all refer to the same context_id used during
 * establishing dynamic connections */
static void free_port_name_tag(int tag)
{
    int idx, rem_tag;

    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);

    idx = tag / (sizeof(int) * 8);
    rem_tag = tag - (idx * sizeof(int) * 8);

    MPIDI_OFI_global.port_name_tag_mask[idx] &= ~(1u << ((8 * sizeof(int)) - 1 - rem_tag));

    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_EXIT;
}

static int get_port_name_tag(int *port_name_tag)
{
    unsigned i, j;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);

    for (i = 0; i < MPIR_MAX_CONTEXT_MASK; i++)
        if (MPIDI_OFI_global.port_name_tag_mask[i] != ~0)
            break;

    if (i < MPIR_MAX_CONTEXT_MASK)
        for (j = 0; j < (8 * sizeof(int)); j++) {
            if ((MPIDI_OFI_global.port_name_tag_mask[i] | (1u << ((8 * sizeof(int)) - j - 1))) !=
                MPIDI_OFI_global.port_name_tag_mask[i]) {
                MPIDI_OFI_global.port_name_tag_mask[i] |= (1u << ((8 * sizeof(int)) - j - 1));
                *port_name_tag = ((i * 8 * sizeof(int)) + j);
                goto fn_exit;
            }
    } else
        goto fn_fail;

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    *port_name_tag = -1;
    mpi_errno = MPI_ERR_OTHER;
    goto fn_exit;
}

static int get_tag_from_port(const char *port_name, int *port_name_tag)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPL_SUCCESS;

    MPIR_FUNC_ENTER;

    if (strlen(port_name) == 0)
        goto fn_exit;

    str_errno = MPL_str_get_int_arg(port_name, PORT_NAME_TAG_KEY, port_name_tag);
    MPIR_ERR_CHKANDJUMP(str_errno, mpi_errno, MPI_ERR_OTHER, "**argstr_no_port_name_tag");
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_conn_name_from_port(const char *port_name, char *connname)
{
    int mpi_errno = MPI_SUCCESS;
    int maxlen = MPIDI_KVSAPPSTRLEN;

    MPIR_FUNC_ENTER;

    /* WB TODO - Only setting up nic 0 for spawn right now. */
    MPL_str_get_binary_arg(port_name, CONNENTR_TAG_KEY, connname, MPIDI_OFI_global.addrnamelen,
                           &maxlen);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                               MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = dynamic_intercomm_create(port_name, info, root, comm_ptr, timeout, true, newcomm);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Comm_free_impl(comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPL_SUCCESS;
    int port_name_tag = 0;
    int len = MPI_MAX_PORT_NAME;
    MPIR_FUNC_ENTER;

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        MPIR_Assert(0);
        goto fn_exit;
    }

    mpi_errno = get_port_name_tag(&port_name_tag);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_OFI_STR_CALL(MPL_str_add_int_arg(&port_name, &len, PORT_NAME_TAG_KEY, port_name_tag),
                       port_str);
    /* WB TODO - Only setting up nic 0 for spawn right now. */
    MPIDI_OFI_STR_CALL(MPL_str_add_binary_arg(&port_name, &len, CONNENTR_TAG_KEY,
                                              MPIDI_OFI_global.addrname[0],
                                              MPIDI_OFI_global.addrnamelen), port_str);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_close_port(const char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    int port_name_tag;

    MPIR_FUNC_ENTER;

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        MPIR_Assert(0);
        goto fn_exit;
    }

    mpi_errno = get_tag_from_port(port_name, &port_name_tag);
    free_port_name_tag(port_name_tag);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static int dynamic_send(uint64_t remote_gpid, int tag, const void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    int avtid = MPIDIU_GPID_GET_AVTID(remote_gpid);
    int lpid = MPIDIU_GPID_GET_LPID(remote_gpid);
    fi_addr_t remote_addr = MPIDI_OFI_av_to_phys(&MPIDIU_get_av(avtid, lpid), 0, 0, 0);

    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
    uint64_t match_bits;
    match_bits = MPIDI_OFI_init_sendtag(0, tag, MPIDI_OFI_DYNPROC_SEND);

    MPL_time_t time_start, time_now;
    double time_gap;
    MPL_wtime(&time_start);
    MPIDI_OFI_VCI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[0].tx,
                                          buf, size, NULL /* desc */ , 0,
                                          remote_addr, match_bits, (void *) &req.context),
                             0, tsenddata, FALSE /* eagain */);
    do {
        mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_wtime(&time_now);
        MPL_wtime_diff(&time_start, &time_now, &time_gap);
    } while (!req.done && (timeout == 0 || time_gap < (double) timeout));

    if (!req.done) {
        /* FIXME: better error message */
        mpi_errno = MPI_ERR_PORT;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int dynamic_recv(int tag, void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
    uint64_t match_bits = 0;
    uint64_t mask_bits = 0;
    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, 0, tag);
    match_bits |= MPIDI_OFI_DYNPROC_SEND;

    MPL_time_t time_start, time_now;
    double time_gap;
    MPL_wtime(&time_start);
    MPIDI_OFI_VCI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[0].rx,
                                      buf, size, NULL,
                                      FI_ADDR_UNSPEC, match_bits, mask_bits, &req.context),
                             0, trecv, FALSE);
    do {
        mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_wtime(&time_now);
        MPL_wtime_diff(&time_start, &time_now, &time_gap);
    } while (!req.done && (timeout == 0 || time_gap < (double) timeout));

    if (!req.done) {
        /* FIXME: better error message */
        mpi_errno = MPI_ERR_PORT;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_my_addrname(char *addrname_buf, int *len)
{
    MPIR_Assert(*len >= MPIDI_OFI_global.addrnamelen);
    *len = MPIDI_OFI_global.addrnamelen;
    memcpy(addrname_buf, MPIDI_OFI_global.addrname[0], MPIDI_OFI_global.addrnamelen);
    return MPI_SUCCESS;
}

#define MPIDI_DYNPROC_NAME_MAX FI_NAME_MAX
struct dynproc_conn_hdr {
    MPIR_Context_id_t context_id;
    int addrname_len;
    char addrname[MPIDI_DYNPROC_NAME_MAX];
};

static int peer_intercomm_create(char *remote_addrname, int len, int tag,
                                 int timeout, bool is_sender, MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t context_id, recvcontext_id;
    uint64_t remote_gpid;

    mpi_errno = MPIR_Get_contextid_sparse(MPIR_Process.comm_self, &recvcontext_id, FALSE);
    MPIR_ERR_CHECK(mpi_errno);

    struct dynproc_conn_hdr hdr;
    if (is_sender) {
        /* insert remote address */
        int addrname_len = len;
        uint64_t *remote_gpids = &remote_gpid;
        mpi_errno = MPIDIU_upids_to_gpids(1, &addrname_len, remote_addrname, remote_gpids);
        MPIR_ERR_CHECK(mpi_errno);

        /* send remote context_id + addrname */
        hdr.context_id = recvcontext_id;
        hdr.addrname_len = MPIDI_DYNPROC_NAME_MAX;
        mpi_errno = get_my_addrname(hdr.addrname, &hdr.addrname_len);
        MPIR_ERR_CHECK(mpi_errno);

        int hdr_sz = sizeof(hdr) - MPIDI_DYNPROC_NAME_MAX + hdr.addrname_len;
        mpi_errno = dynamic_send(remote_gpid, tag, &hdr, hdr_sz, timeout);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = dynamic_recv(tag, &hdr, sizeof(hdr), timeout);
        MPIR_ERR_CHECK(mpi_errno);
        context_id = hdr.context_id;
    } else {
        /* recv remote address */
        mpi_errno = dynamic_recv(tag, &hdr, sizeof(hdr), timeout);
        MPIR_ERR_CHECK(mpi_errno);
        context_id = hdr.context_id;

        /* insert remote address */
        int addrname_len = hdr.addrname_len;
        uint64_t *remote_gpids = &remote_gpid;
        mpi_errno = MPIDIU_upids_to_gpids(1, &addrname_len, hdr.addrname, remote_gpids);
        MPIR_ERR_CHECK(mpi_errno);

        /* send remote context_id */
        hdr.context_id = recvcontext_id;
        mpi_errno = dynamic_send(remote_gpid, tag, &hdr, sizeof(hdr.context_id), timeout);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* create peer intercomm */
    mpi_errno = MPIR_peer_intercomm_create(context_id, recvcontext_id,
                                           remote_gpid, is_sender, newcomm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (recvcontext_id) {
        MPIR_Free_contextid(recvcontext_id);
    }
    goto fn_exit;
}

static int dynamic_intercomm_create(const char *port_name, MPIR_Info * info, int root,
                                    MPIR_Comm * comm_ptr, int timeout, bool is_sender,
                                    MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *peer_intercomm = NULL;
    int tag;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int bcast_ints[2];          /* used to bcast tag and errno */

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        MPIR_Assert(0);
        goto fn_exit;
    }

    if (comm_ptr->rank == root) {
        /* NOTE: do not goto fn_fail on error, or it will leave children hanging */
        mpi_errno = get_tag_from_port(port_name, &tag);
        if (mpi_errno)
            goto bcast_tag_and_errno;

        char remote_addrname[FI_NAME_MAX];
        char *addrname;
        int len;
        if (is_sender) {
            mpi_errno = get_conn_name_from_port(port_name, remote_addrname);
            if (mpi_errno)
                goto bcast_tag_and_errno;
            addrname = remote_addrname;
            len = MPIDI_OFI_global.addrnamelen;
        } else {
            /* TODO: check port_name matches MPIDI_OFI_global.addrname[0] */
            /* Use NULL for better error behavior */
            addrname = NULL;
            len = 0;
        }
        mpi_errno = peer_intercomm_create(addrname, len, tag, timeout, is_sender, &peer_intercomm);

      bcast_tag_and_errno:
        bcast_ints[0] = tag;
        bcast_ints[1] = mpi_errno;
        mpi_errno = MPIR_Bcast_allcomm_auto(bcast_ints, 2, MPI_INT, root, comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = bcast_ints[1];
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIR_Bcast_allcomm_auto(bcast_ints, 2, MPI_INT, root, comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        if (bcast_ints[1]) {
            /* errno from root cannot be directly returned */
            MPIR_ERR_SET(mpi_errno, MPI_ERR_PORT, "**comm_connect_fail");
            goto fn_fail;
        }
        tag = bcast_ints[0];
    }

    mpi_errno = MPIR_Intercomm_create_impl(comm_ptr, root, peer_intercomm, 0, tag, newcomm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (peer_intercomm) {
        MPIR_Comm_free_impl(peer_intercomm);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root,
                              MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = dynamic_intercomm_create(port_name, info, root, comm_ptr, 0, false, newcomm);

    MPIR_FUNC_EXIT;
    return mpi_errno;
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
