/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_LPID_H_INCLUDED
#define MPIR_LPID_H_INCLUDED

/* Worlds -
 * We need a device-independent way of identifying processes. Assuming the concept of
 * "worlds", we can describe a process with (world_idx, world_rank).
 *
 * The world_idx is a local id because each process may not see all worlds. Thus,
 * each process only can maintain a list of worlds as it encounters them. Thus,
 * a process id derived from (world_idx, world_rank) is referred as LPID, or
 * "local process id".
 *
 * Each process should maintain a table of worlds with sufficient information so
 * processes can match worlds upon connection or making address exchange.
 */

#define MPIR_NAMESPACE_MAX 128
struct MPIR_World {
    char namespace[MPIR_NAMESPACE_MAX];
    /* other useful fields */
    int num_procs;
};

extern struct MPIR_World MPIR_Worlds[];

int MPIR_add_world(const char *namespace, int num_procs);
int MPIR_find_world(const char *namespace);

/* Abstract the integer type for lpid (process id). It is possible to use 32-bit
 * in principle, but 64-bit is simpler since we can trivially combine
 * (world_idx, world_rank).
 */
typedef int64_t MPIR_Lpid;

#define MPIR_LPID_WORLD_INDEX(lpid) ((lpid) >> 32)
#define MPIR_LPID_WORLD_RANK(lpid)  ((lpid) & 0xffffffff)
#define MPIR_LPID_FROM(world_idx, world_rank) (((uint64_t)(world_idx) << 32) | (world_rank))
/* A dynamic mask is used for temporary lpid during establishing dynamic connections.
 *     dynamic_lpid = MPIR_LPID_DYNAMIC_MASK | index_to_dynamic_av_table
 */
#define MPIR_LPID_DYNAMIC_MASK ((MPIR_Lpid)0x1 << 62)   /* MPIR_Lpid is signed, avoid using the signed bit */
#define MPIR_LPID_INVALID      0xffffffff

#endif /* MPIR_LPID_H_INCLUDED */
