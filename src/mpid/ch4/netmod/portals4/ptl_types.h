/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  *  (C) 2016 by Argonne National Laboratory.
 *   *      See COPYRIGHT in top-level directory.
 *    *
 *    */

#ifndef PTL_TYPES_H_INCLUDED
#define PTL_TYPES_H_INCLUDED

#include "mpidimpl.h"
#include "portals4.h"

/* Portals 4 Limits */
#define MPIDI_PTL_EVENT_COUNT          (1024*64)
#define MPIDI_PTL_UNEXPECTED_HDR_COUNT (1024*64)
#define MPIDI_PTL_LIST_SIZE            (1024*64)

/* Active Message Stuff */
#define MPIDI_PTL_NUM_OVERFLOW_BUFFERS (8)
#define MPIDI_PTL_OVERFLOW_BUFFER_SZ   (1024*1024)
#define MPIDI_PTL_MAX_AM_EAGER_SZ      (64*1024)
#define MPIDI_PTL_AM_TAG               (1 << 28)

typedef struct {
    ptl_process_t process;
    ptl_pt_index_t pt;
} MPIDI_PTL_addr_t;

typedef struct {
    MPIDI_PTL_addr_t *addr_table;
    int *node_map;
    int max_node_id;
    char *kvsname;
    char pname[MPI_MAX_PROCESSOR_NAME];
    void *overflow_bufs[MPIDI_PTL_NUM_OVERFLOW_BUFFERS];
    ptl_handle_me_t overflow_me_handles[MPIDI_PTL_NUM_OVERFLOW_BUFFERS];
    ptl_handle_ni_t ni;
    ptl_ni_limits_t ni_limits;
    ptl_handle_eq_t eqs[2];
    ptl_pt_index_t pt;
    ptl_handle_md_t md;
} MPIDI_PTL_global_t;

extern MPIDI_PTL_global_t MPIDI_PTL_global;

#define MPIDI_PTL_CONTEXT_ID_BITS 32
#define MPIDI_PTL_TAG_BITS 32

#define MPIDI_PTL_TAG_MASK      (0x00000000FFFFFFFFULL)
#define MPIDI_PTL_CTX_MASK      (0xFFFFFFFF00000000ULL)
#define MPIDI_PTL_TAG_SHIFT     (MPIDI_PTL_TAG_BITS)

static inline ptl_match_bits_t MPIDI_PTL_init_tag(MPIR_Context_id_t contextid, int tag)
{
    ptl_match_bits_t match_bits = 0;
    match_bits = contextid;
    match_bits <<= MPIDI_PTL_TAG_SHIFT;
    match_bits |= (MPIDI_PTL_TAG_MASK & tag);
    return match_bits;
}

#define MPIDI_PTL_MSG_SZ_MASK   (0x00FFFFFFFFFFFFFFULL)

static inline ptl_hdr_data_t MPIDI_PTL_init_am_hdr(int handler_id, size_t msg_sz)
{
    ptl_hdr_data_t hdr = 0;
    hdr = (ptl_hdr_data_t) handler_id << 56;
    hdr |= (MPIDI_PTL_MSG_SZ_MASK & msg_sz);
    return hdr;
}

#endif /* PTL_TYPES_H_INCLUDED */
