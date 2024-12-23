/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ch4_proc.h"
#include "uthash.h"

/* There are 3 terms referencing processes -- upid, lpid, gpid.
 * Refer to comment in ch4_proc.h
 */

#define MPIDI_CH4_AVTABLE_USE_DDR    1

static int get_next_avtid(void);

int MPIDIU_get_node_id(MPIR_Comm * comm, int rank, int *id_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    *id_p = MPIDIU_comm_rank_to_av(comm, rank)->node_id;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIU_get_n_avts(void)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_global.avt_mgr.n_avts;

    MPIR_FUNC_EXIT;
    return ret;
}

int MPIDIU_get_avt_size(int avtid)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_global.avt_mgr.av_tables[avtid]->size;

    MPIR_FUNC_EXIT;
    return ret;
}

static int get_next_avtid(void)
{
    /* return a free entry if we have one */
    if (MPIDI_global.avt_mgr.n_free > 0) {
        /* find a free one */
        for (int i = 0; i < MPIDI_global.avt_mgr.n_avts; i++) {
            if (MPIDI_global.avt_mgr.av_tables[i] == NULL) {
                MPIDI_global.avt_mgr.n_free--;
                return i;
            }
        }
        MPIR_Assert(0);
    }

    /* check if we need grow the tables */
    if (MPIDI_global.avt_mgr.max_n_avts == 0) {
        /* allocate the initial tables */
        MPIDI_global.avt_mgr.max_n_avts = 10;
        size_t table_size = MPIDI_global.avt_mgr.max_n_avts * sizeof(MPIDI_av_table_t *);
        MPIDI_global.avt_mgr.av_tables = MPL_malloc(table_size, MPL_MEM_ADDRESS);
        MPIR_Assert(MPIDI_global.avt_mgr.av_tables);
    } else if (MPIDI_global.avt_mgr.n_avts + 1 > MPIDI_global.avt_mgr.max_n_avts) {
        /* grow the tables */
        MPIDI_global.avt_mgr.max_n_avts *= 2;
        size_t table_size = MPIDI_global.avt_mgr.max_n_avts * sizeof(MPIDI_av_table_t *);
        MPIDI_global.avt_mgr.av_tables = MPL_realloc(MPIDI_global.avt_mgr.av_tables,
                                                     table_size, MPL_MEM_ADDRESS);
    }

    /* return the next available entry */
    return MPIDI_global.avt_mgr.n_avts++;
}

int MPIDIU_new_avt(int size, int *avtid)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_table_t *new_av_table;

    MPIR_FUNC_ENTER;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " new_avt: size=%d", size));

    *avtid = get_next_avtid();

    /* note: zeroed so is_local default to 0, which is true for *avtid > 0 */
    new_av_table = (MPIDI_av_table_t *) MPL_calloc(1, size * sizeof(MPIDI_av_entry_t)
                                                   + sizeof(MPIDI_av_table_t), MPL_MEM_ADDRESS);
    new_av_table->size = size;
    for (int i = 0; i < size; i++) {
        new_av_table->table[i].node_id = -1;
    }
    MPIDI_global.avt_mgr.av_tables[*avtid] = new_av_table;

    MPIR_cc_set(&MPIDI_global.avt_mgr.av_tables[*avtid]->ref_count, 0);

    /* TODO: to support dynamic processes and dynamic av insertions, we need device hooks to initialize table with invalid entries */

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIU_free_avt(int avtid)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);

    MPL_free(MPIDI_global.avt_mgr.av_tables[avtid]);
    MPIDI_global.avt_mgr.av_tables[avtid] = NULL;
    MPIDI_global.avt_mgr.n_free++;

    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIU_avt_add_ref(int avtid)
{
    MPIR_FUNC_ENTER;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " incr avtid=%d", avtid));
    MPIR_cc_inc(&MPIDI_global.avt_mgr.av_tables[avtid]->ref_count);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIDIU_avt_release_ref(int avtid)
{
    int in_use;

    MPIR_FUNC_ENTER;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " decr avtid=%d", avtid));
    MPIR_cc_decr(&MPIDI_global.avt_mgr.av_tables[avtid]->ref_count, &in_use);
    if (!in_use) {
        MPIDIU_free_avt(avtid);
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static void init_dynamic_av_table(void);
static void destroy_dynamic_av_table(void);

int MPIDIU_avt_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int first_avtid ATTRIBUTE((unused));
    first_avtid = get_next_avtid();
    MPIR_Assert(first_avtid == 0);

    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;
    size_t table_size = sizeof(MPIDI_av_table_t) + size * sizeof(MPIDI_av_entry_t);
    MPIDI_global.avt_mgr.av_table0 = MPL_calloc(1, table_size, MPL_MEM_ADDRESS);
    MPIR_Assert(MPIDI_global.avt_mgr.av_table0);

#if MPIDI_CH4_AVTABLE_USE_DDR
    MPIR_hwtopo_gid_t mem_gid = MPIR_hwtopo_get_obj_by_type(MPIR_HWTOPO_TYPE__DDR);
    if (mem_gid != MPIR_HWTOPO_GID_ROOT) {
        MPIR_hwtopo_mem_bind(MPIDI_global.avt_mgr.av_table0, table_size, mem_gid);
    }
#endif

    MPIDI_global.avt_mgr.av_table0->size = size;
    MPIR_cc_set(&MPIDI_global.avt_mgr.av_table0->ref_count, 1);

    for (int i = 0; i < size; i++) {
        MPIDI_global.avt_mgr.av_table0->table[i].is_local =
            (MPIR_Process.node_map[i] == MPIR_Process.node_map[rank]) ? 1 : 0;
        MPIDI_global.avt_mgr.av_table0->table[i].node_id = MPIR_Process.node_map[i];
    }

    MPIDI_global.avt_mgr.av_tables[0] = MPIDI_global.avt_mgr.av_table0;

    init_dynamic_av_table();

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIU_avt_destroy(void)
{
    MPIR_FUNC_ENTER;

    for (int i = 0; i < MPIDI_global.avt_mgr.n_avts; i++) {
        if (MPIDI_global.avt_mgr.av_tables[i] != NULL) {
            MPIDIU_avt_release_ref(i);
            /*TODO: Check all references is cleared and the entry is set to NULL */
        }
    }

    destroy_dynamic_av_table();

    MPL_free(MPIDI_global.avt_mgr.av_tables);
    memset(&MPIDI_global.avt_mgr, 0, sizeof(MPIDI_global.avt_mgr));

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

#define MPIDIU_DYN_AV_TABLE MPIDI_global.avt_mgr.dynamic_av_table
#define MPIDIU_DYN_AV(idx) (MPIDI_av_entry_t *)((char *) MPIDI_global.avt_mgr.dynamic_av_table.table + (idx) * sizeof(MPIDI_av_entry_t))

static void init_dynamic_av_table(void)
{
    /* allocate dynamic_av_table */
    int table_size = MPIDIU_DYNAMIC_AV_MAX * sizeof(MPIDI_av_entry_t);
    MPIDIU_DYN_AV_TABLE.table = MPL_malloc(table_size, MPL_MEM_ADDRESS);
    MPIDIU_DYN_AV_TABLE.size = 0;
}

static void destroy_dynamic_av_table(void)
{
    MPIR_Assert(MPIDIU_DYN_AV_TABLE.size == 0);
    MPL_free(MPIDIU_DYN_AV_TABLE.table);
}

/* NOTE: The following functions --
 *    * MPIDIU_insert_dynamic_upid
 *    * MPIDIU_free_dynamic_lpid
 *    * MPIDIU_find_dynamic_av
 * are thread-unsafe. Caller should enter (VCI-0) critical section.
 */

int MPIDIU_insert_dynamic_upid(MPIR_Lpid * lpid_out, const char *upid, int upid_len)
{
    int mpi_errno = MPI_SUCCESS;

    /* allocate idx from dynamic av table */
    int idx = MPIDIU_DYN_AV_TABLE.size;
    for (int i = 0; i < MPIDIU_DYN_AV_TABLE.size; i++) {
        if (MPIDIU_DYN_AV_TABLE.upids[i] == NULL) {
            idx = i;
            break;
        }
    }
    if (idx == MPIDIU_DYN_AV_TABLE.size) {
        MPIDIU_DYN_AV_TABLE.size++;
        if (MPIDIU_DYN_AV_TABLE.size >= MPIDIU_DYNAMIC_AV_MAX) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**intern");
        }
    }

    /* copy the upid */
    char *upid_copy = MPL_malloc(upid_len, MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!upid_copy, mpi_errno, MPI_ERR_OTHER, "**nomem");
    memcpy(upid_copy, upid, upid_len);

    MPIDIU_DYN_AV_TABLE.upids[idx] = upid_copy;
    MPIDIU_DYN_AV_TABLE.upid_sizes[idx] = upid_len;

    /* insert upid */
    *lpid_out = MPIR_LPID_DYNAMIC_MASK | idx;

    mpi_errno = MPIDI_NM_insert_upid(*lpid_out, upid, upid_len);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return MPI_SUCCESS;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_free_dynamic_lpid(MPIR_Lpid lpid)
{
    MPIR_Assert(lpid & MPIR_LPID_DYNAMIC_MASK);
    int idx = lpid & (~MPIR_LPID_DYNAMIC_MASK);
    MPIR_Assert(idx >= 0 && idx < MPIDIU_DYN_AV_TABLE.size);

    /* free the upid buffer */
    MPL_free((char *) MPIDIU_DYN_AV_TABLE.upids[idx]);
    /* mark the av as free by setting upid to NULL and upid_size to 0 */
    MPIDIU_DYN_AV_TABLE.upids[idx] = NULL;
    MPIDIU_DYN_AV_TABLE.upid_sizes[idx] = 0;

    /* if the last entry is empty, reduce size */
    while (MPIDIU_DYN_AV_TABLE.size > 0 &&
           MPIDIU_DYN_AV_TABLE.upids[MPIDIU_DYN_AV_TABLE.size - 1] == NULL) {
        MPIDIU_DYN_AV_TABLE.size--;
    }

    return MPI_SUCCESS;
}

MPIDI_av_entry_t *MPIDIU_find_dynamic_av(const char *upid, int upid_len)
{
    for (int i = 0; i < MPIDIU_DYN_AV_TABLE.size; i++) {
        if (MPIDIU_DYN_AV_TABLE.upid_sizes[i] == upid_len &&
            memcmp(MPIDIU_DYN_AV_TABLE.upids[i], upid, upid_len) == 0) {
            return MPIDIU_DYN_AV(i);
        }
    }
    return NULL;
}

/* this version handles dynamic av or av entries that are not allocated yet (e.g. new world)
 */
MPIDI_av_entry_t *MPIDIU_lpid_to_av_slow(MPIR_Lpid lpid)
{
    if (lpid & MPIR_LPID_DYNAMIC_MASK) {
        int idx = lpid & (~MPIR_LPID_DYNAMIC_MASK);
        MPIR_Assert(idx >= 0 && idx < MPIDIU_DYN_AV_TABLE.size);
        return &MPIDIU_DYN_AV_TABLE.table[idx];
    } else {
        int world_idx = MPIR_LPID_WORLD_INDEX(lpid);
        int world_rank = MPIR_LPID_WORLD_RANK(lpid);

        MPIR_Assert(world_rank < MPIR_Worlds[world_idx].num_procs);

        if (world_idx >= MPIDI_global.avt_mgr.n_avts) {
            for (int i = MPIDI_global.avt_mgr.n_avts; i < world_idx + 1; i++) {
                int avtid;
                MPIDIU_new_avt(MPIR_Worlds[i].num_procs, &avtid);
                MPIR_Assert(avtid == i);
            }
        }

        return &MPIDI_global.avt_mgr.av_tables[world_idx]->table[world_rank];
    }
}

#ifdef MPIDI_BUILD_CH4_UPID_HASH
/* Store the upid, avtid, lpid in a hash to support get_local_upids and upids_to_lupids */
static MPIDI_upid_hash *upid_hash = NULL;

void MPIDIU_upidhash_add(const void *upid, int upid_len, MPIR_Lpid lpid)
{
    MPIDI_upid_hash *t;
    t = MPL_malloc(sizeof(MPIDI_upid_hash), MPL_MEM_OTHER);
    t->lpid = lpid;
    t->upid = MPL_malloc(upid_len, MPL_MEM_OTHER);
    memcpy(t->upid, upid, upid_len);
    t->upid_len = upid_len;
    HASH_ADD_KEYPTR(hh, upid_hash, t->upid, upid_len, t, MPL_MEM_OTHER);

    MPIDIU_lpid_to_av(lpid)->hash = t;
}

MPIDI_upid_hash *MPIDIU_upidhash_find(const void *upid, int upid_len)
{
    MPIDI_upid_hash *t;
    HASH_FIND(hh, upid_hash, upid, upid_len, t);
    return t;
}

void MPIDIU_upidhash_free(void)
{
    MPIDI_upid_hash *cur, *tmp;
    HASH_ITER(hh, upid_hash, cur, tmp) {
        HASH_DEL(upid_hash, cur);
        MPL_free(cur->upid);
        MPL_free(cur);
    }
}
#endif
