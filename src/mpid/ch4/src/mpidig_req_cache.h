/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_REQ_CACHE_H_INCLUDED
#define MPIDIG_REQ_CACHE_H_INCLUDED

/* the request cache is for saving the mapping between sreq and rreq for recv request that has more
 * data coming. The implementation uses a hash table that maps (sreq, key2) -> (rreq). We only
 * use sreq as key for the map because the request address should be random and less likely to have
 * too process using sreq with the exact equal address. The key2 is added to handle that rare
 * case */
static inline MPIR_Request *MPIDIG_req_cache_lookup(void *req_map, uint64_t key)
{
    void *ret = MPIDIU_map_lookup(req_map, key);
    if (ret != MPIDIU_MAP_NOT_FOUND) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "req_map found: key=0x%" PRIx64 ", value=0x%p", key, ret));
        return (MPIR_Request *) ret;
    }
    return NULL;
}

static inline void MPIDIG_req_cache_add(void *req_map, uint64_t key, MPIR_Request * rreq)
{
    void *ret = MPIDIU_map_lookup(req_map, key);

    if (likely(ret == MPIDIU_MAP_NOT_FOUND)) {
        MPIDIU_map_set_unsafe(req_map, key, (void *) rreq, MPL_MEM_OTHER);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "req_map new: key=0x%" PRIx64 " value=%p", key, rreq));
    } else {
        MPIR_Assert(0);
    }
}

static inline void MPIDIG_req_cache_remove(void *req_map, uint64_t key)
{
    void *ret = MPIDIU_map_lookup(req_map, key);
    if (ret != MPIDIU_MAP_NOT_FOUND) {
        MPIDIU_map_erase(req_map, key);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "req_map remove: key=0x%" PRIx64 ", value=0x%p", key, ret));
    }
}

#endif /* MPIDIG_REQ_CACHE_H_INCLUDED */
