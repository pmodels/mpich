/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TSP_GENTRAN_TYPES_H_INCLUDED
#define TSP_GENTRAN_TYPES_H_INCLUDED

#include "utarray.h"

typedef enum {
    MPII_GENUTIL_VTX_KIND__ISEND,
    MPII_GENUTIL_VTX_KIND__IRECV,
    MPII_GENUTIL_VTX_KIND__IRECV_STATUS,
    MPII_GENUTIL_VTX_KIND__IMCAST,
    MPII_GENUTIL_VTX_KIND__REDUCE_LOCAL,
    MPII_GENUTIL_VTX_KIND__LOCALCOPY,
    MPII_GENUTIL_VTX_KIND__SELECTIVE_SINK,
    MPII_GENUTIL_VTX_KIND__SINK,
    MPII_GENUTIL_VTX_KIND__FENCE,
    MPII_GENUTIL_VTX_KIND__SCHED,
    MPII_GENUTIL_VTX_KIND__CB,
    MPII_GENUTIL_VTX_KIND__LAST,        /* marks the last built-in kind */
} MPII_Genutil_vtx_kind_e;

typedef enum {
    MPII_GENUTIL_VTX_STATE__INIT,
    MPII_GENUTIL_VTX_STATE__ISSUED,
    MPII_GENUTIL_VTX_STATE__COMPLETE,
} MPII_Genutil_vtx_state_e;

struct MPII_Genutil_sched_t;

typedef struct MPII_Genutil_vtx_t {
    UT_array out_vtcs;

    int vtx_kind;
    int vtx_state;
    int vtx_id;

    int pending_dependencies;
    int num_dependencies;

    /* index of its first channel operation in sched->chan_ops (an imcast has one per
     * destination, in order), or -1 for a vertex that neither sends nor receives */
    int chan_op;

    union {
        struct {
            const void *buf;
            MPI_Aint count;
            MPI_Datatype dt;
            int dest;
            int tag;
            MPIR_Comm *comm;
            MPIR_Request *req;
        } isend;
        struct {
            void *buf;
            MPI_Aint count;
            MPI_Datatype dt;
            int src;
            int tag;
            MPIR_Comm *comm;
            MPIR_Request *req;
        } irecv;
        struct {
            void *buf;
            MPI_Aint count;
            MPI_Datatype dt;
            int src;
            int tag;
            MPIR_Comm *comm;
            MPIR_Request *req;
            MPI_Status *status;
        } irecv_status;
        struct {
            const void *buf;
            MPI_Aint count;
            MPI_Datatype dt;
            UT_array dests;
            int num_dests;
            int tag;
            MPIR_Comm *comm;
            MPIR_Request **req;
            int last_complete;
        } imcast;
        struct {
            const void *inbuf;
            void *inoutbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } reduce_local;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            MPI_Aint recvcount;
            MPI_Datatype recvtype;
        } localcopy;
        struct {
            MPIR_TSP_cb_t cb_p;
            void *cb_data;
        } cb;
        struct {
            void *data;
        } generic;
        struct {
            struct MPII_Genutil_sched_t *sched;
            MPIR_Request *req;
            int is_persistent;
        } sched;
    } u;

    struct MPII_Genutil_vtx_t *next;
} MPII_Genutil_vtx_t;

/* A channel is the sends to one peer, or the receives from one peer, of a schedule. Gentran
 * issues the operations of a channel in the order they were added to the schedule, so that
 * operations with the same tag match in that order (see MPII_Genutil_chan_add_op). */
typedef struct MPII_Genutil_chan_op_t {
    int vtx_id;
    int dest_idx;               /* the destination of an imcast; 0 otherwise */
    int prev;                   /* the previous operation on the channel, or -1 */
    int next;                   /* the next operation on the channel, or -1 */
    bool issued;
} MPII_Genutil_chan_op_t;

typedef struct MPII_Genutil_chan_key_t {
    MPIR_Comm *comm;
    int peer;
    int is_recv;
} MPII_Genutil_chan_key_t;

typedef struct MPII_Genutil_chan_t {
    MPII_Genutil_chan_key_t key;
    int last_op;                /* the last operation added to the channel */
    UT_hash_handle hh;
} MPII_Genutil_chan_t;

typedef struct MPII_Genutil_sched_t {
    UT_array vtcs;
    int total_vtcs;
    int completed_vtcs;
    int last_fence;

    MPIR_Request *req;
    /* buffers allocated for schedule execution */
    MPIR_Coll_owned_t owned;

    /* issued vertices linked list */
    struct MPII_Genutil_vtx_t *issued_head;
    struct MPII_Genutil_vtx_t *issued_tail;

    /* list of new type */
    UT_array generic_types;

    /* channel operations (MPII_Genutil_chan_op_t), and the last one of each channel */
    UT_array chan_ops;
    MPII_Genutil_chan_t *chans;

    bool is_persistent;

    /* the tag of the collective call; all its sends and receives use it */
    int tag;
} MPII_Genutil_sched_t;

typedef MPII_Genutil_vtx_t vtx_t;

typedef struct {
    int id;
    MPIR_TSP_sched_issue_fn issue_fn;
    MPIR_TSP_sched_complete_fn complete_fn;
    MPIR_TSP_sched_free_fn free_fn;
} MPII_Genutil_vtx_type_t;

#endif /* TSP_GENTRAN_TYPES_H_INCLUDED */
