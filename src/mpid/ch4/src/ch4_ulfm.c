/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "uthash.h"

/* A ULFM Comm_agree algorithm without relying on PMI. Instead, it relies on
 * a send_probe function that sends an active message to a peer that should
 * return MPIX_ERR_PROC_FAILED if the peer is dead.
 *
 * The algorithm:
 *
 * * A power-of-2 hierarichical level with the least surviving rank as a level
 *   leader. For example, at level 0, every 2 ranks form a group; at level 1,
 *   every 4 ranks form a group; and so on.
 *
 * * First, start at level 0, each surviving rank probe its peer rank and collect
 *   rank maps for its group at the level. The leader rank increment its level
 *   and continue while the other rank go to the second part waitint for broadcast
 *   message from its peer. The surviving rank continue level up until one root
 *   rank remain at level q (ceil(log2(comm_size))) and collects the entire rank
 *   map.
 *
 * * Second, root decrement its level and send its peer the whole rank map. Every
 *   rank upon receiving broadcast decrement its level and continue the broadcast
 *   until every rank reach level 0 and finish.
 *
 * * If any of the broadcast fails (peer dead during the process), it will return
 *   MPIX_ERR_PROC_FAILED. Otherwise, it returns MPI_SUCCESS.
 */

#define DIR_UP   0
#define DIR_DOWN 1

typedef struct {
    int context_id;
    int comm_size;

    int flag;
    /* {up,down}_probes[level] is the latest epoch for the {up,down} probe
     * at this level that we received. */
    int *up_probes;
    int *down_probes;
    /* process_dead_map[rank] is true if rank is dead */
    bool *process_dead_map;
    /* failed_procs is a dynamic array of failed processes */
    int num_failed;
    int num_failed_max;
    int *failed_procs;

    UT_hash_handle hh;
} comm_agree_entry;

static comm_agree_entry *comm_agree_hash = NULL;

static int num_global_failed_procs;
static MPIR_Lpid *global_failed_procs;

static comm_agree_entry *find_comm_agree_entry(int context_id, int comm_size);
static void update_failed_proc(comm_agree_entry * entry, int rank);
static int send_probe(comm_agree_entry * entry, MPIR_Comm * comm,
                      int epoch, int level, int level_dir);
static int update_global_failed_procs(MPIR_Comm * comm, int num_failed, int *failed_procs);

int MPID_Comm_get_failed(MPIR_Comm * comm, MPIR_Group ** failed_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKPMEM_DECL();

    if (num_global_failed_procs == 0) {
        *failed_group_ptr = MPIR_Group_empty;
    } else {
        int num = 0;
        MPIR_Lpid *procs = NULL;
        for (int i = 0; i < num_global_failed_procs; i++) {
            int r = MPIR_Group_lpid_to_rank(comm->local_group, global_failed_procs[i]);
            if (r != MPI_UNDEFINED) {
                if (procs == NULL) {
                    MPIR_CHKPMEM_MALLOC(procs, num_global_failed_procs * sizeof(MPIR_Lpid),
                                        MPL_MEM_COMM);
                }
                procs[num++] = global_failed_procs[i];
            }
        }
        if (num > 0) {
            mpi_errno = MPIR_Group_create_map(num, -1, comm->session_ptr, procs, failed_group_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            *failed_group_ptr = MPIR_Group_empty;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPID_Comm_agree(MPIR_Comm * comm, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(0));

    comm_agree_entry *entry = find_comm_agree_entry(comm->context_id, comm->local_size);
    int epoch = (++MPIDI_COMM(comm, comm_agree_epoch));

    entry->flag &= *flag;

    int rank, comm_size;
    rank = comm->rank;
    comm_size = comm->local_size;

    int q = MPL_log2(comm_size);
    if ((1 << q) < comm_size) {
        q++;
    }

    /* probe at pof2 level */
    int level = 0;

    /* first ascending the level as we probe the peer. */
    while (level < q) {
        int peer_root = ((rank >> level) ^ 1) << level;
        mpi_errno = send_probe(entry, comm, epoch, level, DIR_UP);
        if (mpi_errno == MPI_SUCCESS) {
            /* probe sent, wait for reply */
            while (entry->up_probes[level] < epoch) {
                mpi_errno = MPIDI_progress_test_vci(0);
                MPIR_ERR_CHECK(mpi_errno);
            }

            if (rank > peer_root) {
                /* done at this level if peer is the lower rank */
                break;
            }
        }
        level += 1;
    }

    /* descending the level as we broadcast to peer the final map for failed processes */

    bool has_new_failures = false;
    /* skip level q since the last two surviving process already exchanged the
     * whole info */
    if (level == q) {
        level--;
    } else if (level < q - 1) {
        /* all other processes wait for the broadcast before proceed */
        while (entry->down_probes[level] < epoch) {
            mpi_errno = MPIDI_progress_test_vci(0);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    while (level > 0) {
        level--;
        mpi_errno = send_probe(entry, comm, epoch, level, DIR_DOWN);
        if (mpi_errno == MPIX_ERR_PROC_FAILED) {
            has_new_failures = true;
        }
    }

    mpi_errno = update_global_failed_procs(comm, entry->num_failed, entry->failed_procs);
    MPIR_ERR_CHECK(mpi_errno);

    if (has_new_failures) {
        mpi_errno = MPIX_ERR_PROC_FAILED;
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(0));
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ulfm active message probe */
static int ulfm_probe_origin_cb(MPIR_Request * req);
static int ulfm_probe_target_msg_cb(void *am_hdr, void *data, MPI_Aint data_sz, uint32_t attr,
                                    MPIR_Request ** req);

int MPIDI_ulfm_init(void)
{
    MPIDIG_am_reg_cb(MPIDI_ULFM_PROBE, &ulfm_probe_origin_cb, &ulfm_probe_target_msg_cb);
    return MPI_SUCCESS;
}

int MPIDI_ulfm_finalize(void)
{
    comm_agree_entry *entry, *tmp;

    HASH_ITER(hh, comm_agree_hash, entry, tmp) {
        HASH_DEL(comm_agree_hash, entry);

        MPL_free(entry->up_probes);
        MPL_free(entry->down_probes);
        MPL_free(entry->process_dead_map);
        MPL_free(entry->failed_procs);

        MPL_free(entry);
    }

    return MPI_SUCCESS;
}

struct probe_hdr {
    int context_id;
    int comm_size;
    int flag;
    int epoch;
    int level;
    int level_dir;              /* DIR_UP or DIR_DOWN */
    int origin_rank;
    int num_failed_procs;
};

static int ulfm_probe_origin_cb(MPIR_Request * sreq)
{
    MPIR_cc_dec(sreq->cc_ptr);
    return MPI_SUCCESS;
}

static int ulfm_probe_target_msg_cb(void *am_hdr, void *data, MPI_Aint data_sz, uint32_t attr,
                                    MPIR_Request ** req)
{
    struct probe_hdr *hdr = am_hdr;
    comm_agree_entry *entry = find_comm_agree_entry(hdr->context_id, hdr->comm_size);
    if (hdr->level_dir == DIR_UP) {
        entry->up_probes[hdr->level] = hdr->epoch;
        entry->flag &= hdr->flag;
    } else {
        entry->down_probes[hdr->level] = hdr->epoch;
        entry->flag = hdr->flag;
    }

    if (hdr->num_failed_procs > 0) {
        int *failed_procs = data;
        for (int i = 0; i < hdr->num_failed_procs; i++) {
            update_failed_proc(entry, failed_procs[i]);
        }
    }

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = NULL;
    }

    return MPI_SUCCESS;
}

/* ---- routines for sending probe ---- */

static int send_probe(comm_agree_entry * entry, MPIR_Comm * comm,
                      int epoch, int level, int level_dir)
{
    int mpi_errno = MPI_SUCCESS;

    int my_rank = comm->rank;
    int peer_root = ((my_rank >> level) ^ 1) << level;
    int peer_max = peer_root + (1 << level);
    if (peer_max > entry->comm_size) {
        peer_max = entry->comm_size;
    }
    /* try sending probe to peer group in rank order, stop at the first
     * peer that is successful (i.e. peer is not dead).
     */
    struct probe_hdr hdr;
    hdr.context_id = entry->context_id;
    hdr.comm_size = entry->comm_size;
    hdr.flag = entry->flag;
    hdr.epoch = epoch;
    hdr.level = level;
    hdr.level_dir = level_dir;
    hdr.origin_rank = my_rank;  /* for debugging purpose */
    hdr.num_failed_procs = entry->num_failed;

    int newly_failed_procs = 0;
    for (int r = peer_root; r < peer_max; r++) {
        if (!entry->process_dead_map[r]) {
            MPIR_Request *sreq;
            MPIDI_CH4_REQUEST_CREATE(sreq, MPIR_REQUEST_KIND__SEND,
                                     0 /* vci */ , 1 /* ref_count */);
            MPIR_ERR_CHKANDJUMP(!sreq, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
            MPIDI_NM_am_request_init(sreq);

            mpi_errno = MPIDI_NM_am_isend(r, comm, MPIDI_ULFM_PROBE, &hdr, sizeof(hdr),
                                          entry->failed_procs, entry->num_failed, MPIR_INT_INTERNAL,
                                          0, 0, sreq);
            if (mpi_errno == MPI_SUCCESS) {
                while (!MPIR_Request_is_complete(sreq)) {
                    mpi_errno = MPIDI_progress_test_vci(0);
                    MPIR_Assert(mpi_errno == MPI_SUCCESS);
                }
                if (sreq->status.MPI_ERROR == MPI_SUCCESS) {
                    /* at least one process is alive, we are done */
                    MPIR_Request_free_unsafe(sreq);
                    /* in the broadcast stage we need know if there is new failures */
                    if (newly_failed_procs && level_dir == DIR_DOWN) {
                        mpi_errno = MPIX_ERR_PROC_FAILED;
                    }
                    goto fn_exit;
                } else {
                    newly_failed_procs++;
                    update_failed_proc(entry, r);
                }
            } else {
                newly_failed_procs++;
                update_failed_proc(entry, r);
            }
        }
    }
    /* all processes in the peer group failed. We only need know is Comm_agree
     * during the UP (reduction) stage */
    if (level_dir == DIR_UP) {
        mpi_errno = MPIX_ERR_PROC_FAILED;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    /* TODO: check failed rank will return MPIX_ERR_PROC_FAILED */
    goto fn_exit;
}

/* routines for managing received probes */

static comm_agree_entry *find_comm_agree_entry(int context_id, int comm_size)
{
    comm_agree_entry *entry = NULL;

    /* Look up entry in hash table by context_id */
    HASH_FIND_INT(comm_agree_hash, &context_id, entry);

    if (entry == NULL) {
        /* Entry not found, allocate and add to hash */
        entry = MPL_malloc(sizeof(comm_agree_entry), MPL_MEM_OTHER);
        if (!entry) {
            goto fn_exit;
        }

        /* initialize the entry */
        entry->context_id = context_id;
        entry->comm_size = comm_size;

        entry->flag = 0xffffffff;

        int pof2 = MPL_pof2(comm_size);
        entry->up_probes = MPL_calloc(pof2, sizeof(int), MPL_MEM_OTHER);
        MPIR_Assert(entry->up_probes);
        entry->down_probes = MPL_calloc(pof2, sizeof(int), MPL_MEM_OTHER);
        MPIR_Assert(entry->down_probes);

        entry->process_dead_map = MPL_calloc(comm_size, sizeof(int), MPL_MEM_OTHER);
        MPIR_Assert(entry->process_dead_map);

        entry->num_failed = 0;
        entry->num_failed_max = 0;
        entry->failed_procs = NULL;

        /* Add to hash table */
        HASH_ADD_INT(comm_agree_hash, context_id, entry, MPL_MEM_OTHER);
    }

  fn_exit:
    return entry;
}

static void update_failed_proc(comm_agree_entry * entry, int rank)
{
    if (!entry->process_dead_map[rank]) {
        entry->process_dead_map[rank] = true;
        /* check the size of dynamic array */
        if (entry->num_failed + 1 > entry->num_failed_max) {
            int new_size = entry->num_failed_max ? entry->num_failed_max * 2 : 10;
            entry->failed_procs = MPL_realloc(entry->failed_procs, new_size * sizeof(int),
                                              MPL_MEM_OTHER);
            MPIR_Assert(entry->failed_procs);
            entry->num_failed_max = new_size;
        }
        /* add the entry */
        entry->failed_procs[entry->num_failed++] = rank;
    }
}

/* -- routine for update global_failed_procs -- */

/* qsort compare function */
static int sort_fn(const void *a, const void *b)
{
    return (*(MPIR_Lpid *) a - *(MPIR_Lpid *) b);
}

static int update_global_failed_procs(MPIR_Comm * comm, int num_failed, int *failed_procs)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int num_new_failed = 0;
    MPIR_Lpid *new_failed = NULL;
    for (int i = 0; i < num_failed; i++) {
        MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(comm, failed_procs[i]);
        bool is_new = true;
        for (int j = 0; j < num_global_failed_procs; j++) {
            if (global_failed_procs[j] == lpid) {
                is_new = false;
                break;
            }
        }
        if (is_new) {
            if (!new_failed) {
                MPIR_CHKLMEM_MALLOC(new_failed, num_failed * sizeof(MPIR_Lpid));
            }
            new_failed[num_new_failed++] = lpid;
        }
    }

    if (num_new_failed > 0) {
        int num = num_global_failed_procs + num_new_failed;
        global_failed_procs = MPL_realloc(global_failed_procs, num * sizeof(MPIR_Lpid),
                                          MPL_MEM_OTHER);
        for (int i = 0; i < num_new_failed; i++) {
            global_failed_procs[num_global_failed_procs++] = new_failed[i];
        }
        qsort(global_failed_procs, num, sizeof(MPIR_Lpid), sort_fn);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
