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

extern struct ibv_cq *MPID_nem_ib_rc_shared_scq;
extern struct ibv_cq *MPID_nem_ib_rc_shared_scq_lmt_put;
extern struct ibv_cq *MPID_nem_ib_rc_shared_scq_scratch_pad;
extern struct ibv_cq *MPID_nem_ib_ud_shared_rcq;

#define MPID_NEM_IB_COM_SIZE		2048    /* one process uses 2-4 fds */
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

#define MPID_NEM_IB_COM_NBUF_SCRATCH_PAD 1      /* number of <addr, sz, lkey, rkey> */
#define MPID_NEM_IB_COM_SCRATCH_PAD_TO 0        /* index to RDMA-write-to buffer */

/* send command templates */
#define MPID_NEM_IB_COM_RC_SR_NTEMPLATE (8+1+2) /* number of request templates, 8 for inline-chained-smt, 1 for smt, 1 for lmt */
#define MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 0   /* index to it */
#define MPID_NEM_IB_COM_SMT_INLINE_CHAINED7 7
#define MPID_NEM_IB_COM_SMT_NOINLINE 8
#define MPID_NEM_IB_COM_LMT_INITIATOR 9 /* FIXME: bad naming */

#define MPID_NEM_IB_COM_RC_SR_LMT_PUT_NTEMPLATE MPID_NEM_IB_COM_RC_SR_NTEMPLATE /* FIXME: TEMPLATE named MPID_NEM_IB_COM_RC_SR shares MPID_NEM_IB_COM_LMT_PUT */
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

#define MPID_NEM_IB_COM_UD_SR_NTEMPLATE 1
#define MPID_NEM_IB_COM_UD_RR_NTEMPLATE 1
#define MPID_NEM_IB_COM_UD_INITIATOR 0  /* index to send request template */
#define MPID_NEM_IB_COM_UD_RESPONDER 0  /* index to recv request template */

#define MPID_NEM_IB_COM_SCRATCH_PAD_SR_NTEMPLATE 2
#define MPID_NEM_IB_COM_SCRATCH_PAD_RR_NTEMPLATE 1
#define MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR 0 /* index to send request template */
#define MPID_NEM_IB_COM_SCRATCH_PAD_CAS       1
#define MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER 0 /* index to recv request template */


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
    int sseq_num;
    int rsr_seq_num_poll;
    int rsr_seq_num_tail;       /* occupation status of remote Send Request (SR) queue (it covers occupation status of local RDMA-wr-to buffer) */
    int rsr_seq_num_tail_last_sent;     /* latest one sent to remote rank */
    int lsr_seq_num_tail;       /* occupation status of local Send Request (SR) queue */
    int lsr_seq_num_tail_last_requested;        /* value when lmt_start_send issued req_seq_num */
    int rdmabuf_occupancy_notify_rstate, rdmabuf_occupancy_notify_lstate;
    int ncom, ncom_lmt_put, ncom_scratch_pad;   /* number of entries in the command queue */

    uint32_t max_inline_data;   /* actual value obtained after ibv_create_qp */
    uint32_t max_send_wr;
    uint32_t max_recv_wr;

    uint32_t open_flag;         /* MPID_NEM_IB_COM_OPEN_UD, ... */
    uint16_t remote_lid;        /* for debug */

    /* other commands can be executed before RDMA-rd command */
    /* see the "Ordering and the Fence Indicator" section in "InfiniBand Architecture" by William T. Futral */
    uint16_t after_rdma_rd;

    uint64_t rsr_seq_num_released[(MPID_NEM_IB_COM_RDMABUF_NSEG + 63) / 64];

} MPID_nem_ib_com_t;

extern int MPID_nem_ib_com_open(int ib_port, int MPID_nem_ib_com_open_flag, int *condesc);
extern int MPID_nem_ib_com_alloc(int condesc, int sz);
extern int MPID_nem_ib_com_close(int);
extern int MPID_nem_ib_com_rts(int condesc, int remote_qpnum, uint16_t remote_lid,
                               union ibv_gid *remote_gid);

extern int MPID_nem_ib_com_reg_mr_connect(int condesc, void *rmem, int rkey);
extern int MPID_nem_ib_com_isend(int condesc, uint64_t wr_id, void *prefix, int sz_prefix,
                                 void *hdr, int sz_hdr, void *data, int sz_data, int *copied);
extern int MPID_nem_ib_com_isend_chain(int condesc, uint64_t wr_id, void *hdr, int sz_hdr,
                                       void *data, int sz_data);
extern int MPID_nem_ib_com_put_scratch_pad(int condesc, uint64_t wr_id, uint64_t offset, int sz,
                                           void *laddr);
//extern int MPID_nem_ib_com_isend(int condesc, uint64_t wr_id, void* hdr, int sz_hdr, void* data, int sz_data);
extern int MPID_nem_ib_com_irecv(int condesc, uint64_t wr_id);
extern int MPID_nem_ib_com_udsend(int condesc, union ibv_gid *remote_gid, uint16_t remote_lid,
                                  uint32_t remote_qpn, uint32_t imm_data, uint64_t wr_id);
extern int MPID_nem_ib_com_udrecv(int condesc);
extern int MPID_nem_ib_com_lrecv(int condesc, uint64_t wr_id, void *raddr, int sz_data,
                                 uint32_t rkey, void *laddr);
extern int MPID_nem_ib_com_put_lmt(int condesc, uint64_t wr_id, void *raddr, int sz_data,
                                   uint32_t rkey, void *laddr);
extern int MPID_nem_ib_com_poll_cq(int which_cq, struct ibv_wc *wc, int *result);

extern int MPID_nem_ib_com_obtain_pointer(int condesc, MPID_nem_ib_com_t ** MPID_nem_ib_com);

/* for ib_reg_mr.c */
extern int MPID_nem_ib_com_reg_mr(void *addr, int len, struct ibv_mr **mr);
extern int MPID_nem_ib_com_dereg_mr(struct ibv_mr *mr);

extern int MPID_nem_ib_com_get_info_conn(int condesc, int key, void *out, uint32_t out_len);
extern int MPID_nem_ib_com_get_info_mr(int condesc, int memid, int key, void *out, int out_len);

extern int MPID_nem_ib_com_sseq_num_get(int condesc, int *seq_num);
extern int MPID_nem_ib_com_lsr_seq_num_tail_get(int condesc, int **seq_num);
extern int MPID_nem_ib_com_rsr_seq_num_tail_get(int condesc, int **seq_num);
extern int MPID_nem_ib_com_rsr_seq_num_tail_last_sent_get(int condesc, int **seq_num);
extern int MPID_nem_ib_com_rdmabuf_occupancy_notify_rate_get(int condesc, int *notify_rate);
extern int MPID_nem_ib_com_rdmabuf_occupancy_notify_rstate_get(int condesc, int **rstate);
extern int MPID_nem_ib_com_rdmabuf_occupancy_notify_lstate_get(int condesc, int **lstate);

extern char *MPID_nem_ib_com_strerror(int errno);

extern int MPID_nem_ib_com_mem_rdmawr_from(int condesc, void **out);
extern int MPID_nem_ib_com_mem_rdmawr_to(int condesc, int seq_num, void **out);
extern int MPID_nem_ib_com_mem_udwr_from(int condesc, void **out);
extern int MPID_nem_ib_com_mem_udwr_to(int condesc, void **out);

/* ib_reg_mr.c */
extern void MPID_nem_ib_com_register_cache_init(void);
extern void MPID_nem_ib_com_register_cache_destroy(void);
extern struct ibv_mr *MPID_nem_ib_com_reg_mr_fetch(void *addr, int len);

extern int MPID_nem_ib_com_udbuf_init(void *q);

#define MPID_NEM_IB_COM_RC_SHARED_RCQ 0
#define MPID_NEM_IB_COM_RC_SHARED_SCQ 1
#define MPID_NEM_IB_COM_UD_SHARED_RCQ 2
#define MPID_NEM_IB_COM_UD_SHARED_SCQ 3
#define MPID_NEM_IB_COM_RC_SHARED_SCQ_LMT_PUT 4

/* flag for open */
#define MPID_NEM_IB_COM_OPEN_RC            0x01
/* for MPI control message, eager send, rendezvous protocol,
   so via RC-send/recv or RDMA-write/RDMA-read */

#define MPID_NEM_IB_COM_OPEN_UD       0x02
/* obsolete, to wait for you-to-me QP to become RTR state
   so via UD-send/recv */

#define MPID_NEM_IB_COM_OPEN_RC_LMT_PUT       0x03
/* obsolete, tried to use different CQ for LMT-PUT protocol for speed */

#define MPID_NEM_IB_COM_OPEN_SCRATCH_PAD   0x04
/* obsolete, to wait for you-to-me QP to become RTR state
   so via RDMA-write */

#define MPID_NEM_IB_COM_ERR_SETANDJUMP(errno, stmt) { stmt; ibcom_errno = errno; goto fn_fail; }
#define MPID_NEM_IB_COM_ERR_CHKANDJUMP(cond, errno, stmt) if (cond) { stmt; ibcom_errno = errno; goto fn_fail; }

#define MPID_NEM_IB_COM_QKEY 0x1234
#define MPID_NEM_IB_COM_MAGIC 0x55

typedef struct MPID_nem_ib_sz_hdrmagic_t {
    uint32_t sz;
    uint32_t magic;
} MPID_nem_ib_sz_hdrmagic_t;


typedef struct MPID_nem_ib_tailmagic_t {
    uint8_t magic;
    //uint32_t traits; /* for debug */
} MPID_nem_ib_tailmagic_t;

#define MPID_NEM_IB_SZ_DATA_POW2(sz) \
    for(sz_data_pow2 = 15; sz_data_pow2 < (sz); sz_data_pow2 = ((((sz_data_pow2 + 1) << 1) - 1) > MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_tailmagic_t)) ? MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_tailmagic_t) : (((sz_data_pow2 + 1) << 1) - 1)) { } \
        if (sz_data_pow2 > MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_tailmagic_t)) { printf("assertion failed\n"); }; \

#define MPID_NEM_IB_MAX_DATA_POW2 (MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_tailmagic_t))

typedef struct MPID_nem_ib_com_qp_state_t {
    uint32_t state;
} MPID_nem_ib_com_qp_state_t;

#define MPID_NEM_IB_COM_QP_STATE_RTR 0x12345678
#define MPID_NEM_IB_COM_SZ_MPI_HEADER 48
#define MPID_NEM_IB_COM_AMT_SLACK (MPID_NEM_IB_COM_RDMABUF_NSEG > 128 ? 1 : 1)
