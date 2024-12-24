/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_LPID_H_INCLUDED
#define MPIR_LPID_H_INCLUDED

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
typedef uint64_t MPIR_Lpid;

#define MPIR_LPID_WORLD_INDEX(lpid) ((lpid) >> 32)
#define MPIR_LPID_WORLD_RANK(lpid)  ((lpid) & 0xffffffff)
#define MPIR_LPID_FROM(world_idx, world_rank) (((uint64_t)(world_idx) << 32) | (world_rank))
#define MPIR_LPID_DYNAMIC_MASK ((MPIR_Lpid)0x1 << 63)
#define MPIR_LPID_INVALID      0xffffffff

#endif /* MPIR_LPID_H_INCLUDED */
