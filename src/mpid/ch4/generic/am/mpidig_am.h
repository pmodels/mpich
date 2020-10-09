/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_H_INCLUDED
#define MPIDIG_AM_H_INCLUDED

#define MPIDI_AM_HANDLERS_MAX (64)

enum {
    MPIDIG_SEND = 0,

    MPIDIG_SEND_CTS,    /* CTS (clear to send) for send message */
    MPIDIG_SEND_DATA,   /* data for send message */

    MPIDIG_SSEND_ACK,

    MPIDIG_PUT_REQ,
    MPIDIG_PUT_ACK,
    MPIDIG_PUT_DT_REQ,
    MPIDIG_PUT_DAT_REQ,
    MPIDIG_PUT_DT_ACK,

    MPIDIG_GET_REQ,
    MPIDIG_GET_ACK,

    MPIDIG_ACC_REQ,
    MPIDIG_ACC_ACK,
    MPIDIG_ACC_DT_REQ,
    MPIDIG_ACC_DAT_REQ,
    MPIDIG_ACC_DT_ACK,

    MPIDIG_GET_ACC_REQ,
    MPIDIG_GET_ACC_ACK,
    MPIDIG_GET_ACC_DT_REQ,
    MPIDIG_GET_ACC_DAT_REQ,
    MPIDIG_GET_ACC_DT_ACK,

    MPIDIG_CSWAP_REQ,
    MPIDIG_CSWAP_ACK,
    MPIDIG_FETCH_OP,

    MPIDIG_WIN_COMPLETE,
    MPIDIG_WIN_POST,
    MPIDIG_WIN_LOCK,
    MPIDIG_WIN_LOCK_ACK,
    MPIDIG_WIN_UNLOCK,
    MPIDIG_WIN_UNLOCK_ACK,
    MPIDIG_WIN_LOCKALL,
    MPIDIG_WIN_LOCKALL_ACK,
    MPIDIG_WIN_UNLOCKALL,
    MPIDIG_WIN_UNLOCKALL_ACK,

    MPIDIG_COMM_ABORT,

    MPIDI_OFI_INTERNAL_HANDLER_CONTROL,

    MPIDIG_HANDLER_STATIC_MAX
};

typedef int (*MPIDIG_am_target_cmpl_cb) (MPIR_Request * req);
typedef int (*MPIDIG_am_origin_cb) (MPIR_Request * req);

/* Target message callback, or handler function
 *
 * If req on input is NULL, the callback may allocate a request object. If a request
 * object is returned, the caller is expected to transfer the payload to the request,
 * and call target_cmpl_cb upon complete.
 *
 * If is_async is false/0, a request object will never be returned.
 */
typedef int (*MPIDIG_am_target_msg_cb) (int handler_id, void *am_hdr,
                                        void *data, MPI_Aint data_sz,
                                        int is_local, int is_async, MPIR_Request ** req);

typedef struct MPIDIG_global_t {
    MPIDIG_am_target_msg_cb target_msg_cbs[MPIDI_AM_HANDLERS_MAX];
    MPIDIG_am_origin_cb origin_cbs[MPIDI_AM_HANDLERS_MAX];
    /* Control parameters for global progress of RMA target-side active messages.
     * TODO: performance loss need be studied since we add atomic operations
     * in RMA sync and callback routines.*/
    MPL_atomic_int_t rma_am_flag;       /* Indicates whether any incoming RMA target-side active
                                         * messages has been received.
                                         * Set inside each target callback.*/
    MPIR_cc_t rma_am_poll_cntr;
} MPIDIG_global_t;
extern MPIDIG_global_t MPIDIG_global;

void MPIDIG_am_reg_cb(int handler_id,
                      MPIDIG_am_origin_cb origin_cb, MPIDIG_am_target_msg_cb target_msg_cb);
int MPIDIG_am_reg_cb_dynamic(MPIDIG_am_origin_cb origin_cb, MPIDIG_am_target_msg_cb target_msg_cb);

int MPIDIG_am_init(void);
void MPIDIG_am_finalize(void);

/* am protocol prototypes */

void MPIDIG_am_comm_abort_init(void);
int MPIDIG_am_comm_abort(MPIR_Comm * comm, int exit_code);

int MPIDIG_am_check_init(void);

#endif /* MPIDIG_AM_H_INCLUDED */
