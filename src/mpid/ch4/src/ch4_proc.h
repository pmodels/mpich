/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_PROC_H_INCLUDED
#define CH4_PROC_H_INCLUDED

#include "ch4_types.h"

/* There are 3 terms referencing processes:
 * upid, or "unversal process id", is netmod layer address (addrname)
 * lpid, or "local process id", is av entry index in an ch4-layer table
 * gpid, or "global process id", is av table index plus av entry index
 *
 * For non-dynamic processes, av table index is 0, thus lpid equals to gpid (other than type).
 * The upid is used when establishing netmod connections.
 * The lpid and gpid are defined here with avt manager.
 */

int MPIDIU_get_n_avts(void);
int MPIDIU_get_avt_size(int avtid);
int MPIDIU_new_avt(int size, int *avtid);
int MPIDIU_free_avt(int avtid);
int MPIDIU_avt_add_ref(int avtid);
int MPIDIU_avt_release_ref(int avtid);
int MPIDIU_avt_init(void);
int MPIDIU_avt_destroy(void);
int MPIDIU_get_node_id(MPIR_Comm * comm, int rank, int *id_p);

#ifdef MPIDI_BUILD_CH4_UPID_HASH
void MPIDIU_upidhash_add(const void *upid, int upid_len, int avtid, int lpid);
MPIDI_upid_hash *MPIDIU_upidhash_find(const void *upid, int upid_len);
void MPIDIU_upidhash_free(void);
#endif
int MPIDIU_upids_to_lpids(int size, int *remote_upid_size, char *remote_upids,
                          MPIR_Lpid * remote_lpids);
int MPIDIU_alloc_lut(MPIDI_rank_map_lut_t ** lut, int size);
int MPIDIU_release_lut(MPIDI_rank_map_lut_t * lut);
int MPIDIU_alloc_mlut(MPIDI_rank_map_mlut_t ** mlut, int size);
int MPIDIU_release_mlut(MPIDI_rank_map_mlut_t * mlut);
#define MPIDIU_lut_add_ref(lut) \
    do { \
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "inc ref to lut %p", lut)); \
        MPIR_cc_inc(&(lut)->ref_count); \
    } while (0)

#define MPIDIU_mlut_add_ref(mlut) \
    do { \
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "inc ref to mlut %p", mlut)); \
        MPIR_cc_inc(&(mlut)->ref_count); \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDIU_comm_rank_to_pid(MPIR_Comm * comm, int rank, int *idx,
                                                     int *avtid)
{
    MPIR_FUNC_ENTER;

    *avtid = 0;
    *idx = 0;

    switch (MPIDI_COMM(comm, map).mode) {
        case MPIDI_RANK_MAP_DIRECT:
            *avtid = MPIDI_COMM(comm, map).avtid;
            *idx = rank;
            break;
        case MPIDI_RANK_MAP_DIRECT_INTRA:
            *idx = rank;
            break;
        case MPIDI_RANK_MAP_OFFSET:
            *avtid = MPIDI_COMM(comm, map).avtid;
            *idx = rank + MPIDI_COMM(comm, map).reg.offset;
            break;
        case MPIDI_RANK_MAP_OFFSET_INTRA:
            *idx = rank + MPIDI_COMM(comm, map).reg.offset;
            break;
        case MPIDI_RANK_MAP_STRIDE:
            *avtid = MPIDI_COMM(comm, map).avtid;
            *idx = MPIDI_CALC_STRIDE_SIMPLE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                            MPIDI_COMM(comm, map).reg.stride.offset);
            break;
        case MPIDI_RANK_MAP_STRIDE_INTRA:
            *idx = MPIDI_CALC_STRIDE_SIMPLE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                            MPIDI_COMM(comm, map).reg.stride.offset);
            break;
        case MPIDI_RANK_MAP_STRIDE_BLOCK:
            *avtid = MPIDI_COMM(comm, map).avtid;
            *idx = MPIDI_CALC_STRIDE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                     MPIDI_COMM(comm, map).reg.stride.blocksize,
                                     MPIDI_COMM(comm, map).reg.stride.offset);
            break;
        case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
            *idx = MPIDI_CALC_STRIDE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                     MPIDI_COMM(comm, map).reg.stride.blocksize,
                                     MPIDI_COMM(comm, map).reg.stride.offset);
            break;
        case MPIDI_RANK_MAP_LUT:
            *avtid = MPIDI_COMM(comm, map).avtid;
            *idx = MPIDI_COMM(comm, map).irreg.lut.lpid[rank];
            break;
        case MPIDI_RANK_MAP_LUT_INTRA:
            *idx = MPIDI_COMM(comm, map).irreg.lut.lpid[rank];
            break;
        case MPIDI_RANK_MAP_MLUT:
            *idx = MPIDI_COMM(comm, map).irreg.mlut.gpid[rank].lpid;
            *avtid = MPIDI_COMM(comm, map).irreg.mlut.gpid[rank].avtid;
            break;
        case MPIDI_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " comm_to_pid: rank=%d, avtid=%d idx=%d", rank, *avtid, *idx));
    MPIR_FUNC_EXIT;
    return *idx;
}

MPL_STATIC_INLINE_PREFIX MPIDI_av_entry_t *MPIDIU_comm_rank_to_av(MPIR_Comm * comm, int rank)
{
    MPIDI_av_entry_t *ret = NULL;
    MPIR_FUNC_ENTER;

    int lpid;
    switch (MPIDI_COMM(comm, map).mode) {
        case MPIDI_RANK_MAP_DIRECT:
            ret = &MPIDI_global.avt_mgr.av_tables[MPIDI_COMM(comm, map).avtid]->table[rank];
            break;
        case MPIDI_RANK_MAP_DIRECT_INTRA:
            ret = &MPIDI_global.avt_mgr.av_table0->table[rank];
            break;
        case MPIDI_RANK_MAP_OFFSET:
            ret = &MPIDI_global.avt_mgr.av_tables[MPIDI_COMM(comm, map).avtid]
                ->table[rank + MPIDI_COMM(comm, map).reg.offset];
            break;
        case MPIDI_RANK_MAP_OFFSET_INTRA:
            ret = &MPIDI_global.avt_mgr.av_table0->table[rank + MPIDI_COMM(comm, map).reg.offset];
            break;
        case MPIDI_RANK_MAP_STRIDE:
            lpid = MPIDI_CALC_STRIDE_SIMPLE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                            MPIDI_COMM(comm, map).reg.stride.offset);
            ret = &MPIDI_global.avt_mgr.av_tables[MPIDI_COMM(comm, map).avtid]->table[lpid];
            break;
        case MPIDI_RANK_MAP_STRIDE_INTRA:
            lpid = MPIDI_CALC_STRIDE_SIMPLE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                            MPIDI_COMM(comm, map).reg.stride.offset);
            ret = &MPIDI_global.avt_mgr.av_table0->table[lpid];
            break;
        case MPIDI_RANK_MAP_STRIDE_BLOCK:
            lpid = MPIDI_CALC_STRIDE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                     MPIDI_COMM(comm, map).reg.stride.blocksize,
                                     MPIDI_COMM(comm, map).reg.stride.offset);
            ret = &MPIDI_global.avt_mgr.av_tables[MPIDI_COMM(comm, map).avtid]->table[lpid];
            break;
        case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
            lpid = MPIDI_CALC_STRIDE(rank, MPIDI_COMM(comm, map).reg.stride.stride,
                                     MPIDI_COMM(comm, map).reg.stride.blocksize,
                                     MPIDI_COMM(comm, map).reg.stride.offset);
            ret = &MPIDI_global.avt_mgr.av_table0->table[lpid];
            break;
        case MPIDI_RANK_MAP_LUT:
            ret = &MPIDI_global.avt_mgr.av_tables[MPIDI_COMM(comm, map).avtid]
                ->table[MPIDI_COMM(comm, map).irreg.lut.lpid[rank]];
            break;
        case MPIDI_RANK_MAP_LUT_INTRA:
            ret =
                &MPIDI_global.avt_mgr.av_table0->table[MPIDI_COMM(comm, map).irreg.lut.lpid[rank]];
            break;
        case MPIDI_RANK_MAP_MLUT:
            ret = &MPIDI_global.avt_mgr.av_tables[MPIDI_COMM(comm, map).irreg.mlut.gpid[rank].avtid]
                ->table[MPIDI_COMM(comm, map).irreg.mlut.gpid[rank].lpid];
            break;
        case MPIDI_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " comm_to_av_addr: rank=%d, av addr=%p", rank, (void *) ret));
    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDIU_comm_rank_to_pid_local(MPIR_Comm * comm, int rank, int *idx,
                                                           int *avtid)
{
    MPIR_FUNC_ENTER;

    *avtid = MPIDI_COMM(comm, local_map).avtid;
    switch (MPIDI_COMM(comm, local_map).mode) {
        case MPIDI_RANK_MAP_DIRECT:
        case MPIDI_RANK_MAP_DIRECT_INTRA:
            *idx = rank;
            break;
        case MPIDI_RANK_MAP_OFFSET:
        case MPIDI_RANK_MAP_OFFSET_INTRA:
            *idx = rank + MPIDI_COMM(comm, local_map).reg.offset;
            break;
        case MPIDI_RANK_MAP_STRIDE:
        case MPIDI_RANK_MAP_STRIDE_INTRA:
            *idx = MPIDI_CALC_STRIDE_SIMPLE(rank, MPIDI_COMM(comm, local_map).reg.stride.stride,
                                            MPIDI_COMM(comm, local_map).reg.stride.offset);
            break;
        case MPIDI_RANK_MAP_STRIDE_BLOCK:
        case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
            *idx = MPIDI_CALC_STRIDE(rank, MPIDI_COMM(comm, local_map).reg.stride.stride,
                                     MPIDI_COMM(comm, local_map).reg.stride.blocksize,
                                     MPIDI_COMM(comm, local_map).reg.stride.offset);
            break;
        case MPIDI_RANK_MAP_LUT:
        case MPIDI_RANK_MAP_LUT_INTRA:
            *idx = MPIDI_COMM(comm, local_map).irreg.lut.lpid[rank];
            break;
        case MPIDI_RANK_MAP_MLUT:
            *idx = MPIDI_COMM(comm, local_map).irreg.mlut.gpid[rank].lpid;
            *avtid = MPIDI_COMM(comm, local_map).irreg.mlut.gpid[rank].avtid;
            break;
        case MPIDI_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " comm_to_pid_local: rank=%d, avtid=%d idx=%d",
                     rank, *avtid, *idx));
    MPIR_FUNC_EXIT;
    return *idx;
}

MPL_STATIC_INLINE_PREFIX int MPIDIU_av_is_local(MPIDI_av_entry_t * av)
{
    int ret = 0;
    MPIR_FUNC_ENTER;

    ret = av->is_local;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " is_local=%d, av=%p", ret, (void *) av));

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDIU_rank_to_lpid(int rank, MPIR_Comm * comm)
{
    int ret;
    MPIR_FUNC_ENTER;

    int avtid = 0, lpid = 0;
    MPIDIU_comm_rank_to_pid(comm, rank, &lpid, &avtid);
    if (avtid == 0) {
        ret = lpid;
    } else {
        ret = -1;
    }

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_rank_is_local(int rank, MPIR_Comm * comm)
{
    int ret;
    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_DIRECT_NETMOD
    /* Ask the netmod for locality information. If it decided not to build it,
     * it will call back up to the MPIDIU function to get the infomration. */
    ret = MPIDI_NM_rank_is_local(rank, comm);
#else
    ret = MPIDIU_av_is_local(MPIDIU_comm_rank_to_av(comm, rank));
#endif

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_av_is_local(MPIDI_av_entry_t * av)
{
    int ret;

    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_DIRECT_NETMOD
    ret = 0;
#else
    ret = MPIDIU_av_is_local(av);
#endif

    MPIR_FUNC_EXIT;
    return ret;
}

#endif /* CH4_PROC_H_INCLUDED */
