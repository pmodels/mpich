/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_H_INCLUDED
#define MPIDIG_AM_H_INCLUDED

#define MPIDI_AM_HANDLERS_MAX (64)

enum {
    MPIDIG_SEND = 0,            /* Eager send */

    MPIDIG_SEND_LONG_REQ,       /* Rendezvous send RTS (request to send) */
    MPIDIG_SEND_LONG_ACK,       /* Rendezvous send CTS (clear to send) */
    /* FIXME: This old long message protocol will be replaced by the new one
     * I left it here will remove in a separate commit to lessen the conflict */
    MPIDIG_SEND_LONG_LMT,       /* Rendezvous send LMT */

    /* new pipeline protocol, RTS, CTS and segment transmission. The protocol start by sender
     * sending RTS to receiver. The RTS contain the headers and the first segment of data.
     * The receiver will to match a posted recv request. If no posted recv, it will create a
     * recv request for the unexpected data and the first chunk will be received into the
     * unexpected buffer (that is allocated from a GenQ private queue). The receiver will only
     * send back the CTS when the incoming/unexpected message is matched. The sender will wait
     * for the CTS before sending the 2nd and rest segments.
     * Note if the receiver received the RTS for the pipeline protocol, it should not refuse
     * and prefer a different protocol. This is because the pipeline is the ultimate fallback
     * protocol for long messages. If the sender is sending the pipeline RTS, it means either
     * pipeline is the only protocol available for this data. This could be due to the SHM/NM
     * only supports pipeline or a different protocol was previously rejected by the receiver.
     * For example, the sender could attemp to send using RDMA read and the receiver cannot/do
     * not want to receive using RDMA read. Thus the sender fallbacks to pipeline. */
    MPIDIG_SEND_PIPELINE_RTS,
    MPIDIG_SEND_PIPELINE_CTS,
    MPIDIG_SEND_PIPELINE_SEG,

    /* new RDMA read protocol, REQ, ACK and NAK. The sender will register the buffer with network
     * and send memory registration info to the receiver. After getting the REQ, the receiver could
     * start the data transmission by doing RDMA reads, or reject the RDMA read protocol by sending
     * NAK, the NAK should contain a list of protocol that the receiver can accept.
     * If the receiver starts the RDMA read transmission, it will send ACK after all the data is
     * received. The sender will clean the memory registration and complete the send. If the sender
     * receives a NAK, it will clean the memory registration and switch to the next protocol that is
     * supported by the receiver (based on the list receiver sends in NAK). Note in the NAK case,
     * the sender is not required to fallback to pipeline immediately. It could choose another
     * protocol other than RDMA read and pipeline, if such a protocol is available. */
    MPIDIG_SEND_RDMA_READ_REQ,
    MPIDIG_SEND_RDMA_READ_ACK,
    MPIDIG_SEND_RDMA_READ_NAK,

    MPIDIG_SSEND_REQ,
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

enum {
    MPIDIG_AM_PROTOCOL__EAGER,
    MPIDIG_AM_PROTOCOL__PIPELINE,
    MPIDIG_AM_PROTOCOL__RDMA_READ,
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
