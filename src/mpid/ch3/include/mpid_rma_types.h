/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_TYPES_H_INCLUDED)
#define MPID_RMA_TYPES_H_INCLUDED

#include "mpidi_ch3_impl.h"

/* Special case RMA operations */

enum MPIDI_RMA_Datatype {
    MPIDI_RMA_DATATYPE_BASIC = 50,
    MPIDI_RMA_DATATYPE_DERIVED = 51
};

#define MPID_LOCK_NONE 60

/*
 * RMA Declarations.  We should move these into something separate from
 * a Request.
 */

typedef enum MPIDI_RMA_Pool_type {
    MPIDI_RMA_POOL_WIN = 6,
    MPIDI_RMA_POOL_GLOBAL = 7
} MPIDI_RMA_Pool_type_t;

/* for keeping track of RMA ops, which will be executed at the next sync call */
typedef struct MPIDI_RMA_Op {
    struct MPIDI_RMA_Op *next;  /* pointer to next element in list */
    struct MPIDI_RMA_Op *prev;  /* pointer to prev element in list */

    void *origin_addr;
    int origin_count;
    MPI_Datatype origin_datatype;

    void *compare_addr;
    MPI_Datatype compare_datatype;

    void *result_addr;
    int result_count;
    MPI_Datatype result_datatype;

    struct MPID_Request *single_req;    /* used for unstreamed RMA ops */
    struct MPID_Request **multi_reqs;   /* used for streamed RMA ops */
    MPI_Aint reqs_size;         /* when reqs_size == 0, neither single_req nor multi_reqs is used;
                                 * when reqs_size == 1, single_req is used;
                                 * when reqs_size > 1, multi_reqs is used. */

    int target_rank;

    MPIDI_CH3_Pkt_t pkt;
    MPIDI_RMA_Pool_type_t pool_type;
    int piggyback_lock_candidate;

    int issued_stream_count;    /* when >= 0, it specifies number of stream units that have been issued;
                                 * when < 0, it means all stream units of this operation haven been issued. */

    MPID_Request *ureq;

} MPIDI_RMA_Op_t;

typedef struct MPIDI_RMA_Target {
    struct MPIDI_RMA_Op *pending_net_ops_list_head;     /* pending operations that are waiting for network events */
    struct MPIDI_RMA_Op *pending_user_ops_list_head;    /* pending operations that are waiting for user events */
    struct MPIDI_RMA_Op *next_op_to_issue;
    struct MPIDI_RMA_Target *next;
    struct MPIDI_RMA_Target *prev;
    int target_rank;
    enum MPIDI_RMA_states access_state;
    int lock_type;              /* NONE, SHARED, EXCLUSIVE */
    int lock_mode;              /* e.g., MODE_NO_CHECK */
    int win_complete_flag;

    /* The target structure is free to be cleaned up when all of the
     * following conditions hold true:
     *   - No operations are queued up (op_list == NULL)
     *   - There are no outstanding acks (outstanding_acks == 0)
     *   - There are no sync messages to be sent (sync_flag == NONE)
     */
    struct {
        /* next synchronization flag to be sent to the target (either
         * piggybacked or as a separate packet */
        enum MPIDI_RMA_sync_types sync_flag;    /* UNLOCK, FLUSH or FLUSH_LOCAL */

        /* packets sent out that we are expecting an ack for */
        int outstanding_acks;

    } sync;

    /* number of packets that are waiting for local completion */
    int num_pkts_wait_for_local_completion;

    /* number of operations that does not have a FLUSH issued afterwards */
    int num_ops_flush_not_issued;

    MPIDI_RMA_Pool_type_t pool_type;
} MPIDI_RMA_Target_t;

typedef struct MPIDI_RMA_Slot {
    struct MPIDI_RMA_Target *target_list_head;
} MPIDI_RMA_Slot_t;

typedef struct MPIDI_RMA_Target_lock_entry {
    struct MPIDI_RMA_Target_lock_entry *next;
    struct MPIDI_RMA_Target_lock_entry *prev;
    MPIDI_CH3_Pkt_t pkt;        /* all information for this request packet */
    MPIDI_VC_t *vc;
    void *data;                 /* for queued PUTs / ACCs / GACCs, data is copied here */
    int buf_size;
    int all_data_recved;        /* indicate if all data has been received */
} MPIDI_RMA_Target_lock_entry_t;

typedef MPIDI_RMA_Op_t *MPIDI_RMA_Ops_list_t;

#endif /* MPID_RMA_TYPES_H_INCLUDED */
