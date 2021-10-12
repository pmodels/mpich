/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_H_INCLUDED
#define MPIDIG_H_INCLUDED

#define MPIDI_AM_HANDLERS_MAX (64)
#define MPIDI_AM_RNDV_CB_MAX  (10)

enum {
    MPIDIG_SEND = 0,

    MPIDIG_SEND_CTS,    /* CTS (clear to send) for send message */
    MPIDIG_SEND_DATA,   /* data for send message */

    MPIDIG_SSEND_ACK,

    MPIDIG_PART_SEND_INIT,
    MPIDIG_PART_CTS,    /* issued by receiver once start is called and rreq matches */
    MPIDIG_PART_SEND_DATA,      /* data issued by sender after received CTS */

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

    MPIDI_IPC_ACK,

    MPIDI_OFI_INTERNAL_HANDLER_CONTROL,
    MPIDI_OFI_AM_RDMA_READ_ACK,

    MPIDIG_HANDLER_STATIC_MAX
};

enum {
    MPIDIG_RNDV_GENERIC = 0,
    MPIDIG_RNDV_IPC,

    MPIDIG_RNDV_STATIC_MAX
};

typedef int (*MPIDIG_am_target_cmpl_cb) (MPIR_Request * req);
typedef int (*MPIDIG_am_origin_cb) (MPIR_Request * req);

/* Target message callback, or handler function
 *
 * If req on input is NULL, the callback may allocate a request object. If a request
 * object is returned, the caller is expected to transfer the payload to the request,
 * and call target_cmpl_cb upon complete.
 *
 * Transport sets attr with a bit mask of flags. We use uint32_t for attr.
 * Bits are reserved as following:
 *     0-7    ch4-layer defined flags (defined below)
 *     8-15   source vci
 *     16-23  destination vci
 *     24-31  reserved for transport (e.g. transport internal rndv id)
 *
 * The vci bits are needed once we enable multiple vcis for active messages.
 */

#define MPIDIG_AM_ATTR__IS_LOCAL  0x1   /* from shm transport */
#define MPIDIG_AM_ATTR__IS_ASYNC  0x2   /* need request and will asynchronously finish the data copy */
#define MPIDIG_AM_ATTR__IS_RNDV   0x4   /* rndv mode, call MPIDI_{NM,SHM}_am_get_data_copy_cb to setup callback */

#define MPIDIG_AM_ATTR_SRC_VCI_SHIFT 8
#define MPIDIG_AM_ATTR_DST_VCI_SHIFT 16
#define MPIDIG_AM_ATTR_TRANSPORT_SHIFT 24

#define MPIDIG_AM_ATTR_SRC_VCI(attr) ((attr >> MPIDIG_AM_ATTR_SRC_VCI_SHIFT) & 0xff)
#define MPIDIG_AM_ATTR_DST_VCI(attr) ((attr >> MPIDIG_AM_ATTR_DST_VCI_SHIFT) & 0xff)
#define MPIDIG_AM_ATTR_SET_VCIS(attr, src_vci, dst_vci) \
    do { \
        attr |= ((src_vci) << MPIDIG_AM_ATTR_SRC_VCI_SHIFT) | ((dst_vci) << MPIDIG_AM_ATTR_DST_VCI_SHIFT); \
    } while (0)

typedef int (*MPIDIG_am_target_msg_cb) (void *am_hdr, void *data, MPI_Aint data_sz,
                                        uint32_t attr, MPIR_Request ** req);
typedef int (*MPIDIG_am_rndv_cb) (MPIR_Request * rreq);

typedef struct MPIDIG_global_t {
    MPIDIG_am_target_msg_cb target_msg_cbs[MPIDI_AM_HANDLERS_MAX];
    MPIDIG_am_origin_cb origin_cbs[MPIDI_AM_HANDLERS_MAX];
    MPIDIG_am_rndv_cb rndv_cbs[MPIDI_AM_RNDV_CB_MAX];
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
void MPIDIG_am_rndv_reg_cb(int rndv_id, MPIDIG_am_rndv_cb rndv_cb);

int MPIDIG_am_init(void);
void MPIDIG_am_finalize(void);

/* am protocol prototypes */

void MPIDIG_am_comm_abort_init(void);
int MPIDIG_am_comm_abort(MPIR_Comm * comm, int exit_code);

int MPIDIG_am_check_init(void);

int MPIDIG_init_comm(MPIR_Comm * comm);
int MPIDIG_destroy_comm(MPIR_Comm * comm);
void *MPIDIG_mpi_alloc_mem(MPI_Aint size, MPIR_Info * info_ptr);
int MPIDIG_mpi_free_mem(void *ptr);

#endif /* MPIDIG_H_INCLUDED */
