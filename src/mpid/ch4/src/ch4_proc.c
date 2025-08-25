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
    MPIDI_global.avt_mgr.av_table0 = (MPIDI_av_table_t *) MPL_malloc(table_size, MPL_MEM_ADDRESS);
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

    MPL_free(MPIDI_global.avt_mgr.av_tables);
    memset(&MPIDI_global.avt_mgr, 0, sizeof(MPIDI_global.avt_mgr));

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

#ifdef MPIDI_BUILD_CH4_UPID_HASH
/* Store the upid, avtid, lpid in a hash to support get_local_upids and upids_to_lupids */
static MPIDI_upid_hash *upid_hash = NULL;

void MPIDIU_upidhash_add(const void *upid, int upid_len, int avtid, int lpid)
{
    MPIDI_upid_hash *t;
    t = MPL_malloc(sizeof(MPIDI_upid_hash), MPL_MEM_OTHER);
    t->avtid = avtid;
    t->lpid = lpid;
    t->upid = MPL_malloc(upid_len, MPL_MEM_OTHER);
    memcpy(t->upid, upid, upid_len);
    t->upid_len = upid_len;
    HASH_ADD_KEYPTR(hh, upid_hash, t->upid, upid_len, t, MPL_MEM_OTHER);

    MPIDIU_get_av(avtid, lpid).hash = t;
    /* Do not free avt while we use upidhash - FIXME: improve it */
    MPIDIU_avt_add_ref(avtid);
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
        MPIDIU_avt_release_ref(cur->avtid);
        MPL_free(cur->upid);
        MPL_free(cur);
    }
}
#endif

/* convert upid to gpid by netmod.
 * For ofi netmod, it inserts the address and fills an av entry.
 */
int MPIDIU_upids_to_lpids(int size, int *remote_upid_size, char *remote_upids,
                          MPIR_Lpid * remote_lpids)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    mpi_errno = MPIDI_NM_upids_to_lpids(size, remote_upid_size, remote_upids, remote_lpids);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_alloc_lut(MPIDI_rank_map_lut_t ** lut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_rank_map_lut_t *new_lut = NULL;

    MPIR_FUNC_ENTER;

    new_lut = (MPIDI_rank_map_lut_t *) MPL_malloc(sizeof(MPIDI_rank_map_lut_t)
                                                  + size * sizeof(MPIDI_lpid_t), MPL_MEM_ADDRESS);
    if (new_lut == NULL) {
        *lut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_cc_set(&new_lut->ref_count, 1);
    *lut = new_lut;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                    (MPL_DBG_FDEST, "alloc lut %p, size %lu, refcount=%d",
                     new_lut, size * sizeof(MPIDI_lpid_t), MPIR_cc_get(&new_lut->ref_count)));
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_release_lut(MPIDI_rank_map_lut_t * lut)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use = 0;

    MPIR_FUNC_ENTER;

    MPIR_cc_decr(&lut->ref_count, &in_use);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "dec ref to lut %p", lut));
    if (!in_use) {
        MPL_free(lut);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "free lut %p", lut));
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIU_alloc_mlut(MPIDI_rank_map_mlut_t ** mlut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_rank_map_mlut_t *new_mlut = NULL;

    MPIR_FUNC_ENTER;

    new_mlut = (MPIDI_rank_map_mlut_t *) MPL_malloc(sizeof(MPIDI_rank_map_mlut_t)
                                                    + size * sizeof(MPIDI_gpid_t), MPL_MEM_ADDRESS);
    if (new_mlut == NULL) {
        *mlut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_cc_set(&new_mlut->ref_count, 1);
    *mlut = new_mlut;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                    (MPL_DBG_FDEST, "alloc mlut %p, size %lu, refcount=%d",
                     new_mlut, size * sizeof(MPIDI_gpid_t), MPIR_cc_get(&new_mlut->ref_count)));
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_release_mlut(MPIDI_rank_map_mlut_t * mlut)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use = 0;

    MPIR_FUNC_ENTER;

    MPIR_cc_decr(&mlut->ref_count, &in_use);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "dec ref to mlut %p", mlut));
    if (!in_use) {
        MPL_free(mlut);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "free mlut %p", mlut));
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
