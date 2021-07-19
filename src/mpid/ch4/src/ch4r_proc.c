/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ch4r_proc.h"

static int get_next_avtid(int *avtid);
static int free_avtid(int avtid);

int MPIDIU_get_node_id(MPIR_Comm * comm, int rank, int *id_p)
{
    int mpi_errno = MPI_SUCCESS;
    int avtid = 0, lpid = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_NODE_ID);

    MPIDIU_comm_rank_to_pid(comm, rank, &lpid, &avtid);
    if (avtid != 0) {
        *id_p = -1;
    } else {
        *id_p = MPIR_Process.node_map[lpid];
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_NODE_ID);
    return mpi_errno;
}

int MPIDIU_get_n_avts(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_N_AVTS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_N_AVTS);

    ret = MPIDI_global.avt_mgr.n_avts;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_N_AVTS);
    return ret;
}

int MPIDIU_get_max_n_avts(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_MAX_N_AVTS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_MAX_N_AVTS);

    ret = MPIDI_global.avt_mgr.max_n_avts;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_MAX_N_AVTS);
    return ret;
}

int MPIDIU_get_avt_size(int avtid)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_AVT_SIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_AVT_SIZE);

    ret = MPIDI_global.avt_mgr.av_tables[avtid]->size;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_AVT_SIZE);
    return ret;
}

static int get_next_avtid(int *avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_NEXT_AVTID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_NEXT_AVTID);

    if (MPIDI_global.avt_mgr.next_avtid == -1) {        /* out of free avtids */
        int old_max, new_max, i;
        old_max = MPIDI_global.avt_mgr.max_n_avts;
        new_max = old_max + 1;
        MPIDI_global.avt_mgr.free_avtid =
            (int *) MPL_realloc(MPIDI_global.avt_mgr.free_avtid, new_max * sizeof(int),
                                MPL_MEM_ADDRESS);
        for (i = old_max; i < new_max - 1; i++) {
            MPIDI_global.avt_mgr.free_avtid[i] = i + 1;
        }
        MPIDI_global.avt_mgr.free_avtid[new_max - 1] = -1;
        MPIDI_global.avt_mgr.max_n_avts = new_max;
        MPIDI_global.avt_mgr.next_avtid = old_max;
    }

    *avtid = MPIDI_global.avt_mgr.next_avtid;
    MPIDI_global.avt_mgr.next_avtid = MPIDI_global.avt_mgr.free_avtid[*avtid];
    MPIDI_global.avt_mgr.free_avtid[*avtid] = -1;
    MPIDI_global.avt_mgr.n_avts++;
    MPIR_Assert(MPIDI_global.avt_mgr.n_avts <= MPIDI_global.avt_mgr.max_n_avts);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " new_avtid=%d", *avtid));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_NEXT_AVTID);
    return *avtid;
}

static int free_avtid(int avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_FREE_AVTID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_FREE_AVTID);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " free_avtid=%d", avtid));
    MPIR_Assert(MPIDI_global.avt_mgr.n_avts > 0);
    MPIDI_global.avt_mgr.free_avtid[avtid] = MPIDI_global.avt_mgr.next_avtid;
    MPIDI_global.avt_mgr.next_avtid = avtid;
    MPIDI_global.avt_mgr.n_avts--;
    /*
     * TODO:
     * If the allowed number of process groups are very large
     * We need to return unused pages back to OS.
     * madvise(addr, len, MADV_DONTNEED);
     * We need tracking which page can be returned
     */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_FREE_AVTID);
    return 0;
}

int MPIDIU_new_avt(int size, int *avtid)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_table_t *new_av_table;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_NEW_AVT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_NEW_AVT);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " new_avt: size=%d", size));

    get_next_avtid(avtid);

    new_av_table = (MPIDI_av_table_t *) MPL_malloc(size * sizeof(MPIDI_av_entry_t)
                                                   + sizeof(MPIDI_av_table_t), MPL_MEM_ADDRESS);
    new_av_table->size = size;
    MPIDI_global.avt_mgr.av_tables[*avtid] = new_av_table;

    MPIR_Object_set_ref(MPIDI_global.avt_mgr.av_tables[*avtid], 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_NEW_AVT);
    return mpi_errno;
}

int MPIDIU_free_avt(int avtid)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_FREE_AVT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_FREE_AVT);
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);

    MPL_free(MPIDI_global.avt_mgr.av_tables[avtid]);
    MPIDI_global.avt_mgr.av_tables[avtid] = NULL;
    free_avtid(avtid);

    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_FREE_AVT);
    return mpi_errno;
}

int MPIDIU_avt_add_ref(int avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_AVT_ADD_REF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_AVT_ADD_REF);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " incr avtid=%d", avtid));
    MPIR_Object_add_ref(MPIDI_global.avt_mgr.av_tables[avtid]);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_AVT_ADD_REF);
    return MPI_SUCCESS;
}

int MPIDIU_avt_release_ref(int avtid)
{
    int in_use;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_AVT_RELEASE_REF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_AVT_RELEASE_REF);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " decr avtid=%d", avtid));
    MPIR_Object_release_ref(MPIDIU_get_av_table(avtid), &in_use);
    if (!in_use) {
        MPIDIU_free_avt(avtid);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_AVT_RELEASE_REF);
    return MPI_SUCCESS;
}

#define AVT_SIZE (8 * 4 * 1024) /* FIXME: what is this size? */

int MPIDIU_avt_init(void)
{
    int i, mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_AVT_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_AVT_INIT);

    MPIDI_global.avt_mgr.max_n_avts = 1;
    MPIDI_global.avt_mgr.next_avtid = 0;
    MPIDI_global.avt_mgr.n_avts = 0;

    MPIDI_global.avt_mgr.av_tables = (MPIDI_av_table_t **) MPL_malloc(AVT_SIZE, MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_global.avt_mgr.av_tables == NULL,
                        mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    MPIDI_global.avt_mgr.free_avtid =
        (int *) MPL_malloc(MPIDI_global.avt_mgr.max_n_avts * sizeof(int), MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_global.avt_mgr.free_avtid == NULL, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    for (i = 0; i < MPIDI_global.avt_mgr.max_n_avts - 1; i++) {
        MPIDI_global.avt_mgr.free_avtid[i] = i + 1;
    }
    MPIDI_global.avt_mgr.free_avtid[MPIDI_global.avt_mgr.max_n_avts - 1] = -1;

    int first_avtid;
    get_next_avtid(&first_avtid);
    MPIR_Assert(first_avtid == 0);

    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;
    size_t table_size = sizeof(MPIDI_av_table_t) + size * sizeof(MPIDI_av_entry_t);
    MPIDI_av_table0 = (MPIDI_av_table_t *) MPL_malloc(table_size, MPL_MEM_ADDRESS);
    MPIDI_av_table0->size = size;
    MPIR_Object_set_ref(MPIDI_av_table0, 1);

#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    for (i = 0; i < size; i++) {
        MPIDI_av_table0->table[i].is_local =
            (MPIR_Process.node_map[i] == MPIR_Process.node_map[rank]) ? 1 : 0;
    }
#endif

    MPIDI_global.avt_mgr.av_tables[0] = MPIDI_av_table0;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_AVT_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_avt_destroy(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_AVT_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_AVT_DESTROY);

    int max_n_avts = MPIDIU_get_max_n_avts();
    for (int i = 0; i < max_n_avts; i++) {
        if (MPIDI_global.avt_mgr.av_tables[i] != NULL) {
            MPIDIU_avt_release_ref(i);
            /*TODO: Check all references is cleared and the entry is set to NULL */
        }
    }

    MPL_free(MPIDI_global.avt_mgr.av_tables);
    MPL_free(MPIDI_global.avt_mgr.free_avtid);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_AVT_DESTROY);
    return MPI_SUCCESS;
}

int MPIDIU_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                           int **remote_lupids)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_UPIDS_TO_LUPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_UPIDS_TO_LUPIDS);

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    mpi_errno = MPIDI_NM_upids_to_lupids(size, remote_upid_size, remote_upids, remote_lupids);
    MPIR_ERR_CHECK(mpi_errno);

    /* update locality */
    for (i = 0; i < size; i++) {
        int _avtid = 0, _lpid = 0;
        /* if this is a new process, update locality */
        if (MPIDIU_LUPID_IS_NEW_AVT((*remote_lupids)[i])) {
            MPIDIU_LUPID_CLEAR_NEW_AVT_MARK((*remote_lupids)[i]);
            _avtid = MPIDIU_LUPID_GET_AVTID((*remote_lupids)[i]);
            _lpid = MPIDIU_LUPID_GET_LPID((*remote_lupids)[i]);
            if (_avtid != 0) {
#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
                MPIDI_global.avt_mgr.av_tables[_avtid]->table[_lpid].is_local = 0;
#endif
            }
        }
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_UPIDS_TO_LUPIDS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_alloc_lut(MPIDI_rank_map_lut_t ** lut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_rank_map_lut_t *new_lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ALLOC_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ALLOC_LUT);

    new_lut = (MPIDI_rank_map_lut_t *) MPL_malloc(sizeof(MPIDI_rank_map_lut_t)
                                                  + size * sizeof(MPIDI_lpid_t), MPL_MEM_ADDRESS);
    if (new_lut == NULL) {
        *lut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_Object_set_ref(new_lut, 1);
    *lut = new_lut;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                    (MPL_DBG_FDEST, "alloc lut %p, size %lu, refcount=%d",
                     new_lut, size * sizeof(MPIDI_lpid_t), MPIR_Object_get_ref(new_lut)));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ALLOC_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_release_lut(MPIDI_rank_map_lut_t * lut)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_RELEASE_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_RELEASE_LUT);

    MPIR_Object_release_ref(lut, &in_use);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "dec ref to lut %p", lut));
    if (!in_use) {
        MPL_free(lut);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "free lut %p", lut));
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_RELEASE_LUT);
    return mpi_errno;
}

int MPIDIU_alloc_mlut(MPIDI_rank_map_mlut_t ** mlut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_rank_map_mlut_t *new_mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ALLOC_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ALLOC_MLUT);

    new_mlut = (MPIDI_rank_map_mlut_t *) MPL_malloc(sizeof(MPIDI_rank_map_mlut_t)
                                                    + size * sizeof(MPIDI_gpid_t), MPL_MEM_ADDRESS);
    if (new_mlut == NULL) {
        *mlut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_Object_set_ref(new_mlut, 1);
    *mlut = new_mlut;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                    (MPL_DBG_FDEST, "alloc mlut %p, size %lu, refcount=%d",
                     new_mlut, size * sizeof(MPIDI_gpid_t), MPIR_Object_get_ref(new_mlut)));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ALLOC_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_release_mlut(MPIDI_rank_map_mlut_t * mlut)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_RELEASE_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_RELEASE_MLUT);

    MPIR_Object_release_ref(mlut, &in_use);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "dec ref to mlut %p", mlut));
    if (!in_use) {
        MPL_free(mlut);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "free mlut %p", mlut));
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_RELEASE_MLUT);
    return mpi_errno;
}
