/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4R_PROC_H_INCLUDED
#define CH4R_PROC_H_INCLUDED

#include <sys/mman.h>

#include "ch4_types.h"
#include "build_nodemap.h"

#undef FUNCNAME
#define FUNCNAME MPIDIU_comm_rank_to_pid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIU_comm_rank_to_pid(MPIR_Comm * comm, int rank, int *index,
                                                     int *avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_COMM_RANK_TO_PID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_COMM_RANK_TO_PID);

    *avtid = 0;

    switch (MPIDI_COMM(comm, map).mode) {
    case MPIDI_RANK_MAP_DIRECT:
        *avtid = MPIDI_COMM(comm, map).avtid;
        *index = rank;
        break;
    case MPIDI_RANK_MAP_DIRECT_INTRA:
        *index = rank;
        break;
    case MPIDI_RANK_MAP_OFFSET:
        *avtid = MPIDI_COMM(comm, map).avtid;
        *index = rank + MPIDI_COMM(comm, map).reg.offset;
        break;
    case MPIDI_RANK_MAP_OFFSET_INTRA:
        *index = rank + MPIDI_COMM(comm, map).reg.offset;
        break;
    case MPIDI_RANK_MAP_STRIDE:
        *avtid = MPIDI_COMM(comm, map).avtid;
        *index = MPIDI_CALC_STRIDE_SIMPLE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                          MPIDI_COMM(comm, map).reg.stride.offset);
        break;
    case MPIDI_RANK_MAP_STRIDE_INTRA:
        *index = MPIDI_CALC_STRIDE_SIMPLE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                          MPIDI_COMM(comm, map).reg.stride.offset);
        break;
    case MPIDI_RANK_MAP_STRIDE_BLOCK:
        *avtid = MPIDI_COMM(comm, map).avtid;
        *index = MPIDI_CALC_STRIDE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                   MPIDI_COMM(comm, map).reg.stride.blocksize,
                                   MPIDI_COMM(comm, map).reg.stride.offset);
        break;
    case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
        *index = MPIDI_CALC_STRIDE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                   MPIDI_COMM(comm, map).reg.stride.blocksize,
                                   MPIDI_COMM(comm, map).reg.stride.offset);
        break;
    case MPIDI_RANK_MAP_LUT:
        *avtid = MPIDI_COMM(comm, map).avtid;
        *index = MPIDI_COMM(comm, map).irreg.lut.lpid[rank];
        break;
    case MPIDI_RANK_MAP_LUT_INTRA:
        *index = MPIDI_COMM(comm, map).irreg.lut.lpid[rank];
        break;
    case MPIDI_RANK_MAP_MLUT:
        *index = MPIDI_COMM(comm, map).irreg.mlut.gpid[rank].lpid;
        *avtid = MPIDI_COMM(comm, map).irreg.mlut.gpid[rank].avtid;
        break;
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " comm_to_pid: rank=%d, avtid=%d index=%d",
                     rank, *avtid, *index));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_COMM_RANK_TO_PID);
    return *index;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_comm_rank_to_av
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIDI_av_entry_t *MPIDIU_comm_rank_to_av(MPIR_Comm * comm, int rank)
{
    MPIDI_av_entry_t *ret = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_COMM_RANK_TO_AV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_COMM_RANK_TO_AV);

    switch (MPIDI_COMM(comm, map).mode) {
    case MPIDI_RANK_MAP_DIRECT:
        ret = &MPIDI_av_table[MPIDI_COMM(comm, map).avtid]->table[rank];
        break;
    case MPIDI_RANK_MAP_DIRECT_INTRA:
        ret = &MPIDI_av_table0->table[rank];
        break;
    case MPIDI_RANK_MAP_OFFSET:
        ret = &MPIDI_av_table[MPIDI_COMM(comm, map).avtid]
            ->table[rank + MPIDI_COMM(comm, map).reg.offset];
        break;
    case MPIDI_RANK_MAP_OFFSET_INTRA:
        ret = &MPIDI_av_table0->table[rank + MPIDI_COMM(comm, map).reg.offset];
        break;
    case MPIDI_RANK_MAP_STRIDE:
        ret = &MPIDI_av_table[MPIDI_COMM(comm, map).avtid]
            ->table[MPIDI_CALC_STRIDE_SIMPLE(rank,
                                             MPIDI_COMM(comm, map).reg.stride.stride,
                                             MPIDI_COMM(comm, map).reg.stride.offset)];
        break;
    case MPIDI_RANK_MAP_STRIDE_INTRA:
        ret = &MPIDI_av_table0->table[MPIDI_CALC_STRIDE_SIMPLE(rank,
                                                               MPIDI_COMM(comm,
                                                                          map).reg.stride.stride,
                                                               MPIDI_COMM(comm,
                                                                          map).reg.stride.offset)];
        break;
    case MPIDI_RANK_MAP_STRIDE_BLOCK:
        ret = &MPIDI_av_table[MPIDI_COMM(comm, map).avtid]
            ->table[MPIDI_CALC_STRIDE(rank,
                                      MPIDI_COMM(comm, map).reg.stride.stride,
                                      MPIDI_COMM(comm, map).reg.stride.blocksize,
                                      MPIDI_COMM(comm, map).reg.stride.offset)];
        break;
    case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
        ret = &MPIDI_av_table0->table[MPIDI_CALC_STRIDE(rank,
                                                        MPIDI_COMM(comm, map).reg.stride.stride,
                                                        MPIDI_COMM(comm,
                                                                   map).reg.stride.blocksize,
                                                        MPIDI_COMM(comm, map).reg.stride.offset)];
        break;
    case MPIDI_RANK_MAP_LUT:
        ret = &MPIDI_av_table[MPIDI_COMM(comm, map).avtid]
            ->table[MPIDI_COMM(comm, map).irreg.lut.lpid[rank]];
        break;
    case MPIDI_RANK_MAP_LUT_INTRA:
        ret = &MPIDI_av_table0->table[MPIDI_COMM(comm, map).irreg.lut.lpid[rank]];
        break;
    case MPIDI_RANK_MAP_MLUT:
        ret = &MPIDI_av_table[MPIDI_COMM(comm, map).irreg.mlut.gpid[rank].avtid]
            ->table[MPIDI_COMM(comm, map).irreg.mlut.gpid[rank].lpid];
        break;
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " comm_to_av_addr: rank=%d, av addr=%p",
                     rank, (void*)ret));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_COMM_RANK_TO_AV);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_comm_rank_to_pid_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIU_comm_rank_to_pid_local(MPIR_Comm * comm, int rank, int *index,
                                                           int *avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_COMM_RANK_TO_PID_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_COMM_RANK_TO_PID_LOCAL);

    *avtid = MPIDI_COMM(comm, local_map).avtid;
    switch (MPIDI_COMM(comm, local_map).mode) {
    case MPIDI_RANK_MAP_DIRECT:
    case MPIDI_RANK_MAP_DIRECT_INTRA:
        *index = rank;
        break;
    case MPIDI_RANK_MAP_OFFSET:
    case MPIDI_RANK_MAP_OFFSET_INTRA:
        *index = rank + MPIDI_COMM(comm, local_map).reg.offset;
        break;
    case MPIDI_RANK_MAP_STRIDE:
    case MPIDI_RANK_MAP_STRIDE_INTRA:
        *index = MPIDI_CALC_STRIDE_SIMPLE(rank, MPIDI_COMM(comm, local_map).reg.stride.stride,
                                          MPIDI_COMM(comm, local_map).reg.stride.offset);
        break;
    case MPIDI_RANK_MAP_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
        *index = MPIDI_CALC_STRIDE(rank, MPIDI_COMM(comm, local_map).reg.stride.stride,
                                   MPIDI_COMM(comm, local_map).reg.stride.blocksize,
                                   MPIDI_COMM(comm, local_map).reg.stride.offset);
        break;
    case MPIDI_RANK_MAP_LUT:
    case MPIDI_RANK_MAP_LUT_INTRA:
        *index = MPIDI_COMM(comm, local_map).irreg.lut.lpid[rank];
        break;
    case MPIDI_RANK_MAP_MLUT:
        *index = MPIDI_COMM(comm, local_map).irreg.mlut.gpid[rank].lpid;
        *avtid = MPIDI_COMM(comm, local_map).irreg.mlut.gpid[rank].avtid;
        break;
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " comm_to_pid_local: rank=%d, avtid=%d index=%d",
                     rank, *avtid, *index));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_COMM_RANK_TO_PID_LOCAL);
    return *index;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_rank_is_local(int rank, MPIR_Comm * comm)
{
    int ret = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_RANK_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_RANK_IS_LOCAL);

#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    ret = MPIDIU_comm_rank_to_av(comm, rank)->is_local;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " is_local=%d, rank=%d", ret, rank));
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_RANK_IS_LOCAL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_av_is_local(MPIDI_av_entry_t *av)
{
    int ret = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_AV_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_AV_IS_LOCAL);

#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    ret = av->is_local;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " is_local=%d, av=%p", ret, (void*)av));
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_AV_IS_LOCAL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_rank_to_lpid(int rank, MPIR_Comm * comm)
{
    int ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_RANK_TO_LPID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_RANK_TO_LPID);

    int avtid = 0, lpid = 0;
    MPIDIU_comm_rank_to_pid(comm, rank, &lpid, &avtid);
    if (avtid == 0) {
        ret = lpid;
    }
    else {
        ret = -1;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_RANK_TO_LPID);
    return ret;
}

static inline int MPIDI_CH4U_get_node_id(MPIR_Comm * comm, int rank, MPID_Node_id_t * id_p)
{
    int mpi_errno = MPI_SUCCESS;
    int avtid = 0, lpid = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_GET_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_GET_NODE_ID);

    MPIDIU_comm_rank_to_pid(comm, rank, &lpid, &avtid);
    *id_p = MPIDI_CH4_Global.node_map[avtid][lpid];

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_GET_NODE_ID);
    return mpi_errno;
}

static inline int MPIDI_CH4U_get_max_node_id(MPIR_Comm * comm, MPID_Node_id_t * max_id_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_GET_MAX_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_GET_MAX_NODE_ID);

    *max_id_p = MPIDI_CH4_Global.max_node_id;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_GET_MAX_NODE_ID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_build_nodemap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_build_nodemap(int myrank,
                                           MPIR_Comm * comm,
                                           int sz,
                                           MPID_Node_id_t * out_nodemap, MPID_Node_id_t * sz_out)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_BUILD_NODEMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_BUILD_NODEMAP);

    ret = MPIR_NODEMAP_build_nodemap(sz, myrank, out_nodemap, sz_out);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_BUILD_NODEMAP);
    return ret;
}

static inline int MPIDIU_get_n_avts()
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_N_AVTS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_N_AVTS);

    ret = MPIDI_CH4_Global.avt_mgr.n_avts;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_N_AVTS);
    return ret;
}

static inline int MPIDIU_get_max_n_avts()
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_MAX_N_AVTS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_MAX_N_AVTS);

    ret = MPIDI_CH4_Global.avt_mgr.max_n_avts;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_MAX_N_AVTS);
    return ret;
}

static inline int MPIDIU_get_avt_size(int avtid)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_AVT_SIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_AVT_SIZE);

    ret = MPIDI_av_table[avtid]->size;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_AVT_SIZE);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_alloc_globals_for_avtid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_alloc_globals_for_avtid(int avtid)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Node_id_t *new_node_map = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ALLOC_GLOBALS_FOR_AVTID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ALLOC_GLOBALS_FOR_AVTID);

    new_node_map =
        (MPID_Node_id_t *) MPL_malloc(MPIDI_av_table[avtid]->size * sizeof(MPID_Node_id_t));
    MPIR_ERR_CHKANDJUMP(new_node_map == NULL, mpi_errno, MPI_ERR_NO_MEM, "**nomem");
    MPIDI_CH4_Global.node_map[avtid] = new_node_map;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ALLOC_GLOBALS_FOR_AVTID);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_free_globals_for_avtid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_free_globals_for_avtid(int avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_FREE_GLOBALS_FOR_AVTID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_FREE_GLOBALS_FOR_AVTID);
    MPL_free(MPIDI_CH4_Global.node_map[avtid]);
    MPIDI_CH4_Global.node_map[avtid] = NULL;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_FREE_GLOBALS_FOR_AVTID);
    return MPI_SUCCESS;
}

static inline int MPIDIU_get_next_avtid(int *avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_GET_NEXT_AVTID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_GET_NEXT_AVTID);

    if (MPIDI_CH4_Global.avt_mgr.next_avtid == -1) {    /* out of free avtids */
        int old_max, new_max, i;
        old_max = MPIDI_CH4_Global.avt_mgr.max_n_avts;
        new_max = old_max + 1;
        MPIDI_CH4_Global.avt_mgr.free_avtid =
            (int *) MPL_realloc(MPIDI_CH4_Global.avt_mgr.free_avtid, new_max * sizeof(int));
        for (i = old_max; i < new_max - 1; i++) {
            MPIDI_CH4_Global.avt_mgr.free_avtid[i] = i + 1;
        }
        MPIDI_CH4_Global.avt_mgr.free_avtid[new_max - 1] = -1;
        MPIDI_CH4_Global.avt_mgr.max_n_avts = new_max;
        MPIDI_CH4_Global.avt_mgr.next_avtid = old_max;
    }

    *avtid = MPIDI_CH4_Global.avt_mgr.next_avtid;
    MPIDI_CH4_Global.avt_mgr.next_avtid = MPIDI_CH4_Global.avt_mgr.free_avtid[*avtid];
    MPIDI_CH4_Global.avt_mgr.free_avtid[*avtid] = -1;
    MPIDI_CH4_Global.avt_mgr.n_avts++;
    MPIR_Assert(MPIDI_CH4_Global.avt_mgr.n_avts <= MPIDI_CH4_Global.avt_mgr.max_n_avts);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " new_avtid=%d", *avtid));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_GET_NEXT_AVTID);
    return *avtid;
}

static inline int MPIDIU_free_avtid(int avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_FREE_AVTID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_FREE_AVTID);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " free_avtid=%d", avtid));
    MPIR_Assert(MPIDI_CH4_Global.avt_mgr.n_avts > 0);
    MPIDI_CH4_Global.avt_mgr.free_avtid[avtid] = MPIDI_CH4_Global.avt_mgr.next_avtid;
    MPIDI_CH4_Global.avt_mgr.next_avtid = avtid;
    MPIDI_CH4_Global.avt_mgr.n_avts--;
    /*
     * TODO:
     * If the allowed number of process groups are very large
     * We need to return unsed pages back to OS.
     * madvise(addr, len, MADV_DONTNEED);
     * We need tracking wich page can be returned
     */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_FREE_AVTID);
    return 0;
}

static inline int MPIDIU_new_avt(int size, int *avtid)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_table_t *new_av_table;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_NEW_AVT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_NEW_AVT);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " new_avt: size=%d", size));

    MPIDIU_get_next_avtid(avtid);

    new_av_table = (MPIDI_av_table_t *) MPL_malloc(size * sizeof(MPIDI_av_entry_t)
                                                   + sizeof(MPIDI_av_table_t));
    new_av_table->size = size;
    MPIDI_av_table[*avtid] = new_av_table;

    MPIR_Object_set_ref(MPIDI_av_table[*avtid], 0);

    MPIDIU_alloc_globals_for_avtid(*avtid);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_NEW_AVT);
    return mpi_errno;
}

static inline int MPIDIU_free_avt(int avtid)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_FREE_AVT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_FREE_AVT);

    MPIDIU_free_globals_for_avtid(avtid);
    MPL_free(MPIDI_av_table[avtid]);
    MPIDI_av_table[avtid] = NULL;
    MPIDIU_free_avtid(avtid);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_FREE_AVT);
    return mpi_errno;
}

static inline int MPIDIU_avt_add_ref(int avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_AVT_ADD_REF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_AVT_ADD_REF);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " incr avtid=%d", avtid));
    MPIR_Object_add_ref(MPIDI_av_table[avtid]);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_AVT_ADD_REF);
    return MPI_SUCCESS;
}

static inline int MPIDIU_avt_release_ref(int avtid)
{
    int count;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_AVT_RELEASE_REF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_AVT_RELEASE_REF);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " decr avtid=%d", avtid));
    MPIR_Object_release_ref(MPIDIU_get_av_table(avtid), &count);
    if (count == 0) {
        MPIDIU_free_avt(avtid);
        MPIDIU_free_globals_for_avtid(avtid);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_AVT_RELEASE_REF);
    return MPI_SUCCESS;
}

static inline int MPIDIU_avt_init()
{
    int i, mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_AVT_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_AVT_INIT);

    MPIDI_CH4_Global.avt_mgr.mmapped_size = 8 * 4 * 1024;
    MPIDI_CH4_Global.avt_mgr.max_n_avts = 1;
    MPIDI_CH4_Global.avt_mgr.next_avtid = 0;
    MPIDI_CH4_Global.avt_mgr.n_avts = 0;

    MPIDI_av_table = (MPIDI_av_table_t **)
                      mmap(NULL, MPIDI_CH4_Global.avt_mgr.mmapped_size,
                           PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    MPIR_ERR_CHKANDSTMT(MPIDI_av_table == MAP_FAILED, mpi_errno, MPI_ERR_NO_MEM,
                        goto fn_fail, "**nomem");

    MPIDI_CH4_Global.node_map = (MPID_Node_id_t **)
                                mmap(NULL, MPIDI_CH4_Global.avt_mgr.mmapped_size,
                                     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    MPIR_ERR_CHKANDSTMT(MPIDI_CH4_Global.node_map == MAP_FAILED, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    MPIDI_CH4_Global.avt_mgr.free_avtid =
        (int *) MPL_malloc(MPIDI_CH4_Global.avt_mgr.max_n_avts * sizeof(int));
    MPIR_ERR_CHKANDSTMT(MPIDI_CH4_Global.avt_mgr.free_avtid == NULL, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    for (i = 0; i < MPIDI_CH4_Global.avt_mgr.max_n_avts - 1; i++) {
        MPIDI_CH4_Global.avt_mgr.free_avtid[i] = i + 1;
    }
    MPIDI_CH4_Global.avt_mgr.free_avtid[MPIDI_CH4_Global.avt_mgr.max_n_avts - 1] = -1;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_AVT_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDIU_avt_destroy()
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_AVT_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_AVT_DESTROY);

    munmap((void *)MPIDI_CH4_Global.node_map, MPIDI_CH4_Global.avt_mgr.mmapped_size);
    munmap((void *)MPIDI_av_table, MPIDI_CH4_Global.avt_mgr.mmapped_size);
    MPL_free(MPIDI_CH4_Global.avt_mgr.free_avtid);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_AVT_DESTROY);
    return MPI_SUCCESS;
}

static inline int MPIDIU_build_nodemap_avtid(int myrank, MPIR_Comm * comm, int sz, int avtid)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_BUILD_NODEMAP_AVTID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_BUILD_NODEMAP_AVTID);
    ret = MPIDI_CH4U_build_nodemap(myrank, comm, sz,
                                   MPIDI_CH4_Global.node_map[avtid], &MPIDI_CH4_Global.max_node_id);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_BUILD_NODEMAP_AVTID);
    return ret;
}

#endif /* CH4R_PROC_H_INCLUDED */
