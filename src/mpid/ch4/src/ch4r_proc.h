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

#include "ch4_types.h"
#include "build_nodemap.h"

#undef FUNCNAME
#define FUNCNAME MPIDIU_comm_rank_to_pid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIU_comm_rank_to_pid(MPIR_Comm * comm, int rank, int *index, int *avtid)
{
    switch (MPIDII_COMM(comm, map).mode) {
    case MPIDII_RANK_MAP_DIRECT:
        *avtid = MPIDII_COMM(comm, map).avtid;
        *index = rank;
        break;
    case MPIDII_RANK_MAP_DIRECT_INTRA:
        *index = rank;
        break;
    case MPIDII_RANK_MAP_OFFSET:
        *avtid = MPIDII_COMM(comm, map).avtid;
        *index = rank + MPIDII_COMM(comm, map).reg.offset;
        break;
    case MPIDII_RANK_MAP_OFFSET_INTRA:
        *index = rank + MPIDII_COMM(comm, map).reg.offset;
        break;
    case MPIDII_RANK_MAP_STRIDE:
        *avtid = MPIDII_COMM(comm, map).avtid;
        *index = MPIDII_CALC_STRIDE_SIMPLE(rank, MPIDII_COMM(comm, map).reg.stride.stride,
                                           MPIDII_COMM(comm, map).reg.stride.offset);
        break;
    case MPIDII_RANK_MAP_STRIDE_INTRA:
        *index = MPIDII_CALC_STRIDE_SIMPLE(rank, MPIDII_COMM(comm, map).reg.stride.stride,
                                           MPIDII_COMM(comm, map).reg.stride.offset);
        break;
    case MPIDII_RANK_MAP_STRIDE_BLOCK:
        *avtid = MPIDII_COMM(comm, map).avtid;
        *index = MPIDII_CALC_STRIDE(rank, MPIDII_COMM(comm, map).reg.stride.stride,
                                    MPIDII_COMM(comm, map).reg.stride.blocksize,
                                    MPIDII_COMM(comm, map).reg.stride.offset);
        break;
    case MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA:
        *index = MPIDII_CALC_STRIDE(rank, MPIDII_COMM(comm, map).reg.stride.stride,
                                    MPIDII_COMM(comm, map).reg.stride.blocksize,
                                    MPIDII_COMM(comm, map).reg.stride.offset);
        break;
    case MPIDII_RANK_MAP_LUT:
        *avtid = MPIDII_COMM(comm, map).avtid;
        *index = MPIDII_COMM(comm, map).irreg.lut.lpid[rank];
        break;
    case MPIDII_RANK_MAP_LUT_INTRA:
        *index = MPIDII_COMM(comm, map).irreg.lut.lpid[rank];
        break;
    case MPIDII_RANK_MAP_MLUT:
        *index = MPIDII_COMM(comm, map).irreg.mlut.gpid[rank].lpid;
        *avtid = MPIDII_COMM(comm, map).irreg.mlut.gpid[rank].avtid;
        break;
    case MPIDII_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " rank=%d, index=%d", rank, *index));
    return *index;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_comm_rank_to_av
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIDII_av_entry_t *MPIDIU_comm_rank_to_av(MPIR_Comm * comm, int rank)
{
    switch (MPIDII_COMM(comm, map).mode) {
    case MPIDII_RANK_MAP_DIRECT:
        return &MPIDII_av_table[MPIDII_COMM(comm, map).avtid]->table[rank];
    case MPIDII_RANK_MAP_DIRECT_INTRA:
        return &MPIDII_av_table0->table[rank];
    case MPIDII_RANK_MAP_OFFSET:
        return &MPIDII_av_table[MPIDII_COMM(comm, map).avtid]
            ->table[rank + MPIDII_COMM(comm, map).reg.offset];
    case MPIDII_RANK_MAP_OFFSET_INTRA:
        return &MPIDII_av_table0->table[rank + MPIDII_COMM(comm, map).reg.offset];
    case MPIDII_RANK_MAP_STRIDE:
        return &MPIDII_av_table[MPIDII_COMM(comm, map).avtid]
            ->table[MPIDII_CALC_STRIDE_SIMPLE(rank,
                                              MPIDII_COMM(comm, map).reg.stride.stride,
                                              MPIDII_COMM(comm, map).reg.stride.offset)];
    case MPIDII_RANK_MAP_STRIDE_INTRA:
        return &MPIDII_av_table0->table[MPIDII_CALC_STRIDE_SIMPLE(rank,
                                                                  MPIDII_COMM(comm,
                                                                              map).reg.stride.
                                                                  stride, MPIDII_COMM(comm,
                                                                                      map).reg.
                                                                  stride.offset)];
    case MPIDII_RANK_MAP_STRIDE_BLOCK:
        return &MPIDII_av_table[MPIDII_COMM(comm, map).avtid]
            ->table[MPIDII_CALC_STRIDE(rank,
                                       MPIDII_COMM(comm, map).reg.stride.stride,
                                       MPIDII_COMM(comm, map).reg.stride.blocksize,
                                       MPIDII_COMM(comm, map).reg.stride.offset)];
    case MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA:
        return &MPIDII_av_table0->table[MPIDII_CALC_STRIDE(rank,
                                                           MPIDII_COMM(comm, map).reg.stride.stride,
                                                           MPIDII_COMM(comm,
                                                                       map).reg.stride.blocksize,
                                                           MPIDII_COMM(comm,
                                                                       map).reg.stride.offset)];
    case MPIDII_RANK_MAP_LUT:
        return &MPIDII_av_table[MPIDII_COMM(comm, map).avtid]
            ->table[MPIDII_COMM(comm, map).irreg.lut.lpid[rank]];
    case MPIDII_RANK_MAP_LUT_INTRA:
        return &MPIDII_av_table0->table[MPIDII_COMM(comm, map).irreg.lut.lpid[rank]];
    case MPIDII_RANK_MAP_MLUT:
        return &MPIDII_av_table[MPIDII_COMM(comm, map).irreg.mlut.gpid[rank].avtid]
            ->table[MPIDII_COMM(comm, map).irreg.mlut.gpid[rank].lpid];
    case MPIDII_RANK_MAP_NONE:
        MPIR_Assert(0);
        return NULL;
    }
    return NULL;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_comm_rank_to_pid_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIU_comm_rank_to_pid_local(MPIR_Comm * comm, int rank, int *index, int *avtid)
{
    *avtid = MPIDII_COMM(comm, local_map).avtid;
    switch (MPIDII_COMM(comm, local_map).mode) {
    case MPIDII_RANK_MAP_DIRECT:
    case MPIDII_RANK_MAP_DIRECT_INTRA:
        *index = rank;
        break;
    case MPIDII_RANK_MAP_OFFSET:
    case MPIDII_RANK_MAP_OFFSET_INTRA:
        *index = rank + MPIDII_COMM(comm, local_map).reg.offset;
        break;
    case MPIDII_RANK_MAP_STRIDE:
    case MPIDII_RANK_MAP_STRIDE_INTRA:
        *index = MPIDII_CALC_STRIDE_SIMPLE(rank, MPIDII_COMM(comm, local_map).reg.stride.stride,
                                           MPIDII_COMM(comm, local_map).reg.stride.offset);
        break;
    case MPIDII_RANK_MAP_STRIDE_BLOCK:
    case MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA:
        *index = MPIDII_CALC_STRIDE(rank, MPIDII_COMM(comm, local_map).reg.stride.stride,
                                    MPIDII_COMM(comm, local_map).reg.stride.blocksize,
                                    MPIDII_COMM(comm, local_map).reg.stride.offset);
        break;
    case MPIDII_RANK_MAP_LUT:
    case MPIDII_RANK_MAP_LUT_INTRA:
        *index = MPIDII_COMM(comm, local_map).irreg.lut.lpid[rank];
        break;
    case MPIDII_RANK_MAP_MLUT:
        *index = MPIDII_COMM(comm, local_map).irreg.mlut.gpid[rank].lpid;
        *avtid = MPIDII_COMM(comm, local_map).irreg.mlut.gpid[rank].avtid;
        break;
    case MPIDII_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " rank: rank=%d, index=%d", rank, *index));
    return *index;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_rank_is_local(int rank, MPIR_Comm * comm)
{
    int ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_RANK_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_RANK_IS_LOCAL);

#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        ret = 0;
        goto fn_exit;
    }
    ret = MPIDIU_comm_rank_to_av(comm, rank)->is_local;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " is_local=%d, rank=%d", ret, rank));
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_RANK_IS_LOCAL);
    return ret;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_rank_to_lpid(int rank, MPIR_Comm * comm)
{
    int ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_RANK_TO_LPID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_RANK_TO_LPID);

    int avtid = 0, lpid = 0;
    MPIDIU_comm_rank_to_pid(comm, rank, &lpid, &avtid);
    if (avtid == 0) {
        ret = lpid;
    }
    else {
        ret = -1;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_RANK_TO_LPID);
    return ret;
}

static inline int MPIDI_CH4U_get_node_id(MPIR_Comm * comm, int rank, MPID_Node_id_t * id_p)
{
    int mpi_errno = MPI_SUCCESS;
    int avtid = 0, lpid = 0;
    MPIDIU_comm_rank_to_pid(comm, rank, &lpid, &avtid);
    *id_p = MPIDI_CH4_Global.node_map[avtid][lpid];

    return mpi_errno;
}

static inline int MPIDI_CH4U_get_max_node_id(MPIR_Comm * comm, MPID_Node_id_t * max_id_p)
{
    int mpi_errno = MPI_SUCCESS;
    *max_id_p = MPIDI_CH4_Global.max_node_id;

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
    return MPIR_NODEMAP_build_nodemap(sz, myrank, out_nodemap, sz_out);
}

static inline int MPIDIU_get_n_avts()
{
    return MPIDI_CH4_Global.avt_mgr.n_avts;
}

static inline int MPIDIU_get_max_n_avts()
{
    return MPIDI_CH4_Global.avt_mgr.max_n_avts;
}

static inline int MPIDIU_get_avt_size(int avtid)
{
    return MPIDII_av_table[avtid]->size;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_alloc_globals_for_avtid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_alloc_globals_for_avtid(int avtid)
{
    int max_n_avts;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATER_ALLOC_GLOBALS_FOR_AVTID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATER_ALLOC_GLOBALS_FOR_AVTID);
    max_n_avts = MPIDIU_get_max_n_avts();
    if (max_n_avts > MPIDI_CH4_Global.allocated_max_n_avts) {
        MPIDI_CH4_Global.node_map = (MPID_Node_id_t **) MPL_realloc(MPIDI_CH4_Global.node_map,
                                                                    max_n_avts *
                                                                    sizeof(MPID_Node_id_t *));
    }

    MPIDI_CH4_Global.node_map[avtid] =
        (MPID_Node_id_t *) MPL_malloc(MPIDII_av_table[avtid]->size * sizeof(MPID_Node_id_t));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATER_ALLOC_GLOBALS_FOR_AVTID);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_free_globals_for_avtid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_free_globals_for_avtid(int avtid)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATER_FREE_GLOBALS_FOR_AVTID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATER_FREE_GLOBALS_FOR_AVTID);
    MPL_free(MPIDI_CH4_Global.node_map[avtid]);
    MPIDI_CH4_Global.node_map[avtid] = NULL;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATER_FREE_GLOBALS_FOR_AVTID);
    return MPI_SUCCESS;
}

static inline int MPIDIU_get_next_avtid(int *avtid)
{
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
    return *avtid;
}

static inline int MPIDIU_free_avtid(int avtid)
{
    MPIR_Assert(MPIDI_CH4_Global.avt_mgr.n_avts > 0);
    MPIDI_CH4_Global.avt_mgr.free_avtid[avtid] = MPIDI_CH4_Global.avt_mgr.next_avtid;
    MPIDI_CH4_Global.avt_mgr.next_avtid = avtid;
    MPIDI_CH4_Global.avt_mgr.n_avts--;
    return 0;
}

static inline int MPIDIU_new_avt(int size, int *avtid)
{
    int mpi_errno = MPI_SUCCESS;
    int max_n_avts;
    MPIDII_av_table_t *new_av_table;
    MPIDIU_get_next_avtid(avtid);

    new_av_table = (MPIDII_av_table_t *) MPL_malloc(size * sizeof(MPIDII_av_entry_t)
                                                    + sizeof(MPIDII_av_table_t));
    max_n_avts = MPIDIU_get_max_n_avts();
    MPIDII_av_table = (MPIDII_av_table_t **) MPL_realloc(MPIDII_av_table,
                                                         max_n_avts * sizeof(MPIDII_av_table_t *));
    new_av_table->size = size;
    MPIDII_av_table[*avtid] = new_av_table;

    MPIR_Object_set_ref(MPIDII_av_table[*avtid], 0);

    MPIDIU_alloc_globals_for_avtid(*avtid);
    return mpi_errno;
}

static inline int MPIDIU_free_avt(int avtid)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIU_free_globals_for_avtid(avtid);
    MPL_free(MPIDII_av_table[avtid]);
    MPIDII_av_table[avtid] = NULL;
    MPIDIU_free_avtid(avtid);
    return mpi_errno;
}

static inline int MPIDIU_avt_add_ref(int avtid)
{
    MPIR_Object_add_ref(MPIDII_av_table[avtid]);
    return MPI_SUCCESS;
}

static inline int MPIDIU_avt_release_ref(int avtid)
{
    int count;
    MPIR_Object_release_ref(MPIDIU_get_av_table(avtid), &count);
    if (count == 0) {
        MPIDIU_free_avt(avtid);
        MPIDIU_free_globals_for_avtid(avtid);
    }
    return MPI_SUCCESS;
}

static inline int MPIDIU_avt_init()
{
    int i;
    MPIDI_CH4_Global.avt_mgr.max_n_avts = 1;
    MPIDI_CH4_Global.avt_mgr.next_avtid = 0;
    MPIDI_CH4_Global.avt_mgr.n_avts = 0;
    MPIDI_CH4_Global.avt_mgr.free_avtid =
        (int *) MPL_malloc(MPIDI_CH4_Global.avt_mgr.max_n_avts * sizeof(int));

    for (i = 0; i < MPIDI_CH4_Global.avt_mgr.max_n_avts - 1; i++) {
        MPIDI_CH4_Global.avt_mgr.free_avtid[i] = i + 1;
    }
    MPIDI_CH4_Global.avt_mgr.free_avtid[MPIDI_CH4_Global.avt_mgr.max_n_avts - 1] = -1;

    return MPI_SUCCESS;
}

static inline int MPIDIU_avt_destroy()
{
    MPL_free(MPIDI_CH4_Global.avt_mgr.free_avtid);
    return MPI_SUCCESS;
}

static inline int MPIDIU_build_nodemap_avtid(int myrank, MPIR_Comm * comm, int sz, int avtid)
{
    return MPIDI_CH4U_build_nodemap(myrank, comm, sz,
                                    MPIDI_CH4_Global.node_map[avtid],
                                    &MPIDI_CH4_Global.max_node_id);
}

#endif /* CH4R_PROC_H_INCLUDED */
