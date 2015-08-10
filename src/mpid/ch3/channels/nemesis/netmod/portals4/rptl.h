/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined RPTL_H_INCLUDED
#define RPTL_H_INCLUDED

#if !defined HAVE_MACRO_VA_ARGS
#error "portals requires VA_ARGS support"
#endif /* HAVE_MACRO_VA_ARGS */

#if defined HAVE__FUNC__
#define RPTLU_FUNC __func__
#elif defined HAVE_CAP__FUNC__
#define RPTLU_FUNC __FUNC__
#elif defined HAVE__FUNCTION__
#define RPTLU_FUNC __FUNCTION__
#else
#define RPTLU_FUNC "Unknown"
#endif

#define RPTLU_ERR_POP(ret, ...)                                         \
    {                                                                   \
        if (ret) {                                                      \
            MPL_error_printf("%s (%d): ", RPTLU_FUNC, __LINE__);       \
            MPL_error_printf(__VA_ARGS__);                             \
            goto fn_fail;                                               \
        }                                                               \
    }

enum rptl_pt_type {
    RPTL_PT_DATA,
    RPTL_PT_CONTROL
};

struct rptl_target;
struct rptl_op {
    enum {
        RPTL_OP_PUT,
        RPTL_OP_GET
    } op_type;

    enum {
        RPTL_OP_STATE_QUEUED,
        RPTL_OP_STATE_ISSUED,
        RPTL_OP_STATE_NACKED
    } state;

    union {
        struct {
            ptl_handle_md_t md_handle;
            ptl_size_t local_offset;
            ptl_size_t length;
            ptl_ack_req_t ack_req;
            ptl_process_t target_id;
            ptl_pt_index_t pt_index;
            ptl_match_bits_t match_bits;
            ptl_size_t remote_offset;
            void *user_ptr;
            ptl_hdr_data_t hdr_data;

            /* internal variables store events */
            ptl_event_t *send;
            ptl_event_t *ack;
            enum rptl_pt_type pt_type;
        } put;
        struct {
            ptl_handle_md_t md_handle;
            ptl_size_t local_offset;
            ptl_size_t length;
            ptl_process_t target_id;
            ptl_pt_index_t pt_index;
            ptl_match_bits_t match_bits;
            ptl_size_t remote_offset;
            void *user_ptr;
        } get;
    } u;

    int events_ready;
    struct rptl_target *target;

    struct rptl_op *next;
    struct rptl_op *prev;
};

#define RPTL_CONTROL_MSG_PAUSE       (0)
#define RPTL_CONTROL_MSG_PAUSE_ACK   (1)
#define RPTL_CONTROL_MSG_UNPAUSE     (2)

struct rptl {
    /* local portal state */
    enum {
        RPTL_LOCAL_STATE_ACTIVE,
        RPTL_LOCAL_STATE_AWAITING_PAUSE_ACKS
    } local_state;
    uint64_t pause_ack_counter;

    struct {
        ptl_handle_eq_t eq;
        ptl_pt_index_t pt;      /* primary pt for data exchange */

        /* ob_max_count refers to the number of messages that were in
         * the overflow buffer when the pt was disabled */
        uint64_t ob_max_count;

        /* ob_curr_count refers to the current tally of messages in
         * the overflow buffer.  if we are in disabled state, when
         * this count reaches half of the maximum count, we are ready
         * to reenable the PT. */
        uint64_t ob_curr_count;
    } data;

    struct {
        ptl_pt_index_t pt;      /* pt for control messages */

        /* the remaining contents of the control structure are only
         * valid when the control.pt field is not PTL_PT_ANY */
        ptl_handle_me_t *me;
        int me_idx;
    } control;

    ptl_handle_ni_t ni;
    ptl_handle_eq_t eq;
    ptl_handle_md_t md;

    struct rptl *next;
    struct rptl *prev;
};

#define RPTL_OP_POOL_SEGMENT_COUNT  (1024)

struct rptl_target {
    ptl_process_t id;

    enum rptl_target_state {
        RPTL_TARGET_STATE_ACTIVE,
        RPTL_TARGET_STATE_DISABLED,
        RPTL_TARGET_STATE_RECEIVED_PAUSE,
        RPTL_TARGET_STATE_PAUSE_ACKED
    } state;

    /* when we get a pause message, we need to know which rptl it came
     * in on, so we can figure out what the corresponding target
     * portal is.  for this, we store the local rptl */
    struct rptl *rptl;

    struct rptl_op_pool_segment {
        struct rptl_op op[RPTL_OP_POOL_SEGMENT_COUNT];
        struct rptl_op_pool_segment *next;
        struct rptl_op_pool_segment *prev;
    } *op_segment_list;
    struct rptl_op *op_pool;

    struct rptl_op *data_op_list;
    struct rptl_op *control_op_list;

    struct rptl_target *next;
    struct rptl_target *prev;

    int issued_data_ops;
};

struct rptl_info {
    struct rptl *rptl_list;
    struct rptl_target *target_list;

    int world_size;
    uint64_t origin_events_left;
    int (*get_target_info) (int rank, ptl_process_t * id, ptl_pt_index_t local_data_pt,
                            ptl_pt_index_t * target_data_pt, ptl_pt_index_t * target_control_pt);
};

extern struct rptl_info rptl_info;


/* initialization */
int MPID_nem_ptl_rptl_init(int world_size, uint64_t max_origin_events,
                           int (*get_target_info) (int rank, ptl_process_t * id,
                                                   ptl_pt_index_t local_data_pt,
                                                   ptl_pt_index_t * target_data_pt,
                                                   ptl_pt_index_t * target_control_pt));
int MPID_nem_ptl_rptl_drain_eq(int eq_count, ptl_handle_eq_t *eq);
int MPID_nem_ptl_rptl_ptinit(ptl_handle_ni_t ni_handle, ptl_handle_eq_t eq_handle, ptl_pt_index_t data_pt,
                             ptl_pt_index_t control_pt);
int MPID_nem_ptl_rptl_ptfini(ptl_pt_index_t pt_index);
int rptli_post_control_buffer(ptl_handle_ni_t ni_handle, ptl_pt_index_t pt,
                              ptl_handle_me_t * me_handle);

/* op management */
int rptli_op_alloc(struct rptl_op **op, struct rptl_target *target);
void rptli_op_free(struct rptl_op *op);

/* communication */
int MPID_nem_ptl_rptl_put(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length,
                          ptl_ack_req_t ack_req, ptl_process_t target_id, ptl_pt_index_t pt_index,
                          ptl_match_bits_t match_bits, ptl_size_t remote_offset, void *user_ptr,
                          ptl_hdr_data_t hdr_data);
int MPID_nem_ptl_rptl_get(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length,
                          ptl_process_t target_id, ptl_pt_index_t pt_index,
                          ptl_match_bits_t match_bits, ptl_size_t remote_offset, void *user_ptr);
int MPID_nem_ptl_rptl_eqget(ptl_handle_eq_t eq_handle, ptl_event_t * event);

#endif /* RPTL_H_INCLUDED */
