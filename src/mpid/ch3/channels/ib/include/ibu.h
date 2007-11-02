/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef IBU_H
#define IBU_H

#ifdef __cplusplus
extern "C" {
#endif

/* config header file */
#include "mpidi_ch3i_ib_conf.h"
#include "mpichconf.h"
#include "mpiiov.h"

#ifdef MPID_IBU_TYPE_WINDOWS
#include <winsock2.h>
#include <windows.h>
#include <limits.h>
#endif

#ifndef USE_NO_PIN_CACHE
#define IBU_EAGER_PACKET_SIZE		(1024 * (1 << 3) - IBU_CACHELINE)
#else
#define IBU_EAGER_PACKET_SIZE		(1024 * (1 << 5) - IBU_CACHELINE)
#endif

#define IBU_MAX_PINNED (128*1024*1024)
#define IBU_PACKET_COUNT		64
#define IBU_ERROR_MSG_LENGTH		255
/*
#define IBU_NUM_PREPOSTED_RECEIVES	(IBU_ACK_WATER_LEVEL*3)
#define IBU_MAX_POSTED_SENDS		8192
#define IBU_MAX_DATA_SEGMENTS		100
#define IBU_ACK_WATER_LEVEL		32
*/

/* Mellanox: Added by dafna July 7th */
#define IBU_NUM_OF_RDMA_BUFS    64
#define IBU_LOG_CACHELINE       7
#define IBU_CACHELINE          (128)
#define IBU_RDMA_BUF_SIZE      (IBU_EAGER_PACKET_SIZE + IBU_CACHELINE)
#define IBU_DEFAULT_MAX_WQE     8192
#define IBU_CQ_POLL_WATER_MARK  256
/* Mellanox: END Added by dafna July 7th */

#ifdef USE_IB_VAPI

#include <vapi.h>
#include <vapi_common.h>
#include <mpga.h>
typedef VAPI_cq_hndl_t ibu_set_t;
typedef struct ibu_mem_t
{
    VAPI_mr_hndl_t handle;
    VAPI_lkey_t lkey;
    VAPI_rkey_t rkey;
} ibu_mem_t;
typedef IB_lid_t ibu_lid_t;

#elif defined(USE_IB_IBAL)

#include <ib_al.h>
typedef ib_cq_handle_t ibu_set_t;
typedef struct ibu_mem_t
{
    ib_mr_handle_t handle;
    uint32_t lkey;
    uint32_t rkey;
} ibu_mem_t;
typedef int ibu_lid_t;

#else
#error No infiniband access layer specified
#endif

/* This is a buffer that holds additional footer sent with the data */
/* Size of ibu_rdma_buf_footer_t is IBU_CACHELINE */
typedef struct ibu_rdma_buf_footer_t
{
    int updated_remote_RDMA_recv_limit; /* Piggybacked update of remote Q state */
    int total_size; /* Total bytes of RDMA buf: headers, total data size, valid flag */
    volatile int RDMA_buf_valid_flag; /* Signals the validity of buffer contents */
} ibu_rdma_buf_footer_t;

/* Define whole statement for typedef of rndv_eager_send_t packet type*/
#if defined(MPID_USE_SEQUENCE_NUMBERS)
#define ibu_seqnum_t MPID_Seqnum_t seqnum;
#else
#define ibu_seqnum_t
#endif


/* definitions */
/* Mellanox: Added by dafna July 7th */
typedef enum ibu_rndv_status_t
{
    IBU_RNDV_SUCCESS = 0,
    IBU_RNDV_RTS_SUCCESS,
    IBU_RNDV_RTS_FAIL,
    IBU_RNDV_RTS_IOV_FAIL,
    IBU_RNDV_CTS_IOV_FAIL,
    IBU_RNDV_NO_DEREG
} ibu_rndv_status_t;

/* Mellanox: Added by dafna July 7th */
typedef enum ibu_rdma_buf_valid_flag_t
{
    IBU_INVALID_RDMA_BUF = 0,
    IBU_VALID_RDMA_BUF = 1
} ibu_rdma_buf_valid_flag_t;

typedef enum ibu_rdma_type_t
{
    IBU_RDMA_EAGER_UNSIGNALED = 0,
    IBU_RDMA_RNDV_UNSIGNALED,
    IBU_RDMA_EAGER_SIGNALED,
    IBU_RDMA_RDNV_SIGNALED
} ibu_rdma_type_t;

/* This is the buffer into which eager RDMA is performed */
typedef struct ibu_rdma_buf_t
{
    unsigned char alignment[IBU_CACHELINE - sizeof(ibu_rdma_buf_footer_t)];
    unsigned char buffer[IBU_EAGER_PACKET_SIZE];
    struct ibu_rdma_buf_footer_t footer;
} ibu_rdma_buf_t;

/* Mellanox: END Added by dafna July 7th */

typedef enum ibu_op_e
{
    IBU_OP_TIMEOUT,
    IBU_OP_READ,
    IBU_OP_WRITE,
    IBU_OP_CLOSE,
    IBU_OP_WAKEUP
} ibu_op_t;

/* insert error codes here */
#define IBU_SUCCESS           0
#define IBU_FAIL              -1
#define IBU_ERR_TIMEOUT       1001



/* definitions/structures specific to Windows */
#ifdef MPID_IBU_TYPE_WINDOWS

#define IBU_INFINITE        INFINITE
#define IBU_INVALID_QP      0

/* definitions/structures specific to Unix */
#elif defined(MPID_IBU_TYPE_UNIX)

#define IBU_INFINITE        -1
#define IBU_INVALID_QP      0

#else
#error Error: MPID_IBU_TYPE not defined
#endif

typedef struct ibu_state_t * ibu_t;


/* function prototypes */
int ibu_init(void);
int ibu_finalize(void);
int ibu_get_lid(void);
int ibu_create_set(ibu_set_t *set);
int ibu_destroy_set(ibu_set_t set);
int ibu_set_vc_ptr(ibu_t ibu, void *vc_ptr);
ibu_t ibu_start_qp(ibu_set_t set, int *qp_num_ptr);
int ibu_finish_qp(ibu_t ibu, ibu_lid_t dest_lid, int dest_qpnum);
int ibu_post_read(ibu_t ibu, void *buf, int len);
int ibu_post_readv(ibu_t ibu, MPID_IOV *iov, int n);
int ibu_write(ibu_t ibu, void *buf, int len, int *num_bytes_ptr);
int ibu_writev(ibu_t ibu, MPID_IOV *iov, int n, int *num_bytes_ptr);
int ibu_wait(int millisecond_timeout, void **vc_pptr, int *num_bytes_ptr, ibu_op_t *op_ptr);
/* These functions may or may not cache the memory registrations based on the build parameters */
int ibu_register_memory(void *buf, int len, ibu_mem_t *state);
int ibu_deregister_memory(void *buf, int len, ibu_mem_t *state);
int ibu_invalidate_memory(void *buf, int len);
/* These functions never cache the registrations */
int ibu_nocache_register_memory(void *buf, int len, ibu_mem_t *state);
int ibu_nocache_deregister_memory(void *buf, int len, ibu_mem_t *state);

#ifdef __cplusplus
}
#endif

#endif
