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

/* to send derived datatype across in RMA ops */
typedef struct MPIDI_RMA_dtype_info {   /* for derived datatypes */
    int is_contig;
    int max_contig_blocks;
    MPI_Aint size;
    MPI_Aint extent;
    int dataloop_size;          /* not needed because this info is sent in
                                 * packet header. remove it after lock/unlock
                                 * is implemented in the device */
    void *dataloop;             /* pointer needed to update pointers
                                 * within dataloop on remote side */
    int dataloop_depth;
    int basic_type;
    MPI_Aint ub, lb, true_ub, true_lb;
    int has_sticky_ub, has_sticky_lb;
} MPIDI_RMA_dtype_info;

typedef enum MPIDI_RMA_Pool_type {
    MPIDI_RMA_POOL_WIN = 6,
    MPIDI_RMA_POOL_GLOBAL = 7
} MPIDI_RMA_Pool_type_t;

/* for keeping track of RMA ops, which will be executed at the next sync call */
typedef struct MPIDI_RMA_Op {
    struct MPIDI_RMA_Op *next;  /* pointer to next element in list */

    void *origin_addr;
    int origin_count;
    MPI_Datatype origin_datatype;

    void *compare_addr;
    MPI_Datatype compare_datatype;

    void *result_addr;
    int result_count;
    MPI_Datatype result_datatype;

    struct MPID_Request **reqs;
    int reqs_size;

    MPIDI_RMA_dtype_info dtype_info;
    void *dataloop;

    int target_rank;

    MPIDI_CH3_Pkt_t pkt;
    MPIDI_RMA_Pool_type_t pool_type;
    int is_dt;
    int piggyback_lock_candidate;

    int issued_stream_count;    /* when >= 0, it specifies number of stream units that have been issued;
                                 * when < 0, it means all stream units of this operation haven been issued. */

    MPID_Request *ureq;
} MPIDI_RMA_Op_t;

typedef struct MPIDI_RMA_Target {
    struct MPIDI_RMA_Op *read_op_list, *read_op_list_tail;
    struct MPIDI_RMA_Op *write_op_list, *write_op_list_tail;
    struct MPIDI_RMA_Op *dt_op_list, *dt_op_list_tail;
    struct MPIDI_RMA_Op *pending_op_list, *pending_op_list_tail;
    struct MPIDI_RMA_Op *next_op_to_issue;
    struct MPIDI_RMA_Target *next;
    int target_rank;
    enum MPIDI_RMA_states access_state;
    int lock_type;              /* NONE, SHARED, EXCLUSIVE */
    int lock_mode;              /* e.g., MODE_NO_CHECK */
    int accumulated_ops_cnt;
    int disable_flush_local;
    int win_complete_flag;
    int put_acc_issued;         /* indicate if PUT/ACC is issued in this epoch
                                 * after the previous synchronization calls. */

    /* The target structure is free to be cleaned up when all of the
     * following conditions hold true:
     *   - No operations are queued up (op_list == NULL)
     *   - There are no outstanding acks (outstanding_acks == 0)
     *   - There are no incomplete ops (have_remote_incomplete_ops == 0)
     *   - There are no sync messages to be sent (sync_flag == NONE)
     */
    struct {
        /* next synchronization flag to be sent to the target (either
         * piggybacked or as a separate packet */
        enum MPIDI_RMA_sync_types sync_flag;    /* UNLOCK, FLUSH or FLUSH_LOCAL */

        /* packets sent out that we are expecting an ack for */
        int outstanding_acks;

        /* if we sent out any operations, but have not waited for
         * their remote completion, this flag is set.  When the next
         * FLUSH or UNLOCK sync flag is set, we will clear this
         * variable. */
        int have_remote_incomplete_ops; /* have ops that have not completed remotely */
    } sync;

    MPIDI_RMA_Pool_type_t pool_type;
} MPIDI_RMA_Target_t;

typedef struct MPIDI_RMA_Slot {
    struct MPIDI_RMA_Target *target_list;
    struct MPIDI_RMA_Target *target_list_tail;
} MPIDI_RMA_Slot_t;

typedef struct MPIDI_RMA_Win_list {
    MPID_Win *win_ptr;
    struct MPIDI_RMA_Win_list *next;
} MPIDI_RMA_Win_list_t;

extern MPIDI_RMA_Win_list_t *MPIDI_RMA_Win_list, *MPIDI_RMA_Win_list_tail;

typedef struct MPIDI_RMA_Lock_entry {
    struct MPIDI_RMA_Lock_entry *next;
    MPIDI_CH3_Pkt_t pkt;        /* all information for this request packet */
    MPIDI_VC_t *vc;
    void *data;                 /* for queued PUTs / ACCs / GACCs, data is copied here */
    int buf_size;
    int all_data_recved;        /* indicate if all data has been received */
} MPIDI_RMA_Lock_entry_t;

typedef MPIDI_RMA_Op_t *MPIDI_RMA_Ops_list_t;

#endif /* MPID_RMA_TYPES_H_INCLUDED */
