/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_PROC_H_INCLUDED
#define CH4_PROC_H_INCLUDED

#include "ch4_types.h"

/* There are 3 terms referencing processes:
 * upid, or "universal process id", is netmod layer address (addrname)
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
int MPIDIU_avt_init(void);
int MPIDIU_avt_finalize(void);
int MPIDIU_get_node_id(MPIR_Comm * comm, int rank, int *id_p);
int MPIDIU_bc_exchange_node_roots(MPIR_Comm * comm, const char *addrname, int addrlen,
                                  void *roots_names);

#ifdef MPIDI_BUILD_CH4_UPID_HASH
void MPIDIU_upidhash_add(const void *upid, int upid_len, MPIR_Lpid lpid);
MPIDI_upid_hash *MPIDIU_upidhash_find(const void *upid, int upid_len);
void MPIDIU_upidhash_free(void);
#endif

MPL_STATIC_INLINE_PREFIX MPIDI_av_entry_t *MPIDIU_comm_rank_to_av(MPIR_Comm * comm, int rank)
{
    MPIDI_av_entry_t *ret = NULL;
    MPIR_FUNC_ENTER;

    MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(comm, rank);
    int world_idx = MPIR_LPID_WORLD_INDEX(lpid);
    int world_rank = MPIR_LPID_WORLD_RANK(lpid);

    ret = &MPIDI_global.avt_mgr.av_tables[world_idx]->table[world_rank];

    MPIR_FUNC_EXIT;
    return ret;
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

MPL_STATIC_INLINE_PREFIX int MPIDIU_get_grank(int rank, MPIR_Comm * comm)
{
    MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(comm, rank);
    if (MPIR_LPID_WORLD_INDEX(lpid) == 0) {
        return (int) lpid;
    } else {
        return -1;
    }
}

/* used in fast path where we know the lpid has a valid av, such as from a committed communicator */
MPL_STATIC_INLINE_PREFIX MPIDI_av_entry_t *MPIDIU_lpid_to_av(MPIR_Lpid lpid)
{
    int world_idx = MPIR_LPID_WORLD_INDEX(lpid);
    int world_rank = MPIR_LPID_WORLD_RANK(lpid);
    return &MPIDI_global.avt_mgr.av_tables[world_idx]->table[world_rank];
}

MPL_STATIC_INLINE_PREFIX int MPIDI_rank_is_local(int rank, MPIR_Comm * comm)
{
    int ret;
    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_DIRECT_NETMOD
    /* Ask the netmod for locality information. If it decided not to build it,
     * it will call back up to the MPIDIU function to get the information. */
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

int MPIDIU_insert_dynamic_upid(MPIR_Lpid * lpid_out, const char *upid, int upid_len);
int MPIDIU_free_dynamic_lpid(MPIR_Lpid lpid);
MPIDI_av_entry_t *MPIDIU_find_dynamic_av(const char *upid, int upid_len);
/* used in communicator creation paths when the av entry may not exist or inserted yet */
MPIDI_av_entry_t *MPIDIU_lpid_to_av_slow(MPIR_Lpid lpid);

#endif /* CH4_PROC_H_INCLUDED */
