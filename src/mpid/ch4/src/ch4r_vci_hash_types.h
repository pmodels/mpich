/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef CH4R_VCI_HASH_TYPES_H_INCLUDED
#define CH4R_VCI_HASH_TYPES_H_INCLUDED

enum {
    MPIDI_CH4_VCI_HASH_COMM_ONLY,
    MPIDI_CH4_VCI_HASH_COMM_RANK,
    MPIDI_CH4_VCI_HASH_COMM_TAG,
    MPIDI_CH4_VCI_HASH_COMM_RANK_TAG,

    MPIDI_CH4_NUM_VCI_HASH_TYPES,
};

static const char *MPIDI_CH4_vci_hash_types[MPIDI_CH4_NUM_VCI_HASH_TYPES] = {
    "comm-only",
    "comm-rank",
    "comm-tag",
    "comm-rank-tag",
};

typedef struct MPIDI_vci_hash_t {
    union {
        struct {
            int nm_vci;
            int shm_vci;
        } single;
        struct {
            int *nm_vci;
            int nm_vci_count;
            int *shm_vci;
            int shm_vci_count;
        } multi;
    } u;
} MPIDI_vci_hash_t;

#endif /* CH4R_VCI_HASH_TYPES_H_INCLUDED */
