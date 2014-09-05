/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 NEC Corporation
 *      Author: Masamichi Takagi
 *  (C) 2012 Oct 14 Yutaka Ishikawa, ishikawa@is.s.u-tokyo.ac.jp
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <linux/mman.h> /* make it define MAP_ANONYMOUS */
#include "mpid_nem_impl.h"

#ifdef HAVE_LIBDCFA
#include "dcfa.h"

/*
*** diff -p verbs.h dcfa.h (structures)
same name, same fields
   struct ibv_device { };
   struct ibv_context { };
   struct ibv_pd { };
   struct ibv_ah_attr { };

same name, different fields
 struct ibv_qp_init_attr {
-  void *qp_context;
-  struct ibv_xrc_domain  *xrc_domain;
};

 struct ibv_mr {
-  void *addr;
+  void *buf;
+  uint64_t host_addr;
-  size_t length;
+  int size;
-  uint32_t handle;
+  uint64_t handle;
+  int flag;  1: offload
-  uint32_t lkey;
+  int lkey;
-  uint32_t rkey;
+  int rkey;
};

 struct ibv_qp {
+  struct mlx4_buf buf;
+  int max_inline_data;
+  int buf_size;

+  uint32_t doorbell_qpn;
+  uint32_t sq_signal_bits;
+  int sq_spare_wqes;
+  struct mlx4_wq sq;

+  uint32_t *db; // doorbell addr for post recv
+  struct mlx4_wq rq;
+  ibmic_qp_conn_info_t   remote_qp_info;

-  uint32_t               handle;
+  uint64_t               handle;

-  struct ibv_context     *context;
-  void                   *qp_context;
-  uint32_t                events_completed;
-  struct ibv_xrc_domain  *xrc_domain;
-  pthread_mutex_t         mutex;
-  pthread_cond_t          cond;
};

 struct ibv_cq {
-  struct ibv_comp_channel *channel;
-  void                   *cq_context;
-  uint32_t                handle;
-  uint32_t                comp_events_completed;
-  uint32_t                async_events_completed;

-  pthread_mutex_t         mutex;
-  pthread_cond_t          cond;

+  struct mlx4_buf         buf;
+  uint32_t                cons_index;
+  uint32_t                wait_index;
+  uint32_t               *set_ci_db;
+  uint32_t               *arm_db;
+  int                     arm_sn;
+  int                     cqe_size;
+  uint64_t                handle;
};

 struct ibv_wc {
-  uint32_t                src_qp;
-  uint16_t                pkey_index;
-  uint16_t                slid;
-  uint8_t                 sl;
-  uint8_t                 dlid_path_bits;
};

 struct ibv_send_wr {
-  struct ibv_sge         *sg_list;
+  struct ibv_sge          sg_list[WR_SG_NUM];
+  uint64_t                addr;
+  uint32_t                length;
+  uint32_t                lkey;
 };

 struct ibv_recv_wr {
-  struct ibv_sge         *sg_list;
+  struct ibv_sge          sg_list[WR_SG_NUM];
 };

 struct ibv_sge {
+  uint64_t mic_addr; // buffer address on mic
 };

non-existent
-  struct ibv_port_attr { };


*** diff -p verbs.h dcfa.h (functions)

same name, same arguments
   ibv_get_device_list
   ibv_open_device	
   ibv_close_device
   ibv_free_device_list
   ibv_alloc_pd
   ibv_dealloc_pd
   ibv_create_qp
   ibv_destroy_qp
   ibv_reg_mr
   ibv_dereg_mr
   ibv_destroy_cq
   ibv_poll_cq
   ibv_modify_qp

same name, different arguments
-  int ibv_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr, struct ibv_send_wr **bad_wr)
+  int ibv_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr);

-  int ibv_post_recv(struct ibv_qp *qp, struct ibv_recv_wr *wr, struct ibv_recv_wr **bad_wr)
+  int ibv_post_recv(struct ibv_qp *qp, struct ibv_recv_wr *wr);

-  struct ibv_cq *ibv_create_cq(struct ibv_context *context, int cqe, void *cq_context, struct ibv_comp_channel *channel, int comp_vector);
+  struct ibv_cq *ibv_create_cq(struct ibv_context *context, int cqe_max);

non-existent
-  ibv_get_device_name
-  ibv_query_port
-  ibv_query_gid
-  ibv_create_ah
struct ibv_ah *ibv_create_ah(struct ibv_pd *pd, struct ibv_ah_attr *attr);
*/

#else
/* Original Infiniband */
#include <infiniband/verbs.h>
#endif

static inline unsigned long long MPID_nem_ib_rdtsc_cpuid(void)
{
    unsigned int lo, hi;
    __asm__ __volatile__(// serialize
                            "xorl %%eax,%%eax \n        cpuid":::"%rax", "%rbx", "%rcx", "%rdx");
    __asm__ __volatile__("rdtsc":"=a"(lo), "=d"(hi));
    return (unsigned long long) hi << 32 | lo;
}

#define MPID_NEM_IB_RDMAWR_FROM_ALLOC_NID 32

extern struct ibv_cq *MPID_nem_ib_rc_shared_scq;
extern struct ibv_cq *MPID_nem_ib_rc_shared_scq_scratch_pad;
extern struct ibv_cq *MPID_nem_ib_rc_shared_rcq_scratch_pad;
extern struct ibv_cq *MPID_nem_ib_ud_shared_rcq;
extern uint8_t *MPID_nem_ib_scratch_pad;
extern int MPID_nem_ib_scratch_pad_ref_count;
extern char *MPID_nem_ib_rdmawr_from_alloc_free_list_front[MPID_NEM_IB_RDMAWR_FROM_ALLOC_NID];
extern char *MPID_nem_ib_rdmawr_from_alloc_arena_free_list[MPID_NEM_IB_RDMAWR_FROM_ALLOC_NID];
extern struct ibv_mr *MPID_nem_ib_rdmawr_to_alloc_mr;
extern uint8_t *MPID_nem_ib_rdmawr_to_alloc_start;
extern uint8_t *MPID_nem_ib_rdmawr_to_alloc_free_list;

#define MPID_NEM_IB_COM_SIZE		(65536*2)       /* Maxiumum number of QPs. One process uses 2 QPs. */
#define MPID_NEM_IB_COM_INLINE_DATA (512-64) /* experimented max is 884 */      /* this is lower bound and more than this value is set. the more this value is, the more the actual value set is. you need to check it */

#define MPID_NEM_IB_COM_MAX_SQ_CAPACITY (256/1)
#define MPID_NEM_IB_COM_MAX_RQ_CAPACITY ((MPID_NEM_IB_COM_MAX_SQ_CAPACITY)+16)  /* We pre-post_recv MPID_NEM_IB_COM_MAX_SQ_CAPACITY of commands */
#define MPID_NEM_IB_COM_MAX_SGE_CAPACITY (32/2) /* maximum for ConnectX-3 looks like 32 */
#define MPID_NEM_IB_COM_MAX_CQ_CAPACITY MPID_NEM_IB_COM_MAX_RQ_CAPACITY
#define MPID_NEM_IB_COM_MAX_CQ_HEIGHT_DRAIN (((MPID_NEM_IB_COM_MAX_CQ_CAPACITY)>>2)+((MPID_NEM_IB_COM_MAX_CQ_CAPACITY)>>1))     /* drain when reaching this amount */
#define MPID_NEM_IB_COM_MAX_SQ_HEIGHT_DRAIN (((MPID_NEM_IB_COM_MAX_SQ_CAPACITY)>>2)+((MPID_NEM_IB_COM_MAX_SQ_CAPACITY)>>1))     /* drain when reaching this amount */
#define MPID_NEM_IB_COM_AMT_CQ_DRAIN ((MPID_NEM_IB_COM_MAX_CQ_CAPACITY)>>2)     /* drain this amount */
#define MPID_NEM_IB_COM_MAX_RD_ATOMIC 4

#define MPID_NEM_IB_COM_MAX_TRIES		 1
#define MPID_NEM_IB_COM_SCQ_FLG		 1
#define MPID_NEM_IB_COM_RCQ_FLG		 2

#define MPID_NEM_IB_COM_INFOKEY_PATTR_MAX_MSG_SZ 100
#define MPID_NEM_IB_COM_INFOKEY_MR_ADDR 200
#define MPID_NEM_IB_COM_INFOKEY_MR_LENGTH 201
#define MPID_NEM_IB_COM_INFOKEY_MR_RKEY 202
#define MPID_NEM_IB_COM_INFOKEY_QP_QPN 300
#define MPID_NEM_IB_COM_INFOKEY_PORT_LID 400
#define MPID_NEM_IB_COM_INFOKEY_PORT_GID 401


/* buffers */
#define MPID_NEM_IB_COM_NBUF_RDMA 2     /* number of <addr, sz, lkey, rkey> */
#define MPID_NEM_IB_COM_RDMAWR_FROM 0   /* index to RDMA-write-from buffer */
#define MPID_NEM_IB_COM_RDMAWR_TO 1     /* index to RDMA-write-to buffer */
/* assuming that the unit (32768) is equals to eager-RDMA-write threashold
   assuming that the multiplier (256) is
   equals to max number of outstanding eager-RDMA-write transactions */
#define MPID_NEM_IB_COM_RDMABUF_SZSEG (16384/4) //(16384+8+40+1) /* this size minus magics and headers must be 2^n because data might grow to the next 2^m boundary, see ib_impl.h, ib_com.c, src/mpid/ch3/src/mpid_isend.c */
#define MPID_NEM_IB_COM_RDMABUF_SZ ((MPID_NEM_IB_COM_RDMABUF_SZSEG) * 16)       /* (32768 * 256) */
#define MPID_NEM_IB_COM_RDMABUF_NSEG ((MPID_NEM_IB_COM_RDMABUF_SZ) / (MPID_NEM_IB_COM_RDMABUF_SZSEG))

#define MPID_NEM_IB_RINGBUF_SHARED_SZSEG (16384/4)
#define MPID_NEM_IB_RINGBUF_SHARED_SZ ((MPID_NEM_IB_RINGBUF_SHARED_SZSEG) * 16)
#define MPID_NEM_IB_RINGBUF_SHARED_NSEG ((MPID_NEM_IB_RINGBUF_SHARED_SZ) / (MPID_NEM_IB_RINGBUF_SHARED_SZSEG))

#define MPID_NEM_IB_COM_SMT_INLINE_NCHAIN 8     /* maximum number of chained inline-send commands */
#define MPID_NEM_IB_COM_RDMABUF_HIGH_WATER_MARK (((MPID_NEM_IB_COM_RDMABUF_NSEG)>>1)+((MPID_NEM_IB_COM_RDMABUF_NSEG)>>2))
#define MPID_NEM_IB_COM_RDMABUF_LOW_WATER_MARK (((MPID_NEM_IB_COM_RDMABUF_NSEG)>>2))
#define MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_HW 1
#define MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_LW 2
#define MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_RATE_HW /*1*/(((MPID_NEM_IB_COM_RDMABUF_NSEG)>>4) == 0 ? 1 : ((MPID_NEM_IB_COM_RDMABUF_NSEG)>>4))
#define MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_RATE_LW (((MPID_NEM_IB_COM_RDMABUF_NSEG)>>2)) /*12*/   /* receiver tries to notify sender the number of releases when receiver find not-noticed releases of more than this number */
#define MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_RATE_DELAY_MULTIPLIER(notify_rate) (notify_rate + (notify_rate>>1)) /* (notify_rate) */        /* send seq_num to the sender side if there is no chance to embed seq_num into a packet bound for the sender side for this number of release events */

#define MPID_NEM_IB_COM_NBUF_UD 2       /* number of <addr, sz, lkey, rkey> */
#define MPID_NEM_IB_COM_UDWR_FROM 0     /* index to UD-write-from buffer */
#define MPID_NEM_IB_COM_UDWR_TO 1       /* index to UD-write-to buffer */
#define MPID_NEM_IB_COM_UDBUF_SZ (128 * 8192)   /* supporting 100K ranks with 10 rounds */
#define MPID_NEM_IB_COM_UDBUF_SZSEG (128)
#define MPID_NEM_IB_COM_UDBUF_NSEG (MPID_NEM_IB_COM_UDBUF_SZ / MPID_NEM_IB_COM_UDBUF_SZSEG)

#define MPID_NEM_IB_COM_NBUF_SCRATCH_PAD 2      /* number of <addr, sz, lkey, rkey> */
#define MPID_NEM_IB_COM_SCRATCH_PAD_FROM_SZ 4096
#define MPID_NEM_IB_COM_SCRATCH_PAD_FROM 0
#define MPID_NEM_IB_COM_SCRATCH_PAD_TO 1        /* index to RDMA-write-to buffer */

/* send command templates */
#define MPID_NEM_IB_COM_RC_SR_NTEMPLATE (8+1+2) /* number of request templates, 8 for inline-chained-smt, 1 for smt, 1 for lmt */
#define MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 0   /* index to it */
#define MPID_NEM_IB_COM_SMT_INLINE_CHAINED7 7
#define MPID_NEM_IB_COM_SMT_NOINLINE 8
#define MPID_NEM_IB_COM_LMT_INITIATOR 9 /* FIXME: bad naming */
#define MPID_NEM_IB_COM_LMT_PUT 10

/* recv command templates */
#define MPID_NEM_IB_COM_RC_RR_NTEMPLATE 1       /* 1 for smt, */
#define MPID_NEM_IB_COM_RDMAWR_RESPONDER  0     /* index to recv request template */

/* sge template */
#define MPID_NEM_IB_COM_SMT_INLINE_INITIATOR_NSGE 4     /* MPI header, (sz;magic), data x1, magic */
#define MPID_NEM_IB_COM_SMT_NOINLINE_INITIATOR_NSGE 4   /* MPI header, (sz;magic), data x1, magic */
#define MPID_NEM_IB_COM_LMT_INITIATOR_NSGE 1    /* data x1 */
#define MPID_NEM_IB_COM_LMT_PUT_NSGE 1  /* data x1 */
#define MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR_NSGE 1    /* QP state */
#define MPID_NEM_IB_COM_SCRATCH_PAD_CAS_NSGE 1  /* QP state */
#define MPID_NEM_IB_COM_SCRATCH_PAD_GET_NSGE 1

#define MPID_NEM_IB_COM_UD_SR_NTEMPLATE 1
#define MPID_NEM_IB_COM_UD_RR_NTEMPLATE 1
#define MPID_NEM_IB_COM_UD_INITIATOR 0  /* index to send request template */
#define MPID_NEM_IB_COM_UD_RESPONDER 0  /* index to recv request template */

#define MPID_NEM_IB_COM_SCRATCH_PAD_SR_NTEMPLATE 4
#define MPID_NEM_IB_COM_SCRATCH_PAD_RR_NTEMPLATE 1
#define MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR 0 /* index to send request template */
#define MPID_NEM_IB_COM_SCRATCH_PAD_CAS       1
#define MPID_NEM_IB_COM_SCRATCH_PAD_GET       2
#define MPID_NEM_IB_COM_SCRATCH_PAD_WR        3
#define MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER 0 /* index to recv request template */

/* Header prepended to the MPI packet */
#define MPID_NEM_IB_NETMOD_HDR_RINGBUF_TYPE_GET(buf)     ((uint32_t)(((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first >> 61))
#define MPID_NEM_IB_NETMOD_HDR_RINGBUF_TYPE_SET(buf, val)     ((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first = (((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first & ~(7ULL<<61)) | ((uint64_t)(val) << 61)

#define MPID_NEM_IB_NETMOD_HDR_RELINDEX_GET(buf)     ((int16_t)((((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first >> 32) & 65535))
#define MPID_NEM_IB_NETMOD_HDR_RELINDEX_SET(buf, val)     ((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first = (((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first & ~(65535ULL<<32)) | ((uint64_t)((val&65535)) << 32)

/* Note that the result is put into [63:32] */
#define MPID_NEM_IB_NETMOD_HDR_ACQADDRH_GET(buf)     ((uint64_t)((((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first << 12) & (((1ULL<<32)-1)<<32)))
/* Note that the value to put is located in [63:32] */
#define MPID_NEM_IB_NETMOD_HDR_ACQADDRH_SET(buf, val)     ((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first = (((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first & ~(((1ULL<<32)-1)<<20)) | (((val) & (((1ULL<<32)-1)<<32)) >> 12)

#define MPID_NEM_IB_NETMOD_HDR_ACQADDR_GET(buf) (MPID_NEM_IB_NETMOD_HDR_ACQADDRH_GET(buf)|((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->second)
#define MPID_NEM_IB_NETMOD_HDR_ACQADDR_SET(buf, val) MPID_NEM_IB_NETMOD_HDR_ACQADDRH_SET((buf), (val)); ((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->second = ((val) & ((1ULL<<32)-1))

#define MPID_NEM_IB_NETMOD_HDR_ACQAMTLOG_GET(buf)     ((uint32_t)((((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first >> 16) & 15))
#define MPID_NEM_IB_NETMOD_HDR_ACQAMTLOG_SET(buf, val)     ((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first = (((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first & ~(15ULL<<16)) | ((uint64_t)(val) << 16)

#define MPID_NEM_IB_NETMOD_HDR_SZ_GET(buf)     ((uint32_t)(((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first & 65535))
#define MPID_NEM_IB_NETMOD_HDR_SZ_SET(buf, val) ((MPID_nem_ib_netmod_hdr_exclusive_t *)(buf))->first = (((MPID_nem_ib_netmod_hdr_exclusive_t *)buf)->first & ~65535ULL) | (val)

#define MPID_NEM_IB_NETMOD_HDR_VC_GET(buf)     ((struct MPIDI_VC *)(((uint64_t)((MPID_nem_ib_netmod_hdr_shared_t *)(buf))->third << 32) | (uint64_t)((MPID_nem_ib_netmod_hdr_shared_t *)(buf))->forth))
#define MPID_NEM_IB_NETMOD_HDR_VC_SET(buf, val) ((MPID_nem_ib_netmod_hdr_shared_t *)(buf))->third = (uint64_t)(val) >> 32; ((MPID_nem_ib_netmod_hdr_shared_t *)(buf))->forth = (uint64_t)(val) & ((1ULL << 32) - 1);

#define MPID_NEM_IB_NETMOD_HDR_SIZEOF(type)     (((type) == MPID_NEM_IB_RINGBUF_EXCLUSIVE) ? sizeof(MPID_nem_ib_netmod_hdr_exclusive_t) : sizeof(MPID_nem_ib_netmod_hdr_shared_t))
#define MPID_NEM_IB_NETMOD_HDR_SIZEOF_GET(buf)     ((MPID_NEM_IB_NETMOD_HDR_RINGBUF_TYPE_GET(buf) & MPID_NEM_IB_RINGBUF_EXCLUSIVE) ? sizeof(MPID_nem_ib_netmod_hdr_exclusive_t) : sizeof(MPID_nem_ib_netmod_hdr_shared_t))

#define MPID_NEM_IB_NETMOD_HDR_HEAD_FLAG_PTR(buf)     (&((MPID_nem_ib_netmod_hdr_shared_t *)(buf))->first)
#define MPID_NEM_IB_NETMOD_HDR_HEAD_FLAG_SET(buf, val) ((MPID_nem_ib_netmod_hdr_shared_t *)(buf))->first = (val);

typedef struct MPID_nem_ib_netmod_hdr_exclusive {
    /*
     * [63:61] ring buffer type
     * remote is exclusive:
     * [47:32]  largest index of contiguous released slots 16-bit
     * reply to slot request:
     * [51:20] Start address of acquired slots, MSB part
     * [19:16] Log_2 of amount of acquired slots
     * [15:0] Packet size without padding
     */
    uint64_t first;
    /* jump case:
     * [31:0] Start address of acquired slots, LSB part
     */
    uint32_t second;

} MPID_nem_ib_netmod_hdr_exclusive_t;

typedef struct MPID_nem_ib_netmod_hdr_shared {
    uint64_t first;
    uint32_t second;

    /* remote is one slot:
     * [31:0] VC pointer in remote node, MSB part */
    uint32_t third;

    /* remote is one slot:
     * [31:0] VC pointer in remote node, LSB part */
    uint32_t forth;
} MPID_nem_ib_netmod_hdr_shared_t;

typedef struct MPID_nem_ib_netmod_trailer {
    uint8_t tail_flag;
    //uint32_t traits; /* for debug */
} MPID_nem_ib_netmod_trailer_t;

/* Allocator for RDMA write to buffer */
typedef struct {
    /* Avoid polluting netmod_hdr and trailer */
    uint8_t padding[sizeof(MPID_nem_ib_netmod_hdr_shared_t)];
    uint8_t *next;
}
MPID_nem_ib_rdmawr_to_alloc_hdr_t;

typedef struct {
    uint64_t wr_id;             /* address of MPID_Request */
    int mf;                     /* more fragment (0 means the end of packet) */
    void *mr_cache;             /* address of mr_cache_entry. derecement refc in drain_scq */
} MPID_nem_ib_rc_send_request;

#define MPID_NEM_IB_LMT_LAST_PKT        0
#define MPID_NEM_IB_LMT_SEGMENT_LAST    1
#define MPID_NEM_IB_LMT_PART_OF_SEGMENT 2
#define MPID_NEM_IB_LAST_PKT            MPID_NEM_IB_LMT_LAST_PKT

/* Ring-buffer to which a remote note RDMA-writes */
#define MPID_NEM_IB_NRINGBUF 64
#define MPID_NEM_IB_RINGBUF_NSLOT 16

/* Ring-buffer type. It is set by ringbuf_alloc on the receiver side
   and sent in SYNACK or ACK1 to the sender side and referenced by isend
   on the sender side and by poll on the receiver side */
/* Exclusive ring buffer has been allocated */
#define MPID_NEM_IB_RINGBUF_EXCLUSIVE 1
/* Shared ring buffer has been allocated */
#define MPID_NEM_IB_RINGBUF_SHARED 2
#define MPID_NEM_IB_RINGBUF_RELINDEX 4

typedef struct {
    uint32_t type;              /* acquiring contiguous slots or a single slot */
    void *start;
    int nslot;
    MPIDI_VC_t *vc;
    uint64_t remote_released[(MPID_NEM_IB_COM_RDMABUF_NSEG + 63) / 64];
    int ref_count;              /* number of VCs sharing the ring-buffer */
} MPID_nem_ib_ringbuf_t;

/* Represent a ring-buffer is exclusively acquired */
extern uint64_t MPID_nem_ib_ringbuf_acquired[(MPID_NEM_IB_NRINGBUF + 63) / 64];

/* Represent a ring-buffer is ready to poll */
extern uint64_t MPID_nem_ib_ringbuf_allocated[(MPID_NEM_IB_NRINGBUF + 63) / 64];

extern MPID_nem_ib_ringbuf_t *MPID_nem_ib_ringbuf;


/* Next ring-buffer type and slots
   Exclusive slots are sticky.
   Shared slot is consumed.
   Use the type described here because we need to
   use up acquired slots of shared ring-buffer when
   transitioning from share to exclusive.
   The next type is absent means we're transitioning
   from exclusive to shared. */
typedef struct MPID_nem_ib_ringbuf_sector {
    uint32_t type;
    void *start;
    int nslot;
    uint16_t head;
    uint16_t tail;

    struct MPID_nem_ib_ringbuf_sector *sectorq_next;
} MPID_nem_ib_ringbuf_sector_t;

typedef GENERIC_Q_DECL(MPID_nem_ib_ringbuf_sector_t) MPID_nem_ib_ringbuf_sectorq_t;

#define MPID_nem_ib_ringbuf_sectorq_empty(q) GENERICM_Q_EMPTY (q)
#define MPID_nem_ib_ringbuf_sectorq_head(q) GENERICM_Q_HEAD (q)
#define MPID_nem_ib_ringbuf_sectorq_next_field(ep, next_field) ((ep)->next_field)
#define MPID_nem_ib_ringbuf_sectorq_next(ep) ((ep)->sectorq_next)
#define MPID_nem_ib_ringbuf_sectorq_enqueue(qp, ep) GENERICM_Q_ENQUEUE (qp, ep, MPID_nem_ib_ringbuf_sectorq_next_field, sectorq_next);
#define MPID_nem_ib_ringbuf_sectorq_dequeue(qp, epp) GENERICM_Q_DEQUEUE (qp, epp, MPID_nem_ib_ringbuf_sectorq_next_field, sectorq_next);


/* IB connection */
typedef struct MPID_nem_ib_com {
    short icom_used;
    short icom_connected;
    int icom_port;
#ifdef HAVE_LIBDCFA
#else
    struct ibv_port_attr icom_pattr;    /* IB port attributes */
#endif
    struct ibv_qp *icom_qp;
    struct ibv_cq *icom_scq;
    struct ibv_cq *icom_rcq;
    struct ibv_mr **icom_mrlist;
    int icom_mrlen;
    union ibv_gid icom_gid;
    void **icom_mem;            /* 0: send 1: recv 2..: rdma */
    int *icom_msize;            /* 0: send 1: recv 2..: rdma */
    struct ibv_send_wr *icom_sr;
    struct ibv_ah_attr *icom_ah_attr;
    struct ibv_recv_wr *icom_rr;
    void **icom_rmem;
    int *icom_rkey;
    size_t *icom_rsize;
    uint16_t sseq_num;
    uint16_t rsr_seq_num_poll;
    uint16_t rsr_seq_num_tail;  /* occupation status of remote Send Request (SR) queue (it covers occupation status of local RDMA-wr-to buffer) */
    uint16_t rsr_seq_num_tail_last_sent;        /* latest one sent to remote rank */
    uint16_t lsr_seq_num_tail;  /* occupation status of local Send Request (SR) queue */
    int lsr_seq_num_tail_last_requested;        /* value when lmt_start_send issued req_seq_num */
    int rdmabuf_occupancy_notify_rstate, rdmabuf_occupancy_notify_lstate;
    int ncom, ncom_scratch_pad; /* number of entries in the command queue */

    uint32_t max_inline_data;   /* actual value obtained after ibv_create_qp */
    uint32_t max_send_wr;
    uint32_t max_recv_wr;

    uint32_t open_flag;         /* MPID_NEM_IB_COM_OPEN_UD, ... */
    uint16_t remote_lid;        /* for debug */

    /* other commands can be executed before RDMA-rd command */
    /* see the "Ordering and the Fence Indicator" section in "InfiniBand Architecture" by William T. Futral */
    uint16_t after_rdma_rd;

    /* Ring-buffer information on the receiver side.
     * It's allocated on the receiver side. */
    MPID_nem_ib_ringbuf_t *remote_ringbuf;

    /* Ring buffer information on the sender side.
     * The information is passed from the receiver side on connection. */
    uint32_t local_ringbuf_type;
    void *local_ringbuf_start;
    int local_ringbuf_rkey;
    uint16_t local_ringbuf_nslot;

    /* VC of remote node. It's embedded in a packet going to the
     * shared ring buffer because no VC information is available on
     * the receiver side in the shared case. c.f. They are stored in
     * the individual exclusive ring-buffers in the exclusive case. */
    MPIDI_VC_t *remote_vc;

    /* Delay the fetch of the second ask until the first issues CAS */
    uint8_t ask_guard;

    /* Ring buffer sectors obtained through ask-send protocol */
    MPID_nem_ib_ringbuf_sectorq_t sectorq;


    /* Two transactions from the both ends for a connection
     * can be outstanding at the same time when they were initiated
     * at the same time. This makes one end try to send ACK2 after
     * freeing scratch-pad QP for the connection. So we must monitor and
     * wait until all the onnection request transactions ends before
     * freeing scratch-pad QP. */
    int outstanding_connection_tx;
    int incoming_connection_tx;
    int notify_outstanding_tx_empty;

} MPID_nem_ib_com_t;

extern void *MPID_nem_ib_rdmawr_to_alloc(int nslots);
extern void MPID_nem_ib_rdmawr_to_free(void *p, int nslots);
extern int MPID_nem_ib_rdmawr_to_munmap(void *p, int nslots);
extern int MPID_nem_ib_com_open(int ib_port, int MPID_nem_ib_com_open_flag, int *condesc);
extern int MPID_nem_ib_com_close(int);
extern int MPID_nem_ib_com_alloc(int condesc, int sz);
extern int MPID_nem_ib_com_free(int condesc, int sz);
extern int MPID_nem_ib_com_rts(int condesc, int remote_qpnum, uint16_t remote_lid,
                               union ibv_gid *remote_gid);

extern int MPID_nem_ib_com_reg_mr_connect(int condesc, void *rmem, int rkey);
extern int MPID_nem_ib_com_connect_ringbuf(int condesc,
                                           uint32_t ringbuf_type,
                                           void *start, int rkey, int nslot,
                                           MPIDI_VC_t * remote_vc, uint32_t alloc_new_mr);

extern int MPID_nem_ib_com_isend(int condesc,
                                 uint64_t wr_id,
                                 void *prefix, int sz_prefix,
                                 void *hdr, int sz_hdr,
                                 void *data, int sz_data,
                                 int *copied,
                                 uint32_t local_ringbuf_type, uint32_t remote_ringbuf_type,
                                 void **buf_from_out, uint32_t * buf_from_sz_out);
extern int MPID_nem_ib_com_isend_chain(int condesc, uint64_t wr_id, void *hdr, int sz_hdr,
                                       void *data, int sz_data);
extern int MPID_nem_ib_com_put_scratch_pad(int condesc, uint64_t wr_id, uint64_t offset, int sz,
                                           void *laddr, void **buf_from_out,
                                           uint32_t * buf_from_sz_out);
extern int MPID_nem_ib_com_get_scratch_pad(int condesc, uint64_t wr_id, uint64_t offset, int sz,
                                           void **buf_from_out, uint32_t * buf_from_sz_out);
extern int MPID_nem_ib_com_cas_scratch_pad(int condesc, uint64_t wr_id, uint64_t offset,
                                           uint64_t compare, uint64_t swap, void **buf_from_out,
                                           uint32_t * buf_from_sz_out);
extern int MPID_nem_ib_com_wr_scratch_pad(int condesc, uint64_t wr_id,
                                          void *buf_from, uint32_t buf_from_sz);

//extern int MPID_nem_ib_com_isend(int condesc, uint64_t wr_id, void* hdr, int sz_hdr, void* data, int sz_data);
extern int MPID_nem_ib_com_irecv(int condesc, uint64_t wr_id);
extern int MPID_nem_ib_com_udsend(int condesc, union ibv_gid *remote_gid, uint16_t remote_lid,
                                  uint32_t remote_qpn, uint32_t imm_data, uint64_t wr_id);
extern int MPID_nem_ib_com_udrecv(int condesc);
extern int MPID_nem_ib_com_lrecv(int condesc, uint64_t wr_id, void *raddr, long sz_data,
                                 uint32_t rkey, void *laddr, int last);
extern int MPID_nem_ib_com_put_lmt(int condesc, uint64_t wr_id, void *raddr, int sz_data,
                                   uint32_t rkey, void *laddr);
extern int MPID_nem_ib_com_scratch_pad_recv(int condesc, int sz_data);
extern int MPID_nem_ib_com_poll_cq(int which_cq, struct ibv_wc *wc, int *result);

extern int MPID_nem_ib_com_obtain_pointer(int condesc, MPID_nem_ib_com_t ** MPID_nem_ib_com);

/* for ib_reg_mr.c */
extern int MPID_nem_ib_com_reg_mr(void *addr, long len, struct ibv_mr **mr,
                                  enum ibv_access_flags additional_flags);
extern int MPID_nem_ib_com_dereg_mr(struct ibv_mr *mr);

extern int MPID_nem_ib_com_get_info_conn(int condesc, int key, void *out, uint32_t out_len);
extern int MPID_nem_ib_com_get_info_mr(int condesc, int memid, int key, void *out, int out_len);

extern int MPID_nem_ib_com_rdmabuf_occupancy_notify_rate_get(int condesc, int *notify_rate);
extern int MPID_nem_ib_com_rdmabuf_occupancy_notify_rstate_get(int condesc, int **rstate);
extern int MPID_nem_ib_com_rdmabuf_occupancy_notify_lstate_get(int condesc, int **lstate);

extern char *MPID_nem_ib_com_strerror(int err);

extern int MPID_nem_ib_com_mem_rdmawr_from(int condesc, void **out);
//extern int MPID_nem_ib_com_mem_rdmawr_to(int condesc, int seq_num, void **out);
extern int MPID_nem_ib_com_mem_udwr_from(int condesc, void **out);
extern int MPID_nem_ib_com_mem_udwr_to(int condesc, void **out);

/* ib_reg_mr.c */
struct MPID_nem_ib_com_reg_mr_listnode_t {
    struct MPID_nem_ib_com_reg_mr_listnode_t *lru_next;
    struct MPID_nem_ib_com_reg_mr_listnode_t *lru_prev;
};

struct MPID_nem_ib_com_reg_mr_cache_entry_t {
    /* : public MPID_nem_ib_com_reg_mr_listnode_t */
    struct MPID_nem_ib_com_reg_mr_listnode_t *lru_next;
    struct MPID_nem_ib_com_reg_mr_listnode_t *lru_prev;
    struct MPID_nem_ib_com_reg_mr_listnode_t g_lru;

    struct ibv_mr *mr;
    void *addr;
    long len;
    int refc;
};
extern int MPID_nem_ib_com_register_cache_init(void);
extern int MPID_nem_ib_com_register_cache_release(void);
extern void *MPID_nem_ib_com_reg_mr_fetch(void *addr, long len,
                                          enum ibv_access_flags additional_flags, int mode);
extern void MPID_nem_ib_com_reg_mr_release(struct MPID_nem_ib_com_reg_mr_cache_entry_t *entry);
#define MPID_NEM_IB_COM_REG_MR_GLOBAL (0)
#define MPID_NEM_IB_COM_REG_MR_STICKY (1)

#define list_entry(ptr, type, member) \
            ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

extern int MPID_nem_ib_com_udbuf_init(void *q);

#define MPID_NEM_IB_COM_RC_SHARED_RCQ 0
#define MPID_NEM_IB_COM_RC_SHARED_SCQ 1
#define MPID_NEM_IB_COM_UD_SHARED_RCQ 2
#define MPID_NEM_IB_COM_UD_SHARED_SCQ 3

/* flag for open */
#define MPID_NEM_IB_COM_OPEN_RC            0x01
/* for MPI control message, eager send, rendezvous protocol,
   so via RC-send/recv or RDMA-write/RDMA-read */

#define MPID_NEM_IB_COM_OPEN_UD       0x02
/* obsolete, to wait for you-to-me QP to become RTR state
   so via UD-send/recv */

#define MPID_NEM_IB_COM_OPEN_SCRATCH_PAD   0x04
/* obsolete, to wait for you-to-me QP to become RTR state
   so via RDMA-write */

#define MPID_nem_ib_segv printf("%d\n", *(int32_t*)0);
#define MPID_NEM_IB_COM_ERR_SETANDJUMP(errno, stmt) { stmt; ibcom_errno = errno; goto fn_fail; }
#define MPID_NEM_IB_COM_ERR_CHKANDJUMP(cond, errno, stmt) if (cond) { stmt; ibcom_errno = errno; goto fn_fail; }
#define MPID_NEM_IB_ERR_FATAL(cond, var, val, tag) if (cond) { var = val; printf("%s\n", tag); MPID_nem_ib_segv; }

#define MPID_NEM_IB_COM_QKEY 0x1234
#define MPID_NEM_IB_COM_MAGIC 0x55

#define MPID_NEM_IB_OFF_POW2_ALIGNED(sz) \
    for(off_pow2_aligned = 15; off_pow2_aligned < (sz); off_pow2_aligned = ((((off_pow2_aligned + 1) << 1) - 1) > MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_netmod_trailer_t)) ? MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_netmod_trailer_t) : (((off_pow2_aligned + 1) << 1) - 1)) { } \
        if (off_pow2_aligned > MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_netmod_trailer_t)) { printf("assertion failed\n"); }; \

#define MPID_NEM_IB_MAX_OFF_POW2_ALIGNED (MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_netmod_trailer_t))

typedef struct MPID_nem_ib_com_qp_state_t {
    uint32_t state;
} MPID_nem_ib_com_qp_state_t;

#define MPID_NEM_IB_COM_QP_STATE_RTR 0x12345678
#define MPID_NEM_IB_COM_SZ_MPI_HEADER 48
#define MPID_NEM_IB_COM_AMT_SLACK (MPID_NEM_IB_COM_RDMABUF_NSEG > 128 ? 1 : 1)

#define MPID_NEM_IB_MAX(a, b) ((a) > (b) ? (a) : (b))

/* Allocator for RDMA write from buffer
   - Allocate performs overflow checks and increments pointer
     - Fast to "malloc" (one load and one store instructions)
   - Free decrements counter at the head of
     aligned memory area. The area is freed when the counter is zero.
     - Fast to "free" (one load and one store instructions)
     - Easy to shrink
   - Refill allocates multiple slots and IB-registers them
     - Fast when first-time allocs occur
   - Free list is pointers for 2^n sizes.
     - Fast to find a empty slot
 */
typedef struct {
    union {
        uint32_t ref_count;
        char *next;
    } first;
    struct ibv_mr *mr;
} MPID_nem_ib_rdmawr_from_alloc_hdr_t;
#define MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA 65536
#define MPID_NEM_IB_RDMAWR_FROM_ALLOC_ROUNDUP64(addr, align) ((addr + align - 1) & ~((unsigned long)align - 1))
#define MPID_NEM_IB_RDMAWR_FROM_ALLOC_ROUNDUP64_ADDR(addr, align) ((char*)(((uint64_t)addr + align - 1) & ~((uint64_t)align - 1)))
#define MPID_NEM_IB_RDMAWR_FROM_ALLOC_NCLUST_SLAB 1
#define MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_START(p) ((void *) ((uint64_t) (p) & ~(MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA - 1)))
#define MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_MR(p) (((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) ((uint64_t) (p) & ~(MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA - 1)))->mr)
#define MPID_NEM_IB_RDMAWR_FROM_ALLOC_PREPROCESS_SZ                     \
    if (_sz < 256) {                                                     \
        clz = 23;                                                       \
        sz = 256;                                                       \
    } else {                                                            \
        clz = __builtin_clz(_sz);                                       \
        int ctz = __builtin_ctz(_sz);                                   \
        if (clz + ctz == 31) {                                          \
            sz = _sz;                                                   \
        } else {                                                        \
            sz = (1ULL << (32 - clz));                                  \
            clz = clz - 1;                                              \
        }                                                               \
    }

static inline void *MPID_nem_ib_rdmawr_from_alloc(uint32_t _sz)
{
    int retval;
    int clz;
    uint32_t sz;
    assert(_sz <= (1ULL << 31));
    MPID_NEM_IB_RDMAWR_FROM_ALLOC_PREPROCESS_SZ;
    char *p = MPID_nem_ib_rdmawr_from_alloc_free_list_front[clz];
    if ((unsigned long) p & (MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA - 1)) {
        MPID_nem_ib_rdmawr_from_alloc_free_list_front[clz] += sz;
        return p;
    }
    else {
        char *q;
        if (MPID_nem_ib_rdmawr_from_alloc_arena_free_list[clz]) {
            q = MPID_nem_ib_rdmawr_from_alloc_arena_free_list[clz];
            MPID_nem_ib_rdmawr_from_alloc_arena_free_list[clz] =
                ((MPID_nem_ib_rdmawr_from_alloc_hdr_t *)
                 MPID_nem_ib_rdmawr_from_alloc_arena_free_list[clz])->first.next;
        }
        else {
            unsigned long sz_clust =
                MPID_NEM_IB_RDMAWR_FROM_ALLOC_ROUNDUP64(MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA *
                                                        MPID_NEM_IB_RDMAWR_FROM_ALLOC_NCLUST_SLAB,
                                                        4096);
            char *unaligned = mmap(NULL,
                                   sz_clust + MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA,
                                   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (unaligned == (void *) -1) {
                printf("mmap failed\n");
                MPID_nem_ib_segv;
            }

            q = MPID_NEM_IB_RDMAWR_FROM_ALLOC_ROUNDUP64_ADDR(unaligned,
                                                             MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA);
            retval = munmap(unaligned, q - unaligned);
            if (q - unaligned != 0 && retval) {
                printf("munmap failed\n");
                MPID_nem_ib_segv;
            }
            retval = munmap(q + sz_clust, MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA - (q - unaligned));
            if (retval) {
                printf("munmap failed\n");
                MPID_nem_ib_segv;
            }

            ((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) q)->mr =
                MPID_nem_ib_com_reg_mr_fetch(q, MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA, 0,
                                             MPID_NEM_IB_COM_REG_MR_STICKY);
            if (!((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) q)->mr) {
                printf("ibv_reg_mr failed\n");
                MPID_nem_ib_segv;
            }

#if MPID_NEM_IB_RDMAWR_FROM_ALLOC_NCLUST_SLAB > 1
            MPID_nem_ib_rdmawr_from_alloc_arena_free_list[clz] =
                q + MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA;
            for (p = q + MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA;
                 p <
                 q + (MPID_NEM_IB_RDMAWR_FROM_ALLOC_NCLUST_SLAB -
                      1) * MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA;
                 p += MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA) {
                ((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) p)->mr =
                    MPID_nem_ib_com_reg_mr_fetch(q, MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA, 0,
                                                 MPID_NEM_IB_COM_REG_MR_STICKY);
                if (!((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) p)->mr) {
                    printf("ibv_reg_mr failed\n");
                    MPID_nem_ib_segv;
                }

                ((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) p)->first.next =
                    p + MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA;
            }
            ((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) p)->first.next = 0;
#endif
        }
        ((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) q)->first.ref_count =
            MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA / sz - 1;
        q += sz + (MPID_NEM_IB_RDMAWR_FROM_ALLOC_SZARENA % sz);
        MPID_nem_ib_rdmawr_from_alloc_free_list_front[clz] = q + sz;
        return q;
    }
}

static inline void MPID_nem_ib_rdmawr_from_free(const void *p, uint32_t _sz)
{
    int clz;
    uint32_t sz;
    assert(_sz <= (1ULL << 31));
    MPID_NEM_IB_RDMAWR_FROM_ALLOC_PREPROCESS_SZ;
    void *q = MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_START(p);
    if (!(--(((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) q)->first.ref_count))) {
        ((MPID_nem_ib_rdmawr_from_alloc_hdr_t *) q)->first.next =
            MPID_nem_ib_rdmawr_from_alloc_arena_free_list[clz];
        MPID_nem_ib_rdmawr_from_alloc_arena_free_list[clz] = (char *) q;
    }
}
