/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 NEC Corporation
 *      Author: Masamichi Takagi
 *  (C) 2012 Oct 14 Yutaka Ishikawa, ishikawa@is.s.u-tokyo.ac.jp
 *      See COPYRIGHT in top-level directory.
 */

/*
 * TODO:
 *	- MPID_nem_ib_com_clean might not clean all allocated memory area. Need to FIX it.
 *	- During error processing in each function, some memory area might not
 *	  be deallocated. Look at all functions.
 */
#include "ib_ibcom.h"
//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <sys/types.h>
#include <assert.h>
#include <linux/mman.h> /* make it define MAP_ANONYMOUS */
#include <sys/mman.h>

//#define MPID_NEM_IB_DEBUG_IBCOM
#ifdef dprintf  /* avoid redefinition with src/mpid/ch3/include/mpidimpl.h */
#undef dprintf
#endif
#ifdef MPID_NEM_IB_DEBUG_IBCOM
#define dprintf printf
#else
#define dprintf(...)
#endif

static MPID_nem_ib_com_t contab[MPID_NEM_IB_COM_SIZE];
static int ib_initialized = 0;
static int maxcon;
static struct ibv_device **ib_devlist;
static struct ibv_context *ib_ctx;
struct ibv_context *MPID_nem_ib_ctx_export;     /* for SC13 demo connector */
static struct ibv_pd *ib_pd;
struct ibv_pd *MPID_nem_ib_pd_export;   /* for SC13 demo connector */
struct ibv_cq *MPID_nem_ib_rc_shared_scq;
static int MPID_nem_ib_rc_shared_scq_ref_count;
struct ibv_cq *MPID_nem_ib_rc_shared_scq_scratch_pad;
static int MPID_nem_ib_rc_shared_scq_scratch_pad_ref_count;
static struct ibv_cq *MPID_nem_ib_ud_shared_scq;
static int MPID_nem_ib_ud_shared_scq_ref_count;
static struct ibv_cq *MPID_nem_ib_rc_shared_rcq;
static int MPID_nem_ib_rc_shared_rcq_ref_count;
struct ibv_cq *MPID_nem_ib_rc_shared_rcq_scratch_pad;
static int MPID_nem_ib_rc_shared_rcq_scratch_pad_ref_count;
struct ibv_cq *MPID_nem_ib_ud_shared_rcq;
static int MPID_nem_ib_ud_shared_rcq_ref_count;
uint8_t *MPID_nem_ib_scratch_pad = 0;
int MPID_nem_ib_scratch_pad_ref_count;
char *MPID_nem_ib_rdmawr_from_alloc_free_list_front[MPID_NEM_IB_RDMAWR_FROM_ALLOC_NID] = { 0 };
char *MPID_nem_ib_rdmawr_from_alloc_arena_free_list[MPID_NEM_IB_RDMAWR_FROM_ALLOC_NID] = { 0 };

struct ibv_mr *MPID_nem_ib_rdmawr_to_alloc_mr;
uint8_t *MPID_nem_ib_rdmawr_to_alloc_start;
uint8_t *MPID_nem_ib_rdmawr_to_alloc_free_list;

#define MPID_NEM_IB_RANGE_CHECK(condesc, conp)			\
{							\
    if (condesc < 0 || condesc >= MPID_NEM_IB_COM_SIZE) return;	\
    conp = &contab[condesc];				\
    if (conp->icom_used != 1) return;			\
}

#define MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp)		\
{							\
    if (condesc < 0 || condesc >= MPID_NEM_IB_COM_SIZE) {		\
        dprintf("condesc=%d\n", condesc);		\
        MPID_nem_ib_segv;				\
        return -1;					\
    }							\
    conp = &contab[condesc];				\
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(conp->icom_used != 1, -1, dprintf("MPID_NEM_IB_RANGE_CHECK_WITH_ERROR,conp->icom_used=%d\n", conp->icom_used));	\
}

/* Allocator for RDMA write to buffer
   - Allocate performs dequeue
     - Slow to "malloc" (two load and one store instructions)
   - Free performs enqueue
     - Slow to "free" (one load and two store instructions)
     - No flagmentation occurs
     - munmap unit is small (4KB)
     - Less header when compared to reference count
   - Refill never happens because IB-registers whole pool at the beginning
     - Fast when first-time allocs occur
   - Free list is a linked list
     - Fast to find a empty slot (one load instruction)
 */
static int MPID_nem_ib_rdmawr_to_init(uint64_t sz)
{
    int ibcom_errno = 0;
    void *start;
    void *cur;
    start = (void *) mmap(0, sz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(start == (void *) -1, -1, printf("mmap failed\n"));
    dprintf("rdmawr_to_init,sz=%ld,start=%p\n", sz, start);

    memset(start, 0, sz);

    MPID_nem_ib_rdmawr_to_alloc_mr =
        MPID_nem_ib_com_reg_mr_fetch(start, sz, 0, MPID_NEM_IB_COM_REG_MR_STICKY);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!MPID_nem_ib_rdmawr_to_alloc_mr, -1,
                                   printf("MPID_nem_ib_com_reg_mr_fetchibv_reg_mr failed\n"));
    dprintf("rdmawr_to_init,rkey=%08x\n", MPID_nem_ib_rdmawr_to_alloc_mr->rkey);

    MPID_nem_ib_rdmawr_to_alloc_start = start;
    MPID_nem_ib_rdmawr_to_alloc_free_list = start;
    for (cur = start;
         cur < (void *) ((uint8_t *) start + sz - MPID_NEM_IB_COM_RDMABUF_SZSEG);
         cur = (uint8_t *) cur + MPID_NEM_IB_COM_RDMABUF_SZSEG) {
        //dprintf("rdmawr_to_init,cur=%p\n", cur);
        ((MPID_nem_ib_rdmawr_to_alloc_hdr_t *) cur)->next =
            (uint8_t *) cur + MPID_NEM_IB_COM_RDMABUF_SZSEG;
    }
    ((MPID_nem_ib_rdmawr_to_alloc_hdr_t *) cur)->next = 0;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

void *MPID_nem_ib_rdmawr_to_alloc(int nslots)
{
    dprintf("rdmawr_to_alloc,nslots=%d\n", nslots);
    void *start;
    int i;
    for (i = 0; i < nslots; i++) {
        //dprintf("MPID_nem_ib_rdmawr_to_alloc,free_list=%p\n", MPID_nem_ib_rdmawr_to_alloc_free_list);
        if (MPID_nem_ib_rdmawr_to_alloc_free_list) {
            if (i == 0) {
                start = MPID_nem_ib_rdmawr_to_alloc_free_list;
            }
            MPID_nem_ib_rdmawr_to_alloc_free_list =
                ((MPID_nem_ib_rdmawr_to_alloc_hdr_t *) MPID_nem_ib_rdmawr_to_alloc_free_list)->next;
        }
        else {
            printf("out of rdmawr_to bufer\n");
            return 0;
        }
    }
    return start;
}

void MPID_nem_ib_rdmawr_to_free(void *p, int nslots)
{
    void *q;
    ((MPID_nem_ib_rdmawr_to_alloc_hdr_t *)
     ((uint8_t *) p + MPID_NEM_IB_COM_RDMABUF_SZSEG * (nslots - 1)))->next =
        MPID_nem_ib_rdmawr_to_alloc_free_list;
    for (q = (uint8_t *) p + MPID_NEM_IB_COM_RDMABUF_SZSEG * (nslots - 2);
         q >= p; q = (uint8_t *) q - MPID_NEM_IB_COM_RDMABUF_SZSEG) {
        ((MPID_nem_ib_rdmawr_to_alloc_hdr_t *) q)->next =
            (uint8_t *) q + MPID_NEM_IB_COM_RDMABUF_SZSEG;
    }
    MPID_nem_ib_rdmawr_to_alloc_free_list = p;
}

int MPID_nem_ib_rdmawr_to_munmap(void *p, int nslots)
{
    int retval;
    int ibcom_errno = 0;
    retval = munmap(p, MPID_NEM_IB_COM_RDMABUF_SZSEG * nslots);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(retval, -1, printf("munmap failed\n"));
  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

static int modify_qp_to_init(struct ibv_qp *qp, int ib_port, int additional_flags)
{
    struct ibv_qp_attr attr;
    int flags;
    int rc;

    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = ib_port;
    attr.pkey_index = 0;
    attr.qp_access_flags =
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
        IBV_ACCESS_REMOTE_WRITE | additional_flags;
    flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    rc = ibv_modify_qp(qp, &attr, flags);
    if (rc) {
        fprintf(stderr, "failed to modify QP state to INIT\n");
    }
    return rc;
}

static int modify_qp_to_rtr(struct ibv_qp *qp, uint32_t remote_qpn, uint16_t dlid,
                            union ibv_gid *dgid, int ib_port, int gid_idx)
{
    struct ibv_qp_attr attr;
    int flags;
    int rc;

    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_2048;
    //attr.path_mtu = IBV_MTU_1024;
    //attr.path_mtu = IBV_MTU_256; /* DCFA */
    attr.dest_qp_num = remote_qpn;
    attr.rq_psn = 0;
    attr.max_dest_rd_atomic = MPID_NEM_IB_COM_MAX_RD_ATOMIC;
    //attr.max_dest_rd_atomic = 1;
    //attr.max_dest_rd_atomic = 0; /* DCFA */

    /* Default is 0x12 (= 5.12ms) see IB Spec. Rel. 1.2, Vol. 1, 9.7.5.2.8 */
    attr.min_rnr_timer = 0x12;

    attr.ah_attr.dlid = dlid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.is_global = 0;
    attr.ah_attr.port_num = ib_port;

    /* In dcfa gid is not set and for testing here it is also not set */
#if 1
#ifdef HAVE_LIBDCFA     /* DCFA doesn't use gid */
#else
    if (gid_idx >= 0) {
        attr.ah_attr.is_global = 1;
        attr.ah_attr.port_num = ib_port;
        memcpy(&attr.ah_attr.grh.dgid, dgid, 16);
        attr.ah_attr.grh.flow_label = 0;
        attr.ah_attr.grh.hop_limit = 1;
        attr.ah_attr.grh.sgid_index = gid_idx;
        attr.ah_attr.grh.traffic_class = 0;
    }
#endif
#endif

    flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN
        | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    rc = ibv_modify_qp(qp, &attr, flags);
    if (rc) {
        dprintf("failed to modify QP state to RTR\n");
    }
    return rc;
}

static int modify_qp_to_rts(struct ibv_qp *qp)
{
    struct ibv_qp_attr attr;
    int flags;
    int rc;

    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = (0x14);      /* timeout 4.096us * 2^x */
    attr.retry_cnt = 7;
    attr.rnr_retry = 7;
    attr.sq_psn = 0;
    attr.max_rd_atomic = MPID_NEM_IB_COM_MAX_RD_ATOMIC;
    //attr.max_rd_atomic = 1;
    //attr.max_rd_atomic = 0;   /* DCFA */

    flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT
        | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    rc = ibv_modify_qp(qp, &attr, flags);
    if (rc) {
        fprintf(stderr, "failed to modify QP state to RTS\n");
    }
    return rc;
}

/* called from MPID_nem_ib_com_open if needed */
static int MPID_nem_ib_com_device_init()
{
    int ibcom_errno = 0;
    int dev_num;
    char *dev_name;
    int i;

    if (ib_initialized == 1) {
        dprintf("MPID_nem_ib_com_device_init,already initialized\n");
        return 0;
    }
    if (ib_initialized == -1)
        return -1;

    /* Get the device list */
    ib_devlist = ibv_get_device_list(&dev_num);
    if (!ib_devlist || !dev_num) {
        fprintf(stderr, "No IB device is found\n");
        return -1;
    }

#ifdef HAVE_LIBDCFA
    for (i = 0; i < dev_num; i++) {
        if (ib_devlist[i]) {
            goto dev_found;
        }
    }
#else
    for (i = 0; i < dev_num; i++) {
        if (!strcmp(ibv_get_device_name(ib_devlist[i]), "mlx4_0") ||
            !strcmp(ibv_get_device_name(ib_devlist[i]), "mlx5_0") ||
            !strcmp(ibv_get_device_name(ib_devlist[i]), "qib0")) {
            goto dev_found;
        }
    }
#endif
    MPID_NEM_IB_COM_ERR_SETANDJUMP(-1, printf("IB device not found"));
  dev_found:

    /* Open the requested device */
    if (MPID_nem_ib_ctx_export) {
        ib_ctx = MPID_nem_ib_ctx_export;
    }
    else {
        ib_ctx = ibv_open_device(ib_devlist[i]);
    }
    dprintf("MPID_nem_ib_com_device_init,MPID_nem_ib_ctx_export=%p,ib_ctx=%p\n",
            MPID_nem_ib_ctx_export, ib_ctx);
    if (!ib_ctx) {
        fprintf(stderr, "failed to open IB device\n");
        goto err_exit;
    }
    MPID_nem_ib_ctx_export = ib_ctx;
#ifdef HAVE_LIBDCFA
#else
    dev_name = MPIU_Strdup(ibv_get_device_name(ib_devlist[i]));
    dprintf("MPID_nem_ib_com_device_init,dev_name=%s\n", dev_name);
    MPIU_Free(dev_name);
#endif
    /* Create a PD */
    if (MPID_nem_ib_pd_export) {
        ib_pd = MPID_nem_ib_pd_export;
    }
    else {
        ib_pd = ibv_alloc_pd(ib_ctx);
    }
    dprintf("MPID_nem_ib_com_device_init,MPID_nem_ib_pd_export=%p,ib_pd=%p\n",
            MPID_nem_ib_pd_export, ib_pd);
    if (!ib_pd) {
        fprintf(stderr, "ibv_alloc_pd failed\n");
        goto err_exit;
    }
    MPID_nem_ib_pd_export = ib_pd;

    ib_initialized = 1;
  fn_exit:
    return ibcom_errno;

  err_exit:
    ib_initialized = -1;
    if (ib_devlist)
        ibv_free_device_list(ib_devlist);
    if (ib_ctx)
        ibv_close_device(ib_ctx);
    return -1;
  fn_fail:
    goto fn_exit;
}

static int MPID_nem_ib_com_clean(MPID_nem_ib_com_t * conp)
{
    int i;
    int ibcom_errno = 0;
    int ib_errno;
    int retval;

    if (conp->icom_qp) {
        ibv_destroy_qp(conp->icom_qp);
        conp->icom_qp = NULL;
    }
    if (conp->icom_mrlist && conp->icom_mrlen > 0) {
        switch (conp->open_flag) {
        case MPID_NEM_IB_COM_OPEN_RC:
            MPIU_Assert(MPID_nem_ib_rc_shared_scq_ref_count > 0);
            if (--MPID_nem_ib_rc_shared_scq_ref_count == 0) {
                dprintf("ibcom,destroy MPID_nem_ib_rc_shared_scq\n");
                ib_errno = ibv_destroy_cq(MPID_nem_ib_rc_shared_scq);
                MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, dprintf("ibv_destroy_cq failed\n"));

                /* Tell drain_scq that CQ is destroyed because
                 * drain_scq is called after poll_eager calls vc_terminate */
                MPID_nem_ib_rc_shared_scq = NULL;
            }
            MPIU_Assert(MPID_nem_ib_rc_shared_rcq_ref_count > 0);
            if (--MPID_nem_ib_rc_shared_rcq_ref_count == 0) {
                dprintf("ibcom,destroy MPID_nem_ib_rc_shared_rcq\n");
                ib_errno = ibv_destroy_cq(MPID_nem_ib_rc_shared_rcq);
                MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, dprintf("ibv_destroy_cq failed\n"));

                MPID_nem_ib_rc_shared_rcq = NULL;
            }
#if 0   /* It's not used */
            retval =
                munmap(conp->icom_mem[MPID_NEM_IB_COM_RDMAWR_FROM], MPID_NEM_IB_COM_RDMABUF_SZ);
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(retval, -1, dprintf("munmap"));
#endif
#if 0   /* Don't free it because it's managed through VC_FILED(vc, ibcom->remote_ringbuf) */
            retval = munmap(conp->icom_mem[MPID_NEM_IB_COM_RDMAWR_TO], MPID_NEM_IB_COM_RDMABUF_SZ);
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(retval, -1, dprintf("munmap"));
#endif
            MPIU_Free(conp->icom_mrlist);
            MPIU_Free(conp->icom_mem);
            MPIU_Free(conp->icom_msize);

            MPIU_Free(conp->icom_rmem);
            MPIU_Free(conp->icom_rsize);
            MPIU_Free(conp->icom_rkey);
            for (i = 0; i < MPID_NEM_IB_COM_SMT_INLINE_NCHAIN; i++) {
#ifndef HAVE_LIBDCFA
                MPIU_Free(conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list);
#endif
            }
            MPIU_Free(conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list);
            MPIU_Free(conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].sg_list);
            MPIU_Free(conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].sg_list);
            MPIU_Free(conp->icom_sr);
            MPIU_Free(conp->icom_rr);
            break;
        case MPID_NEM_IB_COM_OPEN_SCRATCH_PAD:
            MPIU_Assert(MPID_nem_ib_rc_shared_scq_scratch_pad_ref_count > 0);
            if (--MPID_nem_ib_rc_shared_scq_scratch_pad_ref_count == 0) {
                ib_errno = ibv_destroy_cq(MPID_nem_ib_rc_shared_scq_scratch_pad);
                MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, dprintf("ibv_destroy_cq failed\n"));
                /* Tell drain_scq that CQ is destroyed because
                 * drain_scq is called after poll_eager calls vc_terminate */
                MPID_nem_ib_rc_shared_scq_scratch_pad = NULL;
            }
            MPIU_Assert(MPID_nem_ib_rc_shared_rcq_scratch_pad_ref_count > 0);
            if (--MPID_nem_ib_rc_shared_rcq_scratch_pad_ref_count == 0) {
                ib_errno = ibv_destroy_cq(MPID_nem_ib_rc_shared_rcq_scratch_pad);
                MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, dprintf("ibv_destroy_cq failed\n"));
            }
            retval = munmap(conp->icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_FROM],
                            MPID_NEM_IB_COM_SCRATCH_PAD_FROM_SZ);
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(retval, -1, dprintf("munmap"));

            MPIU_Free(conp->icom_mrlist);
            MPIU_Free(conp->icom_mem);
            MPIU_Free(conp->icom_msize);

            MPIU_Free(conp->icom_rmem);
            MPIU_Free(conp->icom_rsize);
            MPIU_Free(conp->icom_rkey);

#ifndef HAVE_LIBDCFA
            MPIU_Free(conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].sg_list);
            MPIU_Free(conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].sg_list);
            MPIU_Free(conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].sg_list);
            MPIU_Free(conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].sg_list);
#endif
            MPIU_Free(conp->icom_sr);
#ifndef HAVE_LIBDCFA
            MPIU_Free(conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].sg_list);
#endif
            MPIU_Free(conp->icom_rr);
            break;
        case MPID_NEM_IB_COM_OPEN_UD:
            MPIU_Assert(MPID_nem_ib_ud_shared_scq_ref_count > 0);
            if (--MPID_nem_ib_ud_shared_scq_ref_count == 0) {
                ib_errno = ibv_destroy_cq(MPID_nem_ib_ud_shared_scq);
                MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, dprintf("ibv_destroy_cq"));
                /* Tell drain_scq that CQ is destroyed because
                 * drain_scq is called after poll_eager calls vc_terminate */
                MPID_nem_ib_ud_shared_scq = NULL;
            }
            MPIU_Assert(MPID_nem_ib_ud_shared_rcq_ref_count > 0);
            if (--MPID_nem_ib_ud_shared_rcq_ref_count == 0) {
                ib_errno = ibv_destroy_cq(MPID_nem_ib_ud_shared_rcq);
                MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, dprintf("ibv_destroy_cq"));
            }
            retval = munmap(conp->icom_mem[MPID_NEM_IB_COM_UDWR_FROM], MPID_NEM_IB_COM_UDBUF_SZ);
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(retval, -1, dprintf("munmap"));
            retval = munmap(conp->icom_mem[MPID_NEM_IB_COM_UDWR_TO], MPID_NEM_IB_COM_UDBUF_SZ);
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(retval, -1, dprintf("munmap"));

            MPIU_Free(conp->icom_mrlist);
            MPIU_Free(conp->icom_mem);
            MPIU_Free(conp->icom_msize);

            MPIU_Free(conp->icom_ah_attr);
#ifndef HAVE_LIBDCFA
            MPIU_Free(conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].sg_list);
#endif
            MPIU_Free(conp->icom_sr);

            MPIU_Free(conp->icom_rr[MPID_NEM_IB_COM_UD_RESPONDER].sg_list);
            MPIU_Free(conp->icom_rr);
            break;
        }
    }
    memset(conp, 0, sizeof(MPID_nem_ib_com_t));

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_open(int ib_port, int open_flag, int *condesc)
{
    int ibcom_errno = 0, ib_errno;
    MPID_nem_ib_com_t *conp;
    struct ibv_qp_init_attr qp_init_attr;
    struct ibv_sge *sge;
    int mr_flags;
    int i;

    dprintf("MPID_nem_ib_com_open,port=%d,flag=%08x\n", ib_port, open_flag);

    int open_flag_conn = open_flag;
    if (open_flag_conn != MPID_NEM_IB_COM_OPEN_RC &&
        open_flag_conn != MPID_NEM_IB_COM_OPEN_UD &&
        open_flag_conn != MPID_NEM_IB_COM_OPEN_SCRATCH_PAD) {
        dprintf("MPID_nem_ib_com_open,bad flag\n");
        ibcom_errno = -1;
        goto fn_fail;
    }

    /* Increment reference counter of ibv_reg_mr cache */
    ibcom_errno = MPID_nem_ib_com_register_cache_init();
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ibcom_errno, -1, dprintf("MPID_nem_ib_com_register_cache_init"));

    /* device open error */
    if (MPID_nem_ib_com_device_init() < 0) {
        ibcom_errno = -1;
        goto fn_fail;
    }

    /* no more connection can be estabilished */
    if (maxcon == MPID_NEM_IB_COM_SIZE) {
        ibcom_errno = -1;
        goto fn_fail;
    }

    for (*condesc = 0; *condesc < MPID_NEM_IB_COM_SIZE; (*condesc)++) {
        //dprintf("*condesc=%d,used=%d\n", *condesc, contab[*condesc].icom_used);
        if (contab[*condesc].icom_used == 0) {
            goto ok_cont;
        }
    }
    /* count says not full, but we couldn't fine vacant slot */
    dprintf("contable has inconsistent\n");
    ibcom_errno = -1;
    goto fn_fail;

  ok_cont:
    dprintf("MPID_nem_ib_com_open,condesc=%d\n", *condesc);
    conp = &contab[*condesc];
    memset(conp, 0, sizeof(MPID_nem_ib_com_t));
    conp->icom_used = 1;
    conp->icom_port = ib_port;
    conp->open_flag = open_flag;
    conp->rsr_seq_num_poll = 0; /* it means slot 0 is polled */
    conp->rsr_seq_num_tail = -1;        /* it means slot 0 is not released */
    conp->rsr_seq_num_tail_last_sent = -1;
    conp->lsr_seq_num_tail_last_requested = -2;
    conp->rdmabuf_occupancy_notify_rstate = MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_LW;
    conp->rdmabuf_occupancy_notify_lstate = MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_LW;
    conp->ask_guard = 0;
    //dprintf("MPID_nem_ib_com_open,ptr=%p,rsr_seq_num_poll=%d\n", conp, conp->rsr_seq_num_poll);

#ifdef HAVE_LIBDCFA
#else
    if (ibv_query_port(ib_ctx, ib_port, &conp->icom_pattr)) {
        dprintf("ibv_query_port on port %u failed\n", ib_port);
        goto err_exit;
    }
#endif

    /* Create send/recv CQ */
    switch (open_flag) {
    case MPID_NEM_IB_COM_OPEN_RC:
        MPID_nem_ib_rc_shared_scq_ref_count++;
        if (!MPID_nem_ib_rc_shared_scq) {
#ifdef HAVE_LIBDCFA
            MPID_nem_ib_rc_shared_scq = ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY);
#else
            MPID_nem_ib_rc_shared_scq =
                ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY, NULL, NULL, 0);
#endif
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(!MPID_nem_ib_rc_shared_scq, -1,
                                           dprintf("MPID_nem_ib_rc_shared_scq"));
        }
        conp->icom_scq = MPID_nem_ib_rc_shared_scq;

        MPID_nem_ib_rc_shared_rcq_ref_count++;
        if (!MPID_nem_ib_rc_shared_rcq) {
#ifdef HAVE_LIBDCFA
            MPID_nem_ib_rc_shared_rcq = ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY);
#else
            MPID_nem_ib_rc_shared_rcq =
                ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY, NULL, NULL, 0);
#endif
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(!MPID_nem_ib_rc_shared_rcq, -1,
                                           dprintf("MPID_nem_ib_rc_shared_rcq"));
        }
        conp->icom_rcq = MPID_nem_ib_rc_shared_rcq;
        break;
    case MPID_NEM_IB_COM_OPEN_SCRATCH_PAD:
        MPID_nem_ib_rc_shared_scq_scratch_pad_ref_count++;
        if (!MPID_nem_ib_rc_shared_scq_scratch_pad) {
#ifdef HAVE_LIBDCFA
            MPID_nem_ib_rc_shared_scq_scratch_pad =
                ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY);
#else
            MPID_nem_ib_rc_shared_scq_scratch_pad =
                ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY, NULL, NULL, 0);
#endif
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(!MPID_nem_ib_rc_shared_scq_scratch_pad, -1,
                                           dprintf("MPID_nem_ib_rc_shared_scq"));
        }
        conp->icom_scq = MPID_nem_ib_rc_shared_scq_scratch_pad;

        MPID_nem_ib_rc_shared_rcq_scratch_pad_ref_count++;
        if (!MPID_nem_ib_rc_shared_rcq_scratch_pad) {
#ifdef HAVE_LIBDCFA
            MPID_nem_ib_rc_shared_rcq_scratch_pad =
                ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY);
#else
            MPID_nem_ib_rc_shared_rcq_scratch_pad =
                ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY, NULL, NULL, 0);
#endif
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(!MPID_nem_ib_rc_shared_rcq_scratch_pad, -1,
                                           dprintf("MPID_nem_ib_rc_shared_rcq"));
        }
        conp->icom_rcq = MPID_nem_ib_rc_shared_rcq_scratch_pad;
        break;
    case MPID_NEM_IB_COM_OPEN_UD:
        MPID_nem_ib_ud_shared_scq_ref_count++;
        if (!MPID_nem_ib_ud_shared_scq) {
#ifdef HAVE_LIBDCFA
            MPID_nem_ib_ud_shared_scq = ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY);
#else
            MPID_nem_ib_ud_shared_scq =
                ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY, NULL, NULL, 0);
#endif
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(!MPID_nem_ib_ud_shared_scq, -1,
                                           dprintf("MPID_nem_ib_ud_shared_scq"));
        }
        conp->icom_scq = MPID_nem_ib_ud_shared_scq;

        MPID_nem_ib_ud_shared_rcq_ref_count++;
        if (!MPID_nem_ib_ud_shared_rcq) {
#ifdef HAVE_LIBDCFA
            MPID_nem_ib_ud_shared_rcq = ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY);
#else
            MPID_nem_ib_ud_shared_rcq =
                ibv_create_cq(ib_ctx, MPID_NEM_IB_COM_MAX_CQ_CAPACITY, NULL, NULL, 0);
#endif
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(!MPID_nem_ib_ud_shared_rcq, -1,
                                           dprintf("MPID_nem_ib_ud_shared_rcq"));
        }
        conp->icom_rcq = MPID_nem_ib_ud_shared_rcq;
        break;
    }

    /* Create QP */
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    qp_init_attr.send_cq = conp->icom_scq;
    qp_init_attr.recv_cq = conp->icom_rcq;
    qp_init_attr.cap.max_send_wr = MPID_NEM_IB_COM_MAX_SQ_CAPACITY;
    qp_init_attr.cap.max_recv_wr = MPID_NEM_IB_COM_MAX_RQ_CAPACITY;
    qp_init_attr.cap.max_send_sge = MPID_NEM_IB_COM_MAX_SGE_CAPACITY;
    qp_init_attr.cap.max_recv_sge = MPID_NEM_IB_COM_MAX_SGE_CAPACITY;
    qp_init_attr.cap.max_inline_data = MPID_NEM_IB_COM_INLINE_DATA;
    switch (open_flag) {
    case MPID_NEM_IB_COM_OPEN_RC:
    case MPID_NEM_IB_COM_OPEN_SCRATCH_PAD:
        qp_init_attr.qp_type = IBV_QPT_RC;
        break;
    case MPID_NEM_IB_COM_OPEN_UD:
        qp_init_attr.qp_type = IBV_QPT_UD;
        break;
    default:
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(1, -1, dprintf("invalid open_flag\n"));
        break;
    }
    qp_init_attr.sq_sig_all = 1;

    conp->icom_qp = ibv_create_qp(ib_pd, &qp_init_attr);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!conp->icom_qp, -1, printf("ibv_create_qp\n"));

    conp->max_send_wr = qp_init_attr.cap.max_send_wr;
    conp->max_recv_wr = qp_init_attr.cap.max_recv_wr;
    conp->max_inline_data = qp_init_attr.cap.max_inline_data;

    dprintf("MPID_nem_ib_com_open,max_send_wr=%d,max_recv_wr=%d,max_inline_data=%d\n",
            qp_init_attr.cap.max_send_wr, qp_init_attr.cap.max_recv_wr,
            qp_init_attr.cap.max_inline_data);
    dprintf("MPID_nem_ib_com_open,fd=%d,qpn=%08x\n", *condesc, conp->icom_qp->qp_num);
#ifdef HAVE_LIBDCFA
    dprintf("MPID_nem_ib_com_open,fd=%d,lid=%04x\n", *condesc, ib_ctx->lid);
#else
    dprintf("MPID_nem_ib_com_open,fd=%d,lid=%04x\n", *condesc, conp->icom_pattr.lid);
#endif

#ifdef HAVE_LIBDCFA
    /* DCFA doesn't use gid */
    for (i = 0; i < 16; i++) {
        conp->icom_gid.raw[i] = 0;
    }
#else
    ib_errno = ibv_query_gid(ib_ctx, ib_port, 0, &conp->icom_gid);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, dprintf("ibv_query_gid\n"));

    dprintf("MPID_nem_ib_com_open,fd=%d,my_gid=", *condesc);
    for (i = 0; i < 16; i++) {
        dprintf("%02x", (int) conp->icom_gid.raw[i]);
    }
    dprintf("\n");
#endif

    /* buffers */
    switch (open_flag) {
    case MPID_NEM_IB_COM_OPEN_RC:
        /* RDMA-write-from and -to local memory area */
        conp->icom_mrlist = MPIU_Malloc(sizeof(struct ibv_mr *) * MPID_NEM_IB_COM_NBUF_RDMA);
        memset(conp->icom_mrlist, 0, sizeof(struct ibv_mr *) * MPID_NEM_IB_COM_NBUF_RDMA);
        conp->icom_mrlen = MPID_NEM_IB_COM_NBUF_RDMA;
        conp->icom_mem = (void **) MPIU_Malloc(sizeof(void **) * MPID_NEM_IB_COM_NBUF_RDMA);
        //printf("open,icom_mem=%p\n", conp->icom_mem);
        memset(conp->icom_mem, 0, sizeof(void **) * MPID_NEM_IB_COM_NBUF_RDMA);
        conp->icom_msize = (int *) MPIU_Malloc(sizeof(int *) * MPID_NEM_IB_COM_NBUF_RDMA);
        memset(conp->icom_msize, 0, sizeof(int *) * MPID_NEM_IB_COM_NBUF_RDMA);
        mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

        /* RDMA-write-to local memory area */
        conp->icom_msize[MPID_NEM_IB_COM_RDMAWR_TO] = MPID_NEM_IB_COM_RDMABUF_SZ;
#if 0
        int shmid = shmget(2, MPID_NEM_IB_COM_RDMABUF_SZ, SHM_HUGETLB | IPC_CREAT | SHM_R | SHM_W);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(shmid < 0, -1, perror("shmget"));
        conp->icom_mem[MPID_NEM_IB_COM_RDMAWR_TO] = shmat(shmid, 0, 0);
        if (conp->icom_mem[MPID_NEM_IB_COM_RDMAWR_TO] == (char *) -1) {
            perror("Shared memory attach failure");
            shmctl(shmid, IPC_RMID, NULL);
            ibcom_errno = -1;
            goto fn_fail;
        }
#else
        /* ibv_reg_mr all memory area for all ring buffers
         * including shared and exclusive ones */
        if (!MPID_nem_ib_rdmawr_to_alloc_start) {
            ibcom_errno =
                MPID_nem_ib_rdmawr_to_init(MPID_NEM_IB_COM_RDMABUF_SZ * MPID_NEM_IB_NRINGBUF);
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(ibcom_errno, -1, printf("MPID_nem_ib_rdmawr_to_init"));
            dprintf("ib_com_open,MPID_nem_ib_rdmawr_to_alloc_free_list=%p\n",
                    MPID_nem_ib_rdmawr_to_alloc_free_list);
        }

        conp->icom_mem[MPID_NEM_IB_COM_RDMAWR_TO] = MPID_nem_ib_rdmawr_to_alloc_start;
        //mmap(0, MPID_NEM_IB_COM_RDMABUF_SZ, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
        //-1, 0);
        dprintf("MPID_nem_ib_com_open,mmap=%p,len=%d\n", conp->icom_mem[MPID_NEM_IB_COM_RDMAWR_TO],
                MPID_NEM_IB_COM_RDMABUF_SZ);
#endif

#ifdef HAVE_LIBDCFA
        dprintf("MPID_nem_ib_com_open,fd=%d,rmem=%p\n", *condesc,
                MPID_nem_ib_rdmawr_to_alloc_mr->buf);
#else
        dprintf("MPID_nem_ib_com_open,fd=%d,rmem=%p\n", *condesc,
                MPID_nem_ib_rdmawr_to_alloc_mr->addr);
#endif
        dprintf("MPID_nem_ib_com_open,fd=%d,rkey=%08x\n", *condesc,
                MPID_nem_ib_rdmawr_to_alloc_mr->rkey);

        /* RDMA-write-to remote memory area */
        conp->icom_rmem = (void **) MPIU_Malloc(sizeof(void **) * MPID_NEM_IB_COM_NBUF_RDMA);
        if (conp->icom_rmem == 0)
            goto err_exit;
        memset(conp->icom_rmem, 0, sizeof(void **) * MPID_NEM_IB_COM_NBUF_RDMA);

        conp->icom_rsize = (size_t *) MPIU_Malloc(sizeof(void **) * MPID_NEM_IB_COM_NBUF_RDMA);
        if (conp->icom_rsize == 0)
            goto err_exit;
        memset(conp->icom_rsize, 0, sizeof(void **) * MPID_NEM_IB_COM_NBUF_RDMA);

        conp->icom_rkey = (int *) MPIU_Malloc(sizeof(int) * MPID_NEM_IB_COM_NBUF_RDMA);
        if (conp->icom_rkey == 0)
            goto err_exit;
        memset(conp->icom_rkey, 0, sizeof(int) * MPID_NEM_IB_COM_NBUF_RDMA);
        break;
    case MPID_NEM_IB_COM_OPEN_SCRATCH_PAD:
        /* RDMA-write-from and -to local memory area */
        conp->icom_mrlist =
            (struct ibv_mr **) MPIU_Malloc(sizeof(struct ibv_mr *) *
                                           MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        memset(conp->icom_mrlist, 0, sizeof(struct ibv_mr *) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        conp->icom_mrlen = MPID_NEM_IB_COM_NBUF_SCRATCH_PAD;
        conp->icom_mem = (void **) MPIU_Malloc(sizeof(void *) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        memset(conp->icom_mem, 0, sizeof(void *) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        conp->icom_msize = (int *) MPIU_Malloc(sizeof(int) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        memset(conp->icom_msize, 0, sizeof(int) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

        /* RDMA-write-from local memory area */
        conp->icom_msize[MPID_NEM_IB_COM_SCRATCH_PAD_FROM] = MPID_NEM_IB_COM_SCRATCH_PAD_FROM_SZ;
        conp->icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_FROM] =
            mmap(0, MPID_NEM_IB_COM_SCRATCH_PAD_FROM_SZ, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(conp->icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_FROM] ==
                                       (void *) -1, -1, printf("mmap failed\n"));

        conp->icom_mrlist[MPID_NEM_IB_COM_SCRATCH_PAD_FROM] =
            MPID_nem_ib_com_reg_mr_fetch(conp->icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_FROM],
                                         conp->icom_msize[MPID_NEM_IB_COM_SCRATCH_PAD_FROM], 0,
                                         MPID_NEM_IB_COM_REG_MR_STICKY);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(!conp->icom_mrlist[MPID_NEM_IB_COM_SCRATCH_PAD_FROM], -1,
                                       printf("ibv_reg_mr failed\n"));

        /* RDMA-write-to remote memory area */
        conp->icom_rmem = (void **) MPIU_Malloc(sizeof(void *) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(conp->icom_rmem == 0, -1, dprintf("malloc failed\n"));
        memset(conp->icom_rmem, 0, sizeof(void *) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);

        conp->icom_rsize =
            (size_t *) MPIU_Malloc(sizeof(size_t) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(conp->icom_rsize == 0, -1, dprintf("malloc failed\n"));
        memset(conp->icom_rsize, 0, sizeof(size_t) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);

        conp->icom_rkey = (int *) MPIU_Malloc(sizeof(int) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(conp->icom_rkey == 0, -1, dprintf("malloc failed\n"));
        memset(conp->icom_rkey, 0, sizeof(int) * MPID_NEM_IB_COM_NBUF_SCRATCH_PAD);
        break;

    case MPID_NEM_IB_COM_OPEN_UD:
        /* UD-write-from and -to local memory area */
        conp->icom_mrlist = MPIU_Malloc(sizeof(struct ibv_mr *) * MPID_NEM_IB_COM_NBUF_UD);
        memset(conp->icom_mrlist, 0, sizeof(struct ibv_mr *) * MPID_NEM_IB_COM_NBUF_UD);
        conp->icom_mrlen = MPID_NEM_IB_COM_NBUF_UD;
        conp->icom_mem = (void **) MPIU_Malloc(sizeof(void **) * MPID_NEM_IB_COM_NBUF_UD);
        memset(conp->icom_mem, 0, sizeof(void **) * MPID_NEM_IB_COM_NBUF_UD);
        conp->icom_msize = (int *) MPIU_Malloc(sizeof(int *) * MPID_NEM_IB_COM_NBUF_UD);
        memset(conp->icom_msize, 0, sizeof(int *) * MPID_NEM_IB_COM_NBUF_UD);
        mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

        /* UD-write-from local memory area */
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(MPID_NEM_IB_COM_UDBUF_SZ <= 40, -1,
                                       dprintf("buf_size too short\n"));
        conp->icom_msize[MPID_NEM_IB_COM_UDWR_FROM] = MPID_NEM_IB_COM_UDBUF_SZ;
        conp->icom_mem[MPID_NEM_IB_COM_UDWR_FROM] =
            mmap(0, MPID_NEM_IB_COM_UDBUF_SZ, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
                 -1, 0);
        dprintf("MPID_nem_ib_com_open,mmap=%p,len=%d\n", conp->icom_mem[MPID_NEM_IB_COM_UDWR_FROM],
                MPID_NEM_IB_COM_UDBUF_SZ);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(conp->icom_mem[MPID_NEM_IB_COM_UDWR_FROM] == (void *) -1, -1,
                                       dprintf("failed to allocate buffer\n"));
        memset(conp->icom_mem[MPID_NEM_IB_COM_UDWR_FROM], 0,
               conp->icom_msize[MPID_NEM_IB_COM_UDWR_FROM]);

        conp->icom_mrlist[MPID_NEM_IB_COM_UDWR_FROM] =
            MPID_nem_ib_com_reg_mr_fetch(conp->icom_mem[MPID_NEM_IB_COM_UDWR_FROM],
                                         conp->icom_msize[MPID_NEM_IB_COM_UDWR_FROM], 0,
                                         MPID_NEM_IB_COM_REG_MR_STICKY);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(!conp->icom_mrlist[MPID_NEM_IB_COM_UDWR_FROM], -1,
                                       dprintf("ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags));

        /* UD-write-to local memory area */
        /* addr to addr+39 are not filled, addr+40 to addr+length-1 are filled with payload */
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(MPID_NEM_IB_COM_UDBUF_SZ <= 40, -1,
                                       dprintf("buf_size too short\n"));
        conp->icom_msize[MPID_NEM_IB_COM_UDWR_TO] = MPID_NEM_IB_COM_UDBUF_SZ;
        conp->icom_mem[MPID_NEM_IB_COM_UDWR_TO] =
            mmap(0, MPID_NEM_IB_COM_UDBUF_SZ, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
                 -1, 0);
        dprintf("MPID_nem_ib_com_open,mmap=%p,len=%d\n", conp->icom_mem[MPID_NEM_IB_COM_UDWR_TO],
                MPID_NEM_IB_COM_UDBUF_SZ);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(conp->icom_mem[MPID_NEM_IB_COM_UDWR_TO] == (void *) -1, -1,
                                       dprintf("failed to allocate buffer\n"));
        memset(conp->icom_mem[MPID_NEM_IB_COM_UDWR_TO], 0,
               conp->icom_msize[MPID_NEM_IB_COM_UDWR_TO]);

        conp->icom_mrlist[MPID_NEM_IB_COM_UDWR_TO] =
            MPID_nem_ib_com_reg_mr_fetch(conp->icom_mem[MPID_NEM_IB_COM_UDWR_TO],
                                         conp->icom_msize[MPID_NEM_IB_COM_UDWR_TO], 0,
                                         MPID_NEM_IB_COM_REG_MR_STICKY);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(!conp->icom_mrlist[MPID_NEM_IB_COM_UDWR_TO], -1,
                                       dprintf("ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags));

        /* initialize arena allocator for MPID_NEM_IB_COM_UDWR_TO */
        //MPID_nem_ib_com_udbuf_init(conp->icom_mem[MPID_NEM_IB_COM_UDWR_TO]);

        dprintf("MPID_nem_ib_com_open,ud,fd=%d,lkey=%08x\n", *condesc,
                conp->icom_mrlist[MPID_NEM_IB_COM_UDWR_TO]->lkey);
        break;
    default:
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(1, -1, dprintf("invalid open_flag\n"));
        break;

    }

    /* command templates */
    switch (open_flag) {
    case MPID_NEM_IB_COM_OPEN_RC:

        /* SR (send request) template */
        conp->icom_sr =
            (struct ibv_send_wr *) MPIU_Malloc(sizeof(struct ibv_send_wr) *
                                               MPID_NEM_IB_COM_RC_SR_NTEMPLATE);
        memset(conp->icom_sr, 0, sizeof(struct ibv_send_wr) * MPID_NEM_IB_COM_RC_SR_NTEMPLATE);

        for (i = 0; i < MPID_NEM_IB_COM_SMT_INLINE_NCHAIN; i++) {
            /* SGE (RDMA-send-from memory) template */
#ifdef HAVE_LIBDCFA
            memset(&(conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[0]), 0,
                   sizeof(struct ibv_sge) * WR_SG_NUM);
#else
            sge =
                (struct ibv_sge *) MPIU_Malloc(sizeof(struct ibv_sge) *
                                               MPID_NEM_IB_COM_SMT_INLINE_INITIATOR_NSGE);
            memset(sge, 0, sizeof(struct ibv_sge) * MPID_NEM_IB_COM_SMT_INLINE_INITIATOR_NSGE);
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].next =
                (i ==
                 MPID_NEM_IB_COM_SMT_INLINE_NCHAIN -
                 1) ? NULL : &conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i + 1];
#ifdef HAVE_LIBDCFA
#else
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list = sge;
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].opcode = IBV_WR_RDMA_WRITE;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].send_flags =
                IBV_SEND_SIGNALED | IBV_SEND_INLINE;
        }

        {
#ifdef HAVE_LIBDCFA
            memset(&(conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[0]), 0,
                   sizeof(struct ibv_sge) * WR_SG_NUM);
#else
            sge =
                (struct ibv_sge *) MPIU_Malloc(sizeof(struct ibv_sge) *
                                               MPID_NEM_IB_COM_SMT_NOINLINE_INITIATOR_NSGE);
            memset(sge, 0, sizeof(struct ibv_sge) * MPID_NEM_IB_COM_SMT_NOINLINE_INITIATOR_NSGE);
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].next = NULL;
#ifdef HAVE_LIBDCFA
#else
            conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list = sge;
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].opcode = IBV_WR_RDMA_WRITE;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].send_flags = IBV_SEND_SIGNALED;
        }
        {
            /* SR (send request) template for MPID_NEM_IB_COM_LMT_INITIATOR */
#ifdef HAVE_LIBDCFA
            memset(&(conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].sg_list[0]), 0,
                   sizeof(struct ibv_sge) * WR_SG_NUM);
#else
            sge =
                (struct ibv_sge *) MPIU_Malloc(sizeof(struct ibv_sge) *
                                               MPID_NEM_IB_COM_LMT_INITIATOR_NSGE);
            memset(sge, 0, sizeof(struct ibv_sge) * MPID_NEM_IB_COM_LMT_INITIATOR_NSGE);
#endif
            conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].next = NULL;
#ifdef HAVE_LIBDCFA
#else
            conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].sg_list = sge;
#endif
            conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].opcode = IBV_WR_RDMA_READ;
            conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].send_flags = IBV_SEND_SIGNALED;
        }

        /* SR (send request) template for MPID_NEM_IB_COM_LMT_PUT *//* for lmt-put-done */
#ifdef HAVE_LIBDCFA
        memset(&(conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].sg_list[0]), 0,
               sizeof(struct ibv_sge) * WR_SG_NUM);
#else
        sge = (struct ibv_sge *) MPIU_Malloc(sizeof(struct ibv_sge) * MPID_NEM_IB_COM_LMT_PUT_NSGE);
        memset(sge, 0, sizeof(struct ibv_sge) * MPID_NEM_IB_COM_LMT_PUT_NSGE);
#endif
        conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].next = NULL;
#ifdef HAVE_LIBDCFA
#else
        conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].sg_list = sge;
#endif
        conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].opcode = IBV_WR_RDMA_WRITE;
        conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].send_flags = IBV_SEND_SIGNALED;

        /* SR (send request) template for MPID_NEM_IB_COM_RDMAWR_FRMFIXED */
        /* not implemented */

        /* SGE (scatter gather element) template for recv */
        /* nothing is required for RDMA-write */

        /* RR (receive request) template for MPID_NEM_IB_COM_RDMAWR_RESPONDER */
        conp->icom_rr =
            (struct ibv_recv_wr *) MPIU_Malloc(sizeof(struct ibv_recv_wr) *
                                               MPID_NEM_IB_COM_RC_RR_NTEMPLATE);
        memset(conp->icom_rr, 0, sizeof(struct ibv_recv_wr) * MPID_NEM_IB_COM_RC_RR_NTEMPLATE);

        /* create one dummy RR to ibv_post_recv */
        conp->icom_rr[MPID_NEM_IB_COM_RDMAWR_RESPONDER].next = NULL;
#ifdef HAVE_LIBDCFA
#else
        conp->icom_rr[MPID_NEM_IB_COM_RDMAWR_RESPONDER].sg_list = NULL;
#endif
        conp->icom_rr[MPID_NEM_IB_COM_RDMAWR_RESPONDER].num_sge = 0;
        break;

    case MPID_NEM_IB_COM_OPEN_SCRATCH_PAD:{
            /* SR (send request) template */
            conp->icom_sr =
                (struct ibv_send_wr *) MPIU_Malloc(sizeof(struct ibv_send_wr) *
                                                   MPID_NEM_IB_COM_SCRATCH_PAD_SR_NTEMPLATE);
            memset(conp->icom_sr, 0,
                   sizeof(struct ibv_send_wr) * MPID_NEM_IB_COM_SCRATCH_PAD_SR_NTEMPLATE);

            /* SR (send request) template for MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR */
#ifdef HAVE_LIBDCFA
            memset(&(conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].sg_list[0]), 0,
                   sizeof(struct ibv_sge) * WR_SG_NUM);
#else
            sge =
                (struct ibv_sge *) MPIU_Malloc(sizeof(struct ibv_sge) *
                                               MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR_NSGE);
            memset(sge, 0, sizeof(struct ibv_sge) * MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR_NSGE);
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].next = NULL;
#ifdef HAVE_LIBDCFA
#else
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].sg_list = sge;
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].num_sge = 1;
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].opcode = IBV_WR_RDMA_WRITE;
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].send_flags =
                IBV_SEND_SIGNALED | IBV_SEND_INLINE;


#ifdef HAVE_LIBDCFA
            memset(&(conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].sg_list[0]),
                   0, sizeof(struct ibv_sge) * WR_SG_NUM);
#else
            sge =
                (struct ibv_sge *) MPIU_Malloc(sizeof(struct ibv_sge) *
                                               MPID_NEM_IB_COM_SCRATCH_PAD_GET_NSGE);
            memset(sge, 0, sizeof(struct ibv_sge) * MPID_NEM_IB_COM_SCRATCH_PAD_GET_NSGE);
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].next = NULL;
#ifdef HAVE_LIBDCFA
#else
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].sg_list = sge;
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].num_sge = 1;
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].opcode = IBV_WR_RDMA_READ;
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].send_flags = IBV_SEND_SIGNALED;


#ifdef HAVE_LIBDCFA
            memset(&(conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].sg_list[0]),
                   0, sizeof(struct ibv_sge) * WR_SG_NUM);
#else
            sge =
                (struct ibv_sge *) MPIU_Malloc(sizeof(struct ibv_sge) *
                                               MPID_NEM_IB_COM_SCRATCH_PAD_CAS_NSGE);
            memset(sge, 0, sizeof(struct ibv_sge) * MPID_NEM_IB_COM_SCRATCH_PAD_CAS_NSGE);
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].next = NULL;
#ifdef HAVE_LIBDCFA
#else
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].sg_list = sge;
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].num_sge = 1;
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].opcode = IBV_WR_ATOMIC_CMP_AND_SWP;
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].send_flags = IBV_SEND_SIGNALED;

#ifdef HAVE_LIBDCFA
            memset(&(conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].sg_list[0]),
                   0, sizeof(struct ibv_sge) * WR_SG_NUM);
#else
            sge = (struct ibv_sge *) MPIU_Malloc(sizeof(struct ibv_sge));
            memset(sge, 0, sizeof(struct ibv_sge));
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].next = NULL;
#ifdef HAVE_LIBDCFA
#else
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].sg_list = sge;
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].num_sge = 1;
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].opcode = IBV_WR_SEND;
            conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].send_flags = IBV_SEND_SIGNALED;

            /* RR (receive request) template */
            conp->icom_rr =
                (struct ibv_recv_wr *) MPIU_Malloc(sizeof(struct ibv_recv_wr) *
                                                   MPID_NEM_IB_COM_SCRATCH_PAD_RR_NTEMPLATE);
            memset(conp->icom_rr, 0,
                   sizeof(struct ibv_recv_wr) * MPID_NEM_IB_COM_SCRATCH_PAD_RR_NTEMPLATE);

            /* RR (receive request) template for MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER */
#ifdef HAVE_LIBDCFA
            memset(&(conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].sg_list[0]), 0,
                   sizeof(struct ibv_sge) * WR_SG_NUM);
#else
            sge = (struct ibv_sge *) MPIU_Malloc(sizeof(struct ibv_sge));
            memset(sge, 0, sizeof(struct ibv_sge));
#endif
            conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].next = NULL;
#ifdef HAVE_LIBDCFA
#else
            conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].sg_list = sge;
#endif
            conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].num_sge = 1;
            break;
        }

    case MPID_NEM_IB_COM_OPEN_UD:
        /* SGE (RDMA-send-from memory) template for MPID_NEM_IB_COM_UD_INITIATOR */
#ifdef HAVE_LIBDCFA
        sge = &(conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].sg_list[0]);
        memset(sge, 0, sizeof(struct ibv_sge) * WR_SG_NUM);
#else
        sge = (struct ibv_sge *) MPIU_Calloc(1, sizeof(struct ibv_sge));
#endif
        /* addr to addr + length - 1 will be on the payload, but search backword for "<= 40" */
        sge[0].addr = (uint64_t) conp->icom_mem[MPID_NEM_IB_COM_UDWR_FROM] + 40;
        sge[0].length = MPID_NEM_IB_COM_UDBUF_SZSEG - 40;
        sge[0].lkey = conp->icom_mrlist[MPID_NEM_IB_COM_UDWR_FROM]->lkey;


        conp->icom_ah_attr =
            (struct ibv_ah_attr *) MPIU_Calloc(MPID_NEM_IB_COM_UD_SR_NTEMPLATE,
                                               sizeof(struct ibv_ah_attr));

        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].sl = 0;
        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].src_path_bits = 0;
        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].static_rate = 0;       /* not limit on static rate (100% port speed) */
        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].is_global = 0;
        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].port_num = conp->icom_port;

#if 0
        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].is_global = 1;
        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].grh.flow_label = 0;
        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].grh.sgid_index = 0;    /* what is this? */
        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].grh.hop_limit = 1;
        conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].grh.traffic_class = 0;
#endif

        /* SR (send request) template for MPID_NEM_IB_COM_UD_INITIATOR */
        conp->icom_sr =
            (struct ibv_send_wr *) MPIU_Calloc(MPID_NEM_IB_COM_UD_SR_NTEMPLATE,
                                               sizeof(struct ibv_send_wr));

        conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].next = NULL;
#ifdef HAVE_LIBDCFA
#else
        conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].sg_list = sge;
#endif
        conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].num_sge = 1;
        conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].opcode = IBV_WR_SEND;
        conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].send_flags = IBV_SEND_SIGNALED;

        conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].wr.ud.remote_qkey = MPID_NEM_IB_COM_QKEY;

        /* SGE (scatter gather element) template for recv */
#ifdef HAVE_LIBDCFA
        sge = &(conp->icom_rr[MPID_NEM_IB_COM_UD_RESPONDER].sg_list[0]);
        memset(sge, 0, sizeof(struct ibv_sge) * WR_SG_NUM);
#else
        sge = (struct ibv_sge *) MPIU_Calloc(1, sizeof(struct ibv_sge));
#endif
        sge[0].addr = (uint64_t) conp->icom_mem[MPID_NEM_IB_COM_UDWR_TO];
        sge[0].length = MPID_NEM_IB_COM_UDBUF_SZ;
        sge[0].lkey = conp->icom_mrlist[MPID_NEM_IB_COM_UDWR_TO]->lkey;

        /* RR (receive request) template for MPID_NEM_IB_COM_UD_RESPONDER */
        conp->icom_rr =
            (struct ibv_recv_wr *) MPIU_Calloc(MPID_NEM_IB_COM_UD_RR_NTEMPLATE,
                                               sizeof(struct ibv_recv_wr));

        /* create one dummy RR to ibv_post_recv */
        conp->icom_rr[MPID_NEM_IB_COM_UD_RESPONDER].next = NULL;
#ifdef HAVE_LIBDCFA
#else
        conp->icom_rr[MPID_NEM_IB_COM_UD_RESPONDER].sg_list = sge;
#endif
        conp->icom_rr[MPID_NEM_IB_COM_UD_RESPONDER].num_sge = 1;
        break;
    }

    maxcon++;

  fn_exit:
    return ibcom_errno;
  err_exit:
    return -1;
  fn_fail:
    goto fn_exit;
}

/* 1. allocate memory area if it's not allocated or reuse it if it's allocated
   2. ibv_reg_mr it and store rkey to conp->icom_mrlist
   buf is output */
int MPID_nem_ib_com_alloc(int condesc, int sz)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
#ifdef MPID_NEM_IB_DEBUG_IBCOM
    int mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
#endif

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    switch (conp->open_flag) {

    case MPID_NEM_IB_COM_OPEN_SCRATCH_PAD:
        /* RDMA-write-to local memory area */
        MPID_nem_ib_scratch_pad_ref_count++;
        if (!MPID_nem_ib_scratch_pad) {
            MPID_nem_ib_scratch_pad =
                mmap(0, sz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
            dprintf("MPID_nem_ib_com_alloc,mmap=%p,len=%d\n", MPID_nem_ib_scratch_pad, sz);
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(MPID_nem_ib_scratch_pad == (void *) -1, -1,
                                           dprintf("failed to allocate buffer\n"));
            dprintf("MPID_nem_ib_com_alloc,MPID_nem_ib_scratch_pad=%p\n", MPID_nem_ib_scratch_pad);
            memset(MPID_nem_ib_scratch_pad, 0, sz);
        }
        conp->icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_TO] = MPID_nem_ib_scratch_pad;
        conp->icom_msize[MPID_NEM_IB_COM_SCRATCH_PAD_TO] = sz;

        conp->icom_mrlist[MPID_NEM_IB_COM_SCRATCH_PAD_TO] =
            MPID_nem_ib_com_reg_mr_fetch(conp->icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_TO],
                                         conp->icom_msize[MPID_NEM_IB_COM_SCRATCH_PAD_TO],
                                         IBV_ACCESS_REMOTE_ATOMIC, MPID_NEM_IB_COM_REG_MR_STICKY);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(!conp->icom_mrlist[MPID_NEM_IB_COM_SCRATCH_PAD_TO], -1,
                                       dprintf("ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags));

#ifdef HAVE_LIBDCFA
        dprintf("MPID_nem_ib_com_alloc,fd=%d,rmem=%p\n", condesc,
                conp->icom_mrlist[MPID_NEM_IB_COM_SCRATCH_PAD_TO]->buf);
#else
        dprintf("MPID_nem_ib_com_alloc,fd=%d,rmem=%p\n", condesc,
                conp->icom_mrlist[MPID_NEM_IB_COM_SCRATCH_PAD_TO]->addr);
#endif
        dprintf("MPID_nem_ib_com_alloc,fd=%d,rkey=%08x\n", condesc,
                conp->icom_mrlist[MPID_NEM_IB_COM_SCRATCH_PAD_TO]->rkey);
        break;
    default:
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(1, -1,
                                       dprintf("MPID_nem_ib_com_alloc, invalid open_flag=%d\n",
                                               conp->open_flag));
        break;
    }

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_free(int condesc, int sz)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    int retval;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    switch (conp->open_flag) {

    case MPID_NEM_IB_COM_OPEN_SCRATCH_PAD:
        MPIU_Assert(MPID_nem_ib_scratch_pad_ref_count > 0);
        if (--MPID_nem_ib_scratch_pad_ref_count == 0) {
            retval = munmap(MPID_nem_ib_scratch_pad, sz);
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(retval, -1, dprintf("munmap"));
            MPID_nem_ib_scratch_pad = NULL;
            dprintf("ib_com_free,MPID_nem_ib_scratch_pad is freed\n");
        }
        break;
    default:
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(1, -1,
                                       dprintf("MPID_nem_ib_com_free, invalid open_flag=%d\n",
                                               conp->open_flag));
        break;
    }

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_close(int condesc)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;

    dprintf("MPID_nem_ib_com_close,condesc=%d\n", condesc);

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    ibcom_errno = MPID_nem_ib_com_register_cache_release();
    MPID_nem_ib_com_clean(conp);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ibcom_errno, -1,
                                   printf("MPID_nem_ib_com_register_cache_release"));
    --maxcon;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_rts(int condesc, int remote_qpnum, uint16_t remote_lid,
                        union ibv_gid *remote_gid)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    int ib_errno;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    if (conp->icom_connected == 1) {
        ibcom_errno = -1;
        goto fn_fail;
    }

    struct ibv_qp_attr attr;
    int flags;

    switch (conp->open_flag) {
    case MPID_NEM_IB_COM_OPEN_SCRATCH_PAD:
        /* Init QP  */
        ib_errno = modify_qp_to_init(conp->icom_qp, conp->icom_port, IBV_ACCESS_REMOTE_ATOMIC);
        if (ib_errno) {
            fprintf(stderr, "change QP state to INIT failed\n");
            ibcom_errno = ib_errno;
            goto fn_fail;
        }
        goto common_tail;
    case MPID_NEM_IB_COM_OPEN_RC:
        /* Init QP  */
        ib_errno = modify_qp_to_init(conp->icom_qp, conp->icom_port, 0);
        if (ib_errno) {
            fprintf(stderr, "change QP state to INIT failed\n");
            ibcom_errno = ib_errno;
            goto fn_fail;
        }
      common_tail:
        /* Modify QP TO RTR status */
        ib_errno =
            modify_qp_to_rtr(conp->icom_qp, remote_qpnum, remote_lid, remote_gid, conp->icom_port,
                             0);
        conp->remote_lid = remote_lid;  /* for debug */
        if (ib_errno) {
            fprintf(stderr, "failed to modify QP state to RTR\n");
            ibcom_errno = ib_errno;
            goto fn_fail;
        }
        /* Modify QP TO RTS status */
        ib_errno = modify_qp_to_rts(conp->icom_qp);
        if (ib_errno) {
            fprintf(stderr, "failed to modify QP state to RTS\n");
            ibcom_errno = ib_errno;
            goto fn_fail;
        }
        break;
    case MPID_NEM_IB_COM_OPEN_UD:
        /* INIT */
        memset(&attr, 0, sizeof(attr));
        attr.qp_state = IBV_QPS_INIT;
        attr.port_num = conp->icom_port;
        attr.pkey_index = 0;
        attr.qkey = MPID_NEM_IB_COM_QKEY;
        flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_QKEY;
        ib_errno = ibv_modify_qp(conp->icom_qp, &attr, flags);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, perror("ibv_modify_qp"));

        /* RTR */
        memset(&attr, 0, sizeof(attr));
        attr.qp_state = IBV_QPS_RTR;
        flags = IBV_QP_STATE;
        ib_errno = ibv_modify_qp(conp->icom_qp, &attr, flags);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, perror("ibv_modify_qp"));

        /* RTS */
        memset(&attr, 0, sizeof(attr));
        attr.qp_state = IBV_QPS_RTS;
        attr.sq_psn = 0;
        flags = IBV_QP_STATE | IBV_QP_SQ_PSN;
        ib_errno = ibv_modify_qp(conp->icom_qp, &attr, flags);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, perror("ibv_modify_qp"));
        break;
    }
    conp->icom_connected = 1;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

#define MPID_NEM_IB_ENABLE_INLINE
/* <buf_from_out, buf_from_sz_out>: Free the slot in drain_scq */
int MPID_nem_ib_com_isend(int condesc,
                          uint64_t wr_id,
                          void *prefix, int sz_prefix,
                          void *hdr, int sz_hdr,
                          void *data, int sz_data,
                          int *copied,
                          uint32_t local_ringbuf_type, uint32_t remote_ringbuf_type,
                          void **buf_from_out, uint32_t * buf_from_sz_out)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    struct ibv_send_wr *bad_wr;
    int ib_errno;
    int num_sge;

    dprintf
        ("MPID_nem_ib_com_isend,prefix=%p,sz_prefix=%d,hdr=%p,sz_hdr=%d,data=%p,sz_data=%d,local_ringbuf_type=%d,remote_ringbuf_type=%d\n",
         prefix, sz_prefix, hdr, sz_hdr, data, sz_data, local_ringbuf_type, remote_ringbuf_type);

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    if (conp->icom_connected == 0) {
        return -1;
    }


    int off_pow2_aligned;
    MPID_NEM_IB_OFF_POW2_ALIGNED(MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type) + sz_prefix +
                                 sz_hdr + sz_data);
    uint32_t sumsz = off_pow2_aligned + sizeof(MPID_nem_ib_netmod_trailer_t);
    int sz_pad =
        off_pow2_aligned - (MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type) + sz_prefix + sz_hdr +
                            sz_data);

    uint32_t buf_from_sz = MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type) + sz_prefix + sz_hdr +
        sz_pad + sizeof(MPID_nem_ib_netmod_trailer_t);
    *buf_from_sz_out = buf_from_sz;
    void *buf_from = MPID_nem_ib_rdmawr_from_alloc(buf_from_sz);
    dprintf("isend,rdmawr_from_alloc=%p,sz=%d\n", buf_from, buf_from_sz);
    *buf_from_out = buf_from;
    struct ibv_mr *mr_rdmawr_from = MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_MR(buf_from);

    if (sz_data > 16000) {
        //dprintf("MPID_nem_ib_com_isend,sz_data=%d,off_pow2_aligned=%d,sz_max=%ld\n", sz_data, off_pow2_aligned, MPID_NEM_IB_MAX_DATA_POW2);
    }

    num_sge = 0;
    uint32_t hdr_ringbuf_type = local_ringbuf_type;
    MPID_NEM_IB_NETMOD_HDR_SZ_SET(buf_from,
                                  MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type) +
                                  sz_prefix + sz_hdr + sz_data);
    if (remote_ringbuf_type == MPID_NEM_IB_RINGBUF_EXCLUSIVE) {
        hdr_ringbuf_type |= MPID_NEM_IB_RINGBUF_RELINDEX;
        MPID_NEM_IB_NETMOD_HDR_RELINDEX_SET(buf_from, conp->rsr_seq_num_tail);
        conp->rsr_seq_num_tail_last_sent = conp->rsr_seq_num_tail;
        dprintf("isend,rsr_seq_num_tail=%d\n", MPID_NEM_IB_NETMOD_HDR_RELINDEX_GET(buf_from));
    }
    if (local_ringbuf_type == MPID_NEM_IB_RINGBUF_SHARED) {
        MPID_NEM_IB_NETMOD_HDR_VC_SET(buf_from, conp->remote_vc);
        dprintf("isend,remote_vc=%p\n", MPID_NEM_IB_NETMOD_HDR_VC_GET(buf_from));
    }
    MPID_NEM_IB_NETMOD_HDR_RINGBUF_TYPE_SET(buf_from, hdr_ringbuf_type);
    dprintf("isend,hdr_ringbuf_type=%08x\n", MPID_NEM_IB_NETMOD_HDR_RINGBUF_TYPE_GET(buf_from));

    /* memcpy hdr is needed because hdr resides in stack when sending close-VC command */
    /* memcpy is performed onto MPID_NEM_IB_COM_RDMAWR_FROM buffer */
    void *hdr_copy = (uint8_t *) buf_from + MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type);
    memcpy(hdr_copy, prefix, sz_prefix);
    memcpy((uint8_t *) hdr_copy + sz_prefix, hdr, sz_hdr);
#ifdef HAVE_LIBDCFA
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].mic_addr = (uint64_t) buf_from;
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].addr =
        mr_rdmawr_from->host_addr +
        ((uint64_t) buf_from - (uint64_t) MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_START(buf_from));

#else
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].addr = (uint64_t) buf_from;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].length =
        MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type) + sz_prefix + sz_hdr;
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].lkey = mr_rdmawr_from->lkey;
    num_sge += 1;

    struct MPID_nem_ib_com_reg_mr_cache_entry_t *mr_cache = NULL;
    if (sz_data) {
        //dprintf("MPID_nem_ib_com_isend,data=%p,sz_data=%d\n", data, sz_data);
        mr_cache = MPID_nem_ib_com_reg_mr_fetch(data, sz_data, 0, MPID_NEM_IB_COM_REG_MR_GLOBAL);
        MPID_NEM_IB_COM_ERR_CHKANDJUMP(!mr_cache, -1,
                                       printf("MPID_nem_ib_com_isend,ibv_reg_mr_fetch failed\n"));
        struct ibv_mr *mr_data = mr_cache->mr;
#ifdef HAVE_LIBDCFA
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].mic_addr = (uint64_t) data;
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].addr =
            mr_data->host_addr + ((uint64_t) data - (uint64_t) data);
#else
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].addr = (uint64_t) data;
#endif
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].length = sz_data;
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].lkey = mr_data->lkey;
        num_sge += 1;
    }

    MPID_nem_ib_netmod_trailer_t *netmod_trailer =
        (MPID_nem_ib_netmod_trailer_t *) ((uint8_t *) buf_from +
                                          MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type) +
                                          sz_prefix + sz_hdr + sz_pad);
    netmod_trailer->tail_flag = MPID_NEM_IB_COM_MAGIC;
#ifdef HAVE_LIBDCFA
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].mic_addr =
        (uint64_t) buf_from + MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type) + sz_prefix +
        sz_hdr;
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].addr =
        mr_rdmawr_from->host_addr + ((uint64_t) buf_from +
                                     MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type) + sz_prefix +
                                     sz_hdr - (uint64_t)
                                     MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_START(buf_from));
#else
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].addr =
        (uint64_t) buf_from + MPID_NEM_IB_NETMOD_HDR_SIZEOF(local_ringbuf_type) + sz_prefix +
        sz_hdr;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].length =
        sz_pad + sizeof(MPID_nem_ib_netmod_trailer_t);
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].sg_list[num_sge].lkey = mr_rdmawr_from->lkey;
    num_sge += 1;
    dprintf("MPID_nem_ib_com_isend,sz_data=%d,pow2=%d,sz_pad=%d,num_sge=%d\n", sz_data,
            off_pow2_aligned, sz_pad, num_sge);

    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].num_sge = num_sge;
#if 1
    MPID_nem_ib_rc_send_request *wrap_wr_id = MPIU_Malloc(sizeof(MPID_nem_ib_rc_send_request));
    wrap_wr_id->wr_id = wr_id;
    wrap_wr_id->mf = MPID_NEM_IB_LAST_PKT;
    wrap_wr_id->mr_cache = (void *) mr_cache;

    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr_id = (uint64_t) wrap_wr_id;
#else
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr_id = wr_id;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr.rdma.remote_addr =
        (uint64_t) conp->local_ringbuf_start +
        MPID_NEM_IB_COM_RDMABUF_SZSEG * ((uint16_t) (conp->sseq_num % conp->local_ringbuf_nslot));
    dprintf("isend,ringbuf_start=%p,local_head=%04ux,nslot=%d,rkey=%08x,remote_addr=%lx\n",
            conp->local_ringbuf_start, conp->sseq_num, conp->local_ringbuf_nslot,
            conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr.rdma.rkey,
            conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr.rdma.remote_addr);
    if (conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr.rdma.remote_addr <
        (uint64_t) conp->local_ringbuf_start ||
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr.rdma.remote_addr >=
        (uint64_t) conp->local_ringbuf_start +
        MPID_NEM_IB_COM_RDMABUF_SZSEG * conp->local_ringbuf_nslot) {
        MPID_nem_ib_segv;
    }
    /* rkey is defined in MPID_nem_ib_com_connect_ringbuf */

    //dprintf("MPID_nem_ib_com_isend,condesc=%d,num_sge=%d,opcode=%08x,imm_data=%08x,wr_id=%016lx, raddr=%p, rkey=%08x\n", condesc, conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].num_sge, conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].opcode, conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].imm_data, conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr_id, conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr.rdma.remote_addr, conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr.rdma.rkey);

    /* other commands can executed RDMA-rd command */
    /* see the "Ordering and the Fence Indicator" section in "InfiniBand Architecture" by William T. Futral */
#if 0
    if (conp->after_rdma_rd) {
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].send_flags |= IBV_SEND_FENCE;
    }
#endif
#ifdef MPID_NEM_IB_ENABLE_INLINE
    if (sumsz <= conp->max_inline_data) {
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].send_flags |= IBV_SEND_INLINE;
        *copied = 1;
    }
    else {
        *copied = 0;
    }
#endif
#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE]);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf("MPID_nem_ib_com_isend, ibv_post_send, rc=%d\n",
                                           ib_errno));
#else
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE], &bad_wr);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_isend, ibv_post_send, rc=%d, bad_wr=%p\n",
                                    ib_errno, bad_wr));
#endif
#ifdef MPID_NEM_IB_ENABLE_INLINE
    if (sumsz <= conp->max_inline_data) {
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].send_flags &= ~IBV_SEND_INLINE;
    }
#endif
#if 0
    if (conp->after_rdma_rd) {
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].send_flags &= ~IBV_SEND_FENCE;
        conp->after_rdma_rd = 0;
    }
#endif

    conp->sseq_num += 1;
    conp->ncom += 1;
  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

#if 0
int MPID_nem_ib_com_isend_chain(int condesc, uint64_t wr_id, void *hdr, int sz_hdr, void *data,
                                int sz_data)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    struct ibv_send_wr *bad_wr;
    int ib_errno;
    int sz_data_rem = sz_data;
    int i;
    struct ibv_mr *mr_data;
    uint32_t sumsz =
        sizeof(MPID_nem_ib_netmod_hdr_t) + sz_hdr + sz_data + sizeof(MPID_nem_ib_netmod_trailer_t);
    unsigned long tscs, tsce;

    dprintf("MPID_nem_ib_com_isend_chain,enter\n");
    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(conp->icom_connected == 0, -1,
                                   printf("MPID_nem_ib_com_isend_chain,icom_connected==0\n"));

    void *buf_from =
        (uint8_t *) conp->icom_mem[MPID_NEM_IB_COM_RDMAWR_FROM] +
        MPID_NEM_IB_COM_RDMABUF_SZSEG *
        ((uint16_t) (conp->sseq_num % MPID_NEM_IB_COM_RDMABUF_NSEG));

    /* make a tail-magic position is in a fixed set */
    int off_pow2_aligned;
    MPID_NEM_IB_OFF_POW2_ALIGNED(sizeof(MPID_nem_ib_netmod_hdr_t) + sz_hdr + sz_data);

    /* let the last command icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAIN-1] which has IBV_WR_RDMA_WRITE_WITH_IMM */
    int s =
        MPID_NEM_IB_COM_SMT_INLINE_NCHAIN - (sizeof(MPID_nem_ib_netmod_hdr_t) + sz_hdr +
                                             off_pow2_aligned +
                                             sizeof(MPID_nem_ib_netmod_trailer_t) +
                                             MPID_NEM_IB_COM_INLINE_DATA -
                                             1) / MPID_NEM_IB_COM_INLINE_DATA;
    MPID_NEM_IB_COM_ERR_CHKANDJUMP((sizeof(MPID_nem_ib_netmod_hdr_t) + sz_hdr +
                                    off_pow2_aligned) % 4 != 0, -1,
                                   printf
                                   ("MPID_nem_ib_com_isend_chain,tail-magic gets over packet-boundary\n"));
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(s < 0 ||
                                   s >= MPID_NEM_IB_COM_SMT_INLINE_NCHAIN, -1,
                                   printf("MPID_nem_ib_com_isend_chain,s\n"));
    dprintf("MPID_nem_ib_com_isend_chain,sz_hdr=%d,sz_data=%d,s=%d\n", sz_hdr, sz_data, s);

    for (i = s; i < MPID_NEM_IB_COM_SMT_INLINE_NCHAIN; i++) {

        //tscs = MPID_nem_ib_rdtsc();
        int sz_used = 0;        /* how much of the payload of a IB packet is used? */
        int num_sge = 0;
        if (i == s) {
            MPID_nem_ib_netmod_hdr_t *netmod_hdr = (MPID_nem_ib_netmod_hdr_t *) buf_from;
            MPID_NEM_IB_NETMOD_HDR_SZ_SET(netmod_hdr, sumsz);
            memcpy((uint8_t *) buf_from + sizeof(MPID_nem_ib_netmod_hdr_t), hdr, sz_hdr);
#ifdef HAVE_LIBDCFA
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].mic_addr =
                (uint64_t) buf_from;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].addr =
                conp->icom_mrlist[MPID_NEM_IB_COM_RDMAWR_FROM]->host_addr + ((uint64_t) buf_from -
                                                                             (uint64_t)
                                                                             conp->icom_mem
                                                                             [MPID_NEM_IB_COM_RDMAWR_FROM]);
#else
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].addr =
                (uint64_t) buf_from;
#endif
            buf_from = (uint8_t *) buf_from + sizeof(MPID_nem_ib_netmod_hdr_t) + sz_hdr;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].length =
                sizeof(MPID_nem_ib_netmod_hdr_t) + sz_hdr;
            sz_used += sizeof(MPID_nem_ib_netmod_hdr_t) + sz_hdr;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].lkey =
                conp->icom_mrlist[MPID_NEM_IB_COM_RDMAWR_FROM]->lkey;
            num_sge += 1;
            dprintf("MPID_nem_ib_com_isend_chain,i=%d,sz_used=%d\n", i, sz_used);
        }
        //tsce = MPID_nem_ib_rdtsc(); printf("0,%ld\n", tsce-tscs);

        //tscs = MPID_nem_ib_rdtsc();
        if (sz_data_rem > 0) {
#ifdef HAVE_LIBDCFA
#else
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].addr =
                (uint64_t) data + sz_data - sz_data_rem;
#endif
            int sz_data_red =
                sz_used + sz_data_rem + sizeof(MPID_nem_ib_netmod_trailer_t) <=
                MPID_NEM_IB_COM_INLINE_DATA ? sz_data_rem : sz_data_rem <=
                MPID_NEM_IB_COM_INLINE_DATA - sz_used ? sz_data_rem : MPID_NEM_IB_COM_INLINE_DATA -
                sz_used;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].length =
                sz_data_red;
            sz_used += sz_data_red;
            sz_data_rem -= sz_data_red;
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(sz_data_rem < 0, -1,
                                           printf("MPID_nem_ib_com_isend_chain,sz_data_rem\n"));

            if (i == s) {
                MPID_NEM_IB_COM_ERR_CHKANDJUMP(!sz_data, -1,
                                               printf("MPID_nem_ib_com_isend_chain,sz_data==0\n"));
                mr_data = MPID_nem_ib_com_reg_mr_fetch(data, sz_data, 0);
                MPID_NEM_IB_COM_ERR_CHKANDJUMP(!mr_data, -1,
                                               printf
                                               ("MPID_nem_ib_com_isend,ibv_reg_mr_fetch failed\n"));
            }
#ifdef HAVE_LIBDCFA
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].mic_addr =
                (uint64_t) data + sz_data - sz_data_rem;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].addr =
                mr_data->host_addr + ((uint64_t) data + sz_data - sz_data_rem - (uint64_t) data);
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].lkey =
                mr_data->lkey;
            num_sge += 1;
            dprintf("MPID_nem_ib_com_isend_chain,i=%d,sz_used=%d,sz_data_rem=%d\n", i, sz_used,
                    sz_data_rem);
        }
        else {  /* netmod_trailer only packet is being generated */

        }
        //tsce = MPID_nem_ib_rdtsc(); printf("1,%ld\n", tsce-tscs);

        //tscs = MPID_nem_ib_rdtsc();
        if (i == MPID_NEM_IB_COM_SMT_INLINE_NCHAIN - 1) {       /* append netmod_trailer */
            int sz_pad = off_pow2_aligned - sz_data;
            MPID_nem_ib_netmod_trailer_t *netmod_trailer =
                (MPID_nem_ib_netmod_trailer_t *) ((uint8_t *) buf_from + sz_pad);
            netmod_trailer->tail_flag = MPID_NEM_IB_COM_MAGIC;
#ifdef HAVE_LIBDCFA
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].mic_addr =
                (uint64_t) buf_from;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].addr =
                conp->icom_mrlist[MPID_NEM_IB_COM_RDMAWR_FROM]->host_addr + ((uint64_t) buf_from -
                                                                             (uint64_t)
                                                                             conp->icom_mem
                                                                             [MPID_NEM_IB_COM_RDMAWR_FROM]);
#else
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].addr =
                (uint64_t) buf_from;
#endif
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].length =
                sz_pad + sizeof(MPID_nem_ib_netmod_trailer_t);
            sz_used += sz_pad + sizeof(MPID_nem_ib_netmod_trailer_t);
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(sz_data_rem != 0, -1,
                                           printf("MPID_nem_ib_com_isend_chain, sz_data_rem\n"));
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].lkey =
                conp->icom_mrlist[MPID_NEM_IB_COM_RDMAWR_FROM]->lkey;
            num_sge += 1;

            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].imm_data = conp->sseq_num;
            dprintf("MPID_nem_ib_com_isend_chain,i=%d,sz_pad=%d,sz_used=%d,num_sge=%d\n", i, sz_pad,
                    sz_used, num_sge);
        }
        else if (MPID_NEM_IB_COM_INLINE_DATA - sz_used > 0) {   /* data fell short of the packet, so pad */
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(1, -1,
                                           printf
                                           ("MPID_nem_ib_com_isend_chain,tail-magic gets over packet-boundary\n"));
            int sz_pad = MPID_NEM_IB_COM_INLINE_DATA - sz_used;
#ifdef HAVE_LIBDCFA
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].mic_addr =
                (uint64_t) buf_from;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].addr =
                conp->icom_mrlist[MPID_NEM_IB_COM_RDMAWR_FROM]->host_addr + ((uint64_t) buf_from -
                                                                             (uint64_t)
                                                                             conp->icom_mem
                                                                             [MPID_NEM_IB_COM_RDMAWR_FROM]);
#else
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].addr =
                (uint64_t) buf_from;
#endif
            buf_from = (uint8_t *) buf_from + sz_pad;
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].length = sz_pad;
            sz_used += sz_pad;
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(sz_used != MPID_NEM_IB_COM_INLINE_DATA, -1,
                                           printf("MPID_nem_ib_com_isend_chain, sz_used\n"));
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].sg_list[num_sge].lkey =
                conp->icom_mrlist[MPID_NEM_IB_COM_RDMAWR_FROM]->lkey;
            num_sge += 1;
            dprintf("MPID_nem_ib_com_isend_chain,i=%d,sz_pad=%d,sz_used=%d\n", i, sz_pad, sz_used);
        }
        else {  /* packet is full with data */
            MPID_NEM_IB_COM_ERR_CHKANDJUMP(sz_used != MPID_NEM_IB_COM_INLINE_DATA, -1,
                                           printf("MPID_nem_ib_com_isend_chain, sz_used\n"));
        }
        //tsce = MPID_nem_ib_rdtsc(); printf("2,%ld\n", tsce-tscs);

        conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].num_sge = num_sge;
        conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].wr_id = wr_id;
        conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].wr.rdma.remote_addr =
            (uint64_t) conp->icom_rmem[MPID_NEM_IB_COM_RDMAWR_TO] +
            MPID_NEM_IB_COM_RDMABUF_SZSEG *
            ((uint16_t) (conp->sseq_num % MPID_NEM_IB_COM_RDMABUF_NSEG)) +
            MPID_NEM_IB_COM_INLINE_DATA * (i - s);
    }
#if 0
    if (conp->after_rdma_rd) {
        conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + s].send_flags |= IBV_SEND_FENCE;
    }
#endif
#ifdef HAVE_LIBDCFA
    ib_errno =
        ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + s]);
#else
    ib_errno =
        ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + s],
                      &bad_wr);
#endif
#if 0
    if (i == 0 && conp->after_rdma_rd) {
        conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + s].send_flags &= ~IBV_SEND_FENCE;
        conp->after_rdma_rd = 0;
    }
#endif
#ifdef HAVE_LIBDCFA
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf("MPID_nem_ib_com_isend, ibv_post_send, rc=%d\n",
                                           ib_errno));
#else
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_isend, ibv_post_send, rc=%d, bad_wr=%p\n",
                                    ib_errno, bad_wr));
#endif
    conp->ncom += (MPID_NEM_IB_COM_SMT_INLINE_NCHAIN - s);
    conp->sseq_num += 1;
  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}
#endif

int MPID_nem_ib_com_irecv(int condesc, uint64_t wr_id)
{

    MPID_nem_ib_com_t *conp;
    int ib_errno;
    int ibcom_errno = 0;
    struct ibv_recv_wr *bad_wr;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    //    if (conp->icom_connected == 0) { return -1; }

    //dprintf("MPID_nem_ib_com_irecv,condesc=%d,wr_id=%016lx\n", condesc, wr_id);

    conp->icom_rr[MPID_NEM_IB_COM_RDMAWR_RESPONDER].wr_id = wr_id;
#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_recv(conp->icom_qp, &conp->icom_rr[MPID_NEM_IB_COM_RDMAWR_RESPONDER]);
#else
    ib_errno =
        ibv_post_recv(conp->icom_qp, &conp->icom_rr[MPID_NEM_IB_COM_RDMAWR_RESPONDER], &bad_wr);
#endif
    if (ib_errno) {
#ifdef HAVE_LIBDCFA
        fprintf(stderr, "MPID_nem_ib_com_irecv: failed to post receive, ib_errno=%d\n", ib_errno);
#else
        fprintf(stderr, "MPID_nem_ib_com_irecv: failed to post receive, ib_errno=%d,bad_wr=%p\n",
                ib_errno, bad_wr);
#endif
        ibcom_errno = ib_errno;
        goto fn_fail;
    }
  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_udsend(int condesc, union ibv_gid *remote_gid, uint16_t remote_lid,
                           uint32_t remote_qpn, uint32_t imm_data, uint64_t wr_id)
{
    MPID_nem_ib_com_t *conp;
    struct ibv_send_wr *bad_wr;
    int ibcom_errno = 0, ib_errno;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

#ifdef HAVE_LIBDCFA
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(1, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_udsend not supported by DCFA because DCFA doesn't have ibv_create_ah\n"));
#else
    /* prepare ibv_ah_attr */
    conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].dlid = remote_lid;
#if 0
    conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].grh.dgid = *remote_gid;
#endif

    /* prepare ibv_ah */
    struct ibv_ah *ah;
    ah = ibv_create_ah(ib_pd, &conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR]);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!ah, -1, dprintf("ibv_crate_ah\n"));

    conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].wr.ud.ah = ah;
    conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].wr.ud.remote_qpn = remote_qpn;
    /* qkey is defined in open */

    //dprintf("lid=%04x\n", conp->icom_ah_attr[MPID_NEM_IB_COM_UD_INITIATOR].dlid);
    //dprintf("qpn=%08x\n", conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].wr.ud.remote_qpn);

    /* recv doesn't know the length, so we can't optimize it */
    //    conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].sg_list[0].length = length;

    conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].wr_id = wr_id;
    conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].imm_data = imm_data;

#if 0
    if (length <= qpinfo->max_inline_data) {
        conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR].send_flags |= IBV_SEND_INLINE;
    }
#endif

#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR]);
#else
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_UD_INITIATOR], &bad_wr);
#endif
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, perror("ibv_post_send"));
#endif /* DCFA */

    conp->ncom += 1;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_udrecv(int condesc)
{
    MPID_nem_ib_com_t *conp;
    struct ibv_recv_wr *bad_wr;
    int ibcom_errno = 0, ib_errno;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    /* Create RR */
    conp->icom_rr[MPID_NEM_IB_COM_UD_RESPONDER].wr_id = 0;

    /* Post RR to RQ */
#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_recv(conp->icom_qp, &conp->icom_rr[MPID_NEM_IB_COM_UD_RESPONDER]);
#else
    ib_errno = ibv_post_recv(conp->icom_qp, &conp->icom_rr[MPID_NEM_IB_COM_UD_RESPONDER], &bad_wr);
#endif
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, dprintf("ibv_post_recv ib_errno=%d\n", ib_errno));

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_lrecv(int condesc, uint64_t wr_id, void *raddr, long sz_data, uint32_t rkey,
                          void *laddr, int last)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    struct ibv_send_wr *bad_wr;
    int ib_errno;
    int num_sge = 0;

    dprintf("MPID_nem_ib_com_lrecv,enter,raddr=%p,sz_data=%ld,laddr=%p\n", raddr, sz_data, laddr);

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!conp->icom_connected, -1,
                                   dprintf("MPID_nem_ib_com_lrecv,not connected\n"));
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!sz_data, -1, dprintf("MPID_nem_ib_com_lrecv,sz_data==0\n"));

    /* register memory area containing data */
    struct MPID_nem_ib_com_reg_mr_cache_entry_t *mr_cache =
        MPID_nem_ib_com_reg_mr_fetch(laddr, sz_data, 0, MPID_NEM_IB_COM_REG_MR_GLOBAL);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!mr_cache, -1,
                                   dprintf("MPID_nem_ib_com_lrecv,ibv_reg_mr_fetch failed\n"));
    struct ibv_mr *mr_data = mr_cache->mr;

    MPID_nem_ib_rc_send_request *wrap_wr_id = MPIU_Malloc(sizeof(MPID_nem_ib_rc_send_request));
    wrap_wr_id->wr_id = wr_id;
    wrap_wr_id->mf = last;
    wrap_wr_id->mr_cache = (void *) mr_cache;

    num_sge = 0;

    /* Erase magic, super bug!! */
    //((MPID_nem_ib_netmod_trailer_t*)(laddr + sz_data - sizeof(MPID_nem_ib_netmod_trailer_t)))->magic = 0;
#ifdef HAVE_LIBDCFA
    conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].sg_list[num_sge].mic_addr = (uint64_t) laddr;
    conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].sg_list[num_sge].addr =
        mr_data->host_addr + ((uint64_t) laddr - (uint64_t) laddr);
#else
    conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].sg_list[num_sge].addr = (uint64_t) laddr;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].sg_list[num_sge].length = sz_data;
    conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].sg_list[num_sge].lkey = mr_data->lkey;
    num_sge += 1;

    conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].num_sge = num_sge;
    conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].wr_id = (uint64_t) wrap_wr_id;
    conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].wr.rdma.remote_addr = (uint64_t) raddr;
    conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR].wr.rdma.rkey = rkey;

#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR]);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf("MPID_nem_ib_com_lrecv, ibv_post_send, rc=%d\n",
                                           ib_errno));
#else
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_LMT_INITIATOR], &bad_wr);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_lrecv, ibv_post_send, rc=%d, bad_wr=%p\n",
                                    ib_errno, bad_wr));
#endif

    /* other commands can be executed before RDMA-rd command */
    /* see the "Ordering and the Fence Indicator" section in "InfiniBand Architecture" by William T. Futral */
#if 0
    conp->after_rdma_rd = 1;
#endif
    conp->ncom += 1;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

/* use the same QP as isend */
int MPID_nem_ib_com_put_lmt(int condesc, uint64_t wr_id, void *raddr, int sz_data, uint32_t rkey,
                            void *laddr)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    struct ibv_send_wr *bad_wr;
    int ib_errno;
    int num_sge;

    dprintf("MPID_nem_ib_com_put_lmt,enter,sz_data=%d,laddr=%p\n", sz_data, laddr);

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!conp->icom_connected, -1,
                                   dprintf("MPID_nem_ib_com_put_lmt,not connected\n"));
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!sz_data, -1, dprintf("MPID_nem_ib_com_put_lmt,sz_data==0\n"));

    num_sge = 0;

    /* register memory area containing data */
    struct MPID_nem_ib_com_reg_mr_cache_entry_t *mr_cache =
        MPID_nem_ib_com_reg_mr_fetch(laddr, sz_data, 0, MPID_NEM_IB_COM_REG_MR_GLOBAL);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!mr_cache, -1,
                                   dprintf("MPID_nem_ib_com_put_lmt,ibv_reg_mr_fetch failed\n"));
    struct ibv_mr *mr_data = mr_cache->mr;

#ifdef HAVE_LIBDCFA
    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].sg_list[num_sge].mic_addr = (uint64_t) laddr;
    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].sg_list[num_sge].addr =
        mr_data->host_addr + ((uint64_t) laddr - (uint64_t) laddr);
#else
    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].sg_list[num_sge].addr = (uint64_t) laddr;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].sg_list[num_sge].length = sz_data;
    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].sg_list[num_sge].lkey = mr_data->lkey;
    num_sge += 1;

    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].num_sge = num_sge;
#if 1
    MPID_nem_ib_rc_send_request *wrap_wr_id = MPIU_Malloc(sizeof(MPID_nem_ib_rc_send_request));
    wrap_wr_id->wr_id = wr_id;
    wrap_wr_id->mf = MPID_NEM_IB_LAST_PKT;
    wrap_wr_id->mr_cache = (void *) mr_cache;

    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].wr_id = (uint64_t) wrap_wr_id;
#else
    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].wr_id = wr_id;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].wr.rdma.remote_addr = (uint64_t) raddr;
    conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT].wr.rdma.rkey = rkey;

#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT]);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf("MPID_nem_ib_com_put_lmt, ibv_post_send, rc=%d\n",
                                           ib_errno));
#else
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_LMT_PUT], &bad_wr);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_put_lmt, ibv_post_send, rc=%d, bad_wr=%p\n",
                                    ib_errno, bad_wr));
#endif

    conp->ncom += 1;
    dprintf("MPID_nem_ib_com_put_lmt,exit\n");

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_scratch_pad_recv(int condesc, int sz_data)
{
    MPID_nem_ib_com_t *conp;
    struct ibv_recv_wr *bad_wr;
    int ibcom_errno = 0, ib_errno;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    void *buf_to = MPID_nem_ib_rdmawr_from_alloc(sz_data);
    struct ibv_mr *mr_buf_to = MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_MR(buf_to);

    /* Create RR */

#ifdef HAVE_LIBDCFA
    conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].sg_list[0].mic_addr = (uint64_t) buf_to;
    conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].sg_list[0].addr =
        mr_buf_to->host_addr + ((uint64_t) buf_to -
                                (uint64_t) MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_START(buf_to));
#else
    conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].sg_list[0].addr = (uint64_t) buf_to;
#endif

    conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].sg_list[0].length = sz_data;
    conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].sg_list[0].lkey = mr_buf_to->lkey;

    conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER].wr_id = (uint64_t) buf_to;

    /* Post RR to RQ */
#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_recv(conp->icom_qp, &conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER]);
#else
    ib_errno =
        ibv_post_recv(conp->icom_qp, &conp->icom_rr[MPID_NEM_IB_COM_SCRATCH_PAD_RESPONDER],
                      &bad_wr);
#endif
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1, dprintf("ibv_post_recv ib_errno=%d\n", ib_errno));

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_put_scratch_pad(int condesc, uint64_t wr_id, uint64_t offset, int sz,
                                    void *laddr, void **buf_from_out, uint32_t * buf_from_sz_out)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    struct ibv_send_wr *bad_wr;
    int ib_errno;

    dprintf("MPID_nem_ib_com_put_scratch_pad,enter,wr_id=%llx,offset=%llx,sz=%d,laddr=%p\n",
            (unsigned long long) wr_id, (unsigned long long) offset, sz, laddr);
    dprintf("MPID_nem_ib_com_put_scratch_pad,data=%08x\n", *((uint32_t *) laddr));

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(conp->open_flag != MPID_NEM_IB_COM_OPEN_SCRATCH_PAD, -1,
                                   dprintf("MPID_nem_ib_com_put_scratch_pad,invalid open_flag=%d\n",
                                           conp->open_flag));
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!conp->icom_connected, -1,
                                   dprintf("MPID_nem_ib_com_put_scratch_pad,not connected\n"));
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(!sz, -1, dprintf("MPID_nem_ib_com_put_scratch_pad,sz==0\n"));

    /* Use inline so that we don't need to worry about overwriting write-from buffer */
//    assert(sz <= conp->max_inline_data);

    /* When cm_progress calls this function, 'comp->icom_mem' and 'laddr' are not equal. */
//    assert(conp->icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_FROM] == laddr);
//    memcpy(conp->icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_FROM], laddr, sz);

    /* Instead of using the pre-mmaped memory (comp->icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_FROM]),
     * we allocate a memory. */
    void *buf_from = MPID_nem_ib_rdmawr_from_alloc(sz);
    memcpy(buf_from, laddr, sz);
    dprintf("put_scratch_pad,rdmawr_from_alloc=%p,sz=%d\n", buf_from, sz);
    struct ibv_mr *mr_rdmawr_from = MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_MR(buf_from);

    *buf_from_out = buf_from;
    *buf_from_sz_out = sz;

    void *from = (uint8_t *) buf_from;

#ifdef HAVE_LIBDCFA
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].sg_list[0].mic_addr = (uint64_t) from;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].sg_list[0].addr =
        mr_rdmawr_from->host_addr + ((uint64_t) from - (uint64_t) from);
#else
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].sg_list[0].addr = (uint64_t) from;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].sg_list[0].length = sz;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].sg_list[0].lkey = mr_rdmawr_from->lkey;

    /* num_sge is defined in MPID_nem_ib_com_open */
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].wr_id = wr_id;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].wr.rdma.remote_addr =
        (uint64_t) conp->icom_rmem[MPID_NEM_IB_COM_SCRATCH_PAD_TO] + offset;
    /* rkey is defined in MPID_nem_ib_com_reg_mr_connect */

    dprintf("MPID_nem_ib_com_put_scratch_pad,wr.rdma.remote_addr=%llx\n",
            (unsigned long long) conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].wr.
            rdma.remote_addr);

#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR]);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_put_scratch_pad, ibv_post_send, rc=%d\n",
                                    ib_errno));
#else
    ib_errno =
        ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR],
                      &bad_wr);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_put_scratch_pad, ibv_post_send, rc=%d, bad_wr=%p\n",
                                    ib_errno, bad_wr));
#endif

    conp->ncom_scratch_pad += 1;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_get_scratch_pad(int condesc,
                                    uint64_t wr_id,
                                    uint64_t offset, int sz,
                                    void **buf_from_out, uint32_t * buf_from_sz_out)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    struct ibv_send_wr *bad_wr;
    int ib_errno;

    dprintf("MPID_nem_ib_com_get_scratch_pad,enter,wr_id=%llx,offset=%llx,sz=%d\n",
            (unsigned long long) wr_id, (unsigned long long) offset, sz);

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    *buf_from_sz_out = sz;
    void *buf_from = MPID_nem_ib_rdmawr_from_alloc(sz);
    dprintf("get_scratch_pad,rdmawr_from_alloc=%p,sz=%d\n", buf_from, sz);
    *buf_from_out = buf_from;
    struct ibv_mr *mr_rdmawr_from = MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_MR(buf_from);

#ifdef HAVE_LIBDCFA
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].sg_list[0].mic_addr = (uint64_t) buf_from;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].sg_list[0].addr =
        mr_rdmawr_from->host_addr + ((uint64_t) buf_from - (uint64_t) buf_from);
#else
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].sg_list[0].addr = (uint64_t) buf_from;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].sg_list[0].length = sz;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].sg_list[0].lkey = mr_rdmawr_from->lkey;

    /* num_sge is defined in MPID_nem_ib_com_open */
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].wr_id = wr_id;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].wr.rdma.remote_addr =
        (uint64_t) conp->icom_rmem[MPID_NEM_IB_COM_SCRATCH_PAD_TO] + offset;
    /* rkey is defined in MPID_nem_ib_com_reg_mr_connect */

    dprintf("MPID_nem_ib_com_get_scratch_pad,wr.rdma.remote_addr=%llx\n",
            (unsigned long long) conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].wr.
            rdma.remote_addr);

#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET]);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_put_scratch_pad, ibv_post_send, rc=%d\n",
                                    ib_errno));
#else
    ib_errno =
        ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET], &bad_wr);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_put_scratch_pad, ibv_post_send, rc=%d, bad_wr=%p\n",
                                    ib_errno, bad_wr));
#endif

    conp->ncom_scratch_pad += 1;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_cas_scratch_pad(int condesc,
                                    uint64_t wr_id, uint64_t offset,
                                    uint64_t compare, uint64_t swap,
                                    void **buf_from_out, uint32_t * buf_from_sz_out)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    struct ibv_send_wr *bad_wr;
    int ib_errno;
    uint32_t sz = sizeof(uint64_t);

    dprintf("MPID_nem_ib_com_cas_scratch_pad,enter,wr_id=%llx,offset=%llx\n",
            (unsigned long long) wr_id, (unsigned long long) offset);

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    *buf_from_sz_out = sz;
    void *buf_from = MPID_nem_ib_rdmawr_from_alloc(sz);
    dprintf("cas_scratch_pad,rdmawr_from_alloc=%p,sz=%d\n", buf_from, sz);
    *buf_from_out = buf_from;
    struct ibv_mr *mr_rdmawr_from = MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_MR(buf_from);

#ifdef HAVE_LIBDCFA
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].sg_list[0].mic_addr = (uint64_t) buf_from;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].sg_list[0].addr =
        mr_rdmawr_from->host_addr + ((uint64_t) buf_from - (uint64_t) buf_from);
#else
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].sg_list[0].addr = (uint64_t) buf_from;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].sg_list[0].length = sizeof(uint64_t);
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].sg_list[0].lkey = mr_rdmawr_from->lkey;

    /* num_sge is defined in MPID_nem_ib_com_open */
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].wr_id = wr_id;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].wr.atomic.remote_addr =
        (uint64_t) conp->icom_rmem[MPID_NEM_IB_COM_SCRATCH_PAD_TO] + offset;
    /* atomic.rkey is defined in MPID_nem_ib_com_reg_mr_connect */
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].wr.atomic.compare_add = compare;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].wr.atomic.swap = swap;

    dprintf("MPID_nem_ib_com_cas_scratch_pad,wr.rdma.remote_addr=%llx\n",
            (unsigned long long) conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].wr.
            rdma.remote_addr);

#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS]);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_cas_scratch_pad, ibv_post_send, rc=%d\n",
                                    ib_errno));
#else
    ib_errno =
        ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS], &bad_wr);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_cas_scratch_pad, ibv_post_send, rc=%d, bad_wr=%p\n",
                                    ib_errno, bad_wr));
#endif

    conp->ncom_scratch_pad += 1;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_wr_scratch_pad(int condesc, uint64_t wr_id,
                                   void *buf_from, uint32_t buf_from_sz)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;
    struct ibv_send_wr *bad_wr;
    int ib_errno;

    dprintf("MPID_nem_ib_com_wr_scratch_pad,enter,wr_id=%llx,buf=%llx,sz=%d\n",
            (unsigned long long) wr_id, (unsigned long long) buf_from, buf_from_sz);

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    struct ibv_mr *mr_rdmawr_from = MPID_NEM_IB_RDMAWR_FROM_ALLOC_ARENA_MR(buf_from);

#ifdef HAVE_LIBDCFA
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].sg_list[0].mic_addr = (uint64_t) buf_from;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].sg_list[0].addr =
        mr_rdmawr_from->host_addr + ((uint64_t) buf_from - (uint64_t) buf_from);
#else
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].sg_list[0].addr = (uint64_t) buf_from;
#endif
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].sg_list[0].length = buf_from_sz;
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].sg_list[0].lkey = mr_rdmawr_from->lkey;

    /* num_sge is defined in MPID_nem_ib_com_open */
    conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].wr_id = wr_id;

    dprintf("MPID_nem_ib_com_wr_scratch_pad,wr.rdma.remote_addr=%llx\n",
            (unsigned long long) conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR].wr.rdma.remote_addr);

#ifdef HAVE_LIBDCFA
    ib_errno = ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR]);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_wr_scratch_pad, ibv_post_send, rc=%d\n",
                                    ib_errno));
#else
    ib_errno =
        ibv_post_send(conp->icom_qp, &conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_WR], &bad_wr);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(ib_errno, -1,
                                   dprintf
                                   ("MPID_nem_ib_com_wr_scratch_pad, ibv_post_send, rc=%d, bad_wr=%p\n",
                                    ib_errno, bad_wr));
#endif

    conp->ncom_scratch_pad += 1;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

/* poll completion queue */
int MPID_nem_ib_com_poll_cq(int which_cq, struct ibv_wc *wc, int *result)
{
    int ibcom_errno = 0;

    switch (which_cq) {
    case MPID_NEM_IB_COM_RC_SHARED_RCQ:
        *result = ibv_poll_cq(MPID_nem_ib_rc_shared_rcq, 1, wc);
        break;
    case MPID_NEM_IB_COM_RC_SHARED_SCQ:
        *result = ibv_poll_cq(MPID_nem_ib_rc_shared_scq, 1, wc);
        break;
    case MPID_NEM_IB_COM_UD_SHARED_RCQ:
        *result = ibv_poll_cq(MPID_nem_ib_ud_shared_rcq, 1, wc);
        break;
    case MPID_NEM_IB_COM_UD_SHARED_SCQ:
        *result = ibv_poll_cq(MPID_nem_ib_ud_shared_scq, 1, wc);
        break;
    }

    if (*result < 0) {
        dprintf
            ("MPID_nem_ib_com_poll_cq,status=%08x,vendor_err=%08x,len=%d,opcode=%08x,wr_id=%016lx\n",
             wc->status, wc->vendor_err, wc->byte_len, wc->opcode, wc->wr_id);
        ibcom_errno = *result;
        goto fn_fail;
    }

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_reg_mr_connect(int condesc, void *rmem, int rkey)
{
    int ibcom_errno = 0;
    MPID_nem_ib_com_t *conp;
    int i;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    switch (conp->open_flag) {
    case MPID_NEM_IB_COM_OPEN_RC:
        conp->icom_rmem[MPID_NEM_IB_COM_RDMAWR_TO] = rmem;
        conp->icom_rkey[MPID_NEM_IB_COM_RDMAWR_TO] = rkey;
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr.rdma.rkey =
            conp->icom_rkey[MPID_NEM_IB_COM_RDMAWR_TO];
        for (i = 0; i < MPID_NEM_IB_COM_SMT_INLINE_NCHAIN; i++) {
            conp->icom_sr[MPID_NEM_IB_COM_SMT_INLINE_CHAINED0 + i].wr.rdma.rkey =
                conp->icom_rkey[MPID_NEM_IB_COM_RDMAWR_TO];
        }
        break;

    case MPID_NEM_IB_COM_OPEN_SCRATCH_PAD:
        conp->icom_rmem[MPID_NEM_IB_COM_SCRATCH_PAD_TO] = rmem;
        conp->icom_rkey[MPID_NEM_IB_COM_SCRATCH_PAD_TO] = rkey;
        conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_INITIATOR].wr.rdma.rkey =
            conp->icom_rkey[MPID_NEM_IB_COM_SCRATCH_PAD_TO];
        conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_GET].wr.rdma.rkey =
            conp->icom_rkey[MPID_NEM_IB_COM_SCRATCH_PAD_TO];
        conp->icom_sr[MPID_NEM_IB_COM_SCRATCH_PAD_CAS].wr.atomic.rkey =
            conp->icom_rkey[MPID_NEM_IB_COM_SCRATCH_PAD_TO];
        break;

    default:
        dprintf("invalid open_flag=%d\n", conp->open_flag);
        break;
    }

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

/* alloc_new_mr
   0: The new ring buffer is located in the same IB Memory Region as
      the previous ring buffer is located in.
      This happens when making the connection switch to smaller ring buffer.
   1: The new ring buffer is located in the new IB Memory Region
      This happens when memory area shrunk then has grown. */
int MPID_nem_ib_com_connect_ringbuf(int condesc,
                                    uint32_t ringbuf_type,
                                    void *start, int rkey, int nslot,
                                    MPIDI_VC_t * remote_vc, uint32_t alloc_new_mr)
{
    int ibcom_errno = 0;
    MPID_nem_ib_com_t *conp;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    conp->local_ringbuf_type = ringbuf_type;


    /* Address and size */
    conp->local_ringbuf_start = start;
    conp->local_ringbuf_nslot = nslot;
    switch (conp->local_ringbuf_type) {
    case MPID_NEM_IB_RINGBUF_EXCLUSIVE:
        /* Head and tail pointers */
        conp->sseq_num = 0;
        conp->lsr_seq_num_tail = -1;
        break;
    case MPID_NEM_IB_RINGBUF_SHARED:
        /* Mark as full to make the sender ask */
        conp->lsr_seq_num_tail = conp->sseq_num - conp->local_ringbuf_nslot;
        conp->remote_vc = remote_vc;
        break;
    default:
        printf("unknown ringbuf type");
        break;
    }
    if (alloc_new_mr) {
        conp->local_ringbuf_rkey = rkey;
        conp->icom_sr[MPID_NEM_IB_COM_SMT_NOINLINE].wr.rdma.rkey = rkey;
    }
    dprintf
        ("connect_ringbuf,ringbuf_type=%d,rkey=%08x,start=%p,nslot=%d,sseq_num=%d,lsr_seq_num_tail=%d,remote_vc=%p,alloc_new_mr=%d\n",
         conp->local_ringbuf_type, conp->local_ringbuf_rkey, conp->local_ringbuf_start,
         conp->local_ringbuf_nslot, conp->sseq_num, conp->lsr_seq_num_tail, conp->remote_vc,
         alloc_new_mr);

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_get_info_conn(int condesc, int key, void *out, uint32_t out_len)
{
    int ibcom_errno = 0;
    MPID_nem_ib_com_t *conp;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    switch (key) {
    case MPID_NEM_IB_COM_INFOKEY_QP_QPN:
        memcpy(out, &conp->icom_qp->qp_num, out_len);
        break;
    case MPID_NEM_IB_COM_INFOKEY_PORT_LID:
#ifdef HAVE_LIBDCFA
        dprintf("MPID_nem_ib_com_get_info_conn,lid=%04x\n", ib_ctx->lid);
        memcpy(out, &ib_ctx->lid, out_len);
#else
        dprintf("MPID_nem_ib_com_get_info_conn,lid=%04x\n", conp->icom_pattr.lid);
        memcpy(out, &conp->icom_pattr.lid, out_len);
#endif
        break;
    case MPID_NEM_IB_COM_INFOKEY_PORT_GID:
        memcpy(out, &conp->icom_gid, out_len);
        break;
    case MPID_NEM_IB_COM_INFOKEY_PATTR_MAX_MSG_SZ:{
#ifdef HAVE_LIBDCFA
            uint32_t max_msg_sz = 1073741824;   /* ConnectX-3 */
            memcpy(out, &max_msg_sz, out_len);
#else
            memcpy(out, &conp->icom_pattr.max_msg_sz, out_len);
#endif
            break;
        }
    default:
        ibcom_errno = -1;
        break;
    }
  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_get_info_mr(int condesc, int memid, int key, void *out, int out_len)
{
    int ibcom_errno = 0;
    MPID_nem_ib_com_t *conp;
    struct ibv_mr *mr;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(memid >= conp->icom_mrlen, -1,
                                   dprintf("MPID_nem_ib_com_get_info_mr,wrong mem_id=%d\n", memid));
    mr = conp->icom_mrlist[memid];

    switch (key) {
    case MPID_NEM_IB_COM_INFOKEY_MR_ADDR:
#ifdef HAVE_LIBDCFA
        /* host_addr is created by ibv_reg_mr in MPID_nem_ib_com_open, */
        /* ib_init read this host-addr, put it into KVS, the counter-party read it through KVS */
        memcpy(out, &mr->host_addr, out_len);
#else
        memcpy(out, &mr->addr, out_len);
#endif
        break;
    case MPID_NEM_IB_COM_INFOKEY_MR_LENGTH:{
#ifdef HAVE_LIBDCFA
            assert(out_len == sizeof(size_t));
            size_t length = mr->size;   /* type of mr->size is int */
            memcpy(out, &length, out_len);
#else
            memcpy(out, &mr->length, out_len);
#endif
            break;
        }
    case MPID_NEM_IB_COM_INFOKEY_MR_RKEY:
        memcpy(out, &mr->rkey, out_len);
        break;
    default:
        dprintf("MPID_nem_ib_com_get_info_mr,unknown key=%d\n", key);
        ibcom_errno = -1;
        break;
    }
  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_mem_rdmawr_from(int condesc, void **out)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    *out =
        (uint8_t *) conp->icom_mem[MPID_NEM_IB_COM_RDMAWR_FROM] +
        MPID_NEM_IB_COM_RDMABUF_SZSEG *
        ((uint16_t) (conp->sseq_num % MPID_NEM_IB_COM_RDMABUF_NSEG));

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

#if 0
int MPID_nem_ib_com_mem_rdmawr_to(int condesc, int seq_num, void **out)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    *out =
        (uint8_t *) conp->icom_mem[MPID_NEM_IB_COM_RDMAWR_TO] +
        MPID_NEM_IB_COM_RDMABUF_SZSEG * (seq_num % MPID_NEM_IB_COM_RDMABUF_NSEG);

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}
#endif

int MPID_nem_ib_com_mem_udwr_from(int condesc, void **out)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    *out = conp->icom_mem[MPID_NEM_IB_COM_UDWR_FROM];

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_mem_udwr_to(int condesc, void **out)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    *out = conp->icom_mem[MPID_NEM_IB_COM_UDWR_TO];

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_rdmabuf_occupancy_notify_rate_get(int condesc, int *notify_rate)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);

    switch (conp->rdmabuf_occupancy_notify_lstate) {
    case MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_HW:
        *notify_rate = MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_RATE_HW;
        break;
    case MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_LW:
        *notify_rate = MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_RATE_LW;
        break;
    default:
        ibcom_errno = -1;
        goto fn_fail;
        break;
    }

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_rdmabuf_occupancy_notify_rstate_get(int condesc, int **rstate)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    *rstate = &(conp->rdmabuf_occupancy_notify_rstate);

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_rdmabuf_occupancy_notify_lstate_get(int condesc, int **lstate)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    *lstate = &(conp->rdmabuf_occupancy_notify_lstate);

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_obtain_pointer(int condesc, MPID_nem_ib_com_t ** MPID_nem_ib_com)
{
    MPID_nem_ib_com_t *conp;
    int ibcom_errno = 0;

    MPID_NEM_IB_RANGE_CHECK_WITH_ERROR(condesc, conp);
    *MPID_nem_ib_com = conp;

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

#if 0
static void MPID_nem_ib_comShow(int condesc)
{
    MPID_nem_ib_com_t *conp;
    uint8_t *p;
    int i;

    MPID_NEM_IB_RANGE_CHECK(condesc, conp);
    fprintf(stdout, "qp_num = %d\n", conp->icom_qp->qp_num);
#ifdef HAVE_LIBDCFA
    fprintf(stdout, "lid    = %d\n", ib_ctx->lid);
#else
    fprintf(stdout, "lid    = %d\n", conp->icom_pattr.lid);
#endif
    p = (uint8_t *) & conp->icom_gid;
    fprintf(stdout, "gid    = %02x", p[0]);
    for (i = 1; i < 16; i++) {
        fprintf(stdout, ":%02x", p[i]);
    }
    fprintf(stdout, "\n");
}
#endif

static const char *strerror_tbl[] = {
    [0] = "zero",
    [1] = "one",
    [2] = "two",
    [3] = "three",
};

char *MPID_nem_ib_com_strerror(int err)
{
    char *r;
    if (-err > 3) {
        r = MPIU_Malloc(256);
        sprintf(r, "%d", -err);
        goto fn_exit;
    }
    else {
        r = (char *)strerror_tbl[-err];
    }
  fn_exit:
    return r;
    //fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_reg_mr(void *addr, long len, struct ibv_mr **mr,
                           enum ibv_access_flags additional_flags)
{
    int ibcom_errno = 0;
    int err = -1;
    dprintf("MPID_nem_ib_com_reg_mr,addr=%p,len=%ld,mr=%p\n", addr, len, mr);

    *mr =
        ibv_reg_mr(ib_pd, addr, len,
                   IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE |
                   IBV_ACCESS_REMOTE_READ | additional_flags);

    if (*mr == 0) {
        err = errno;    /* copy errno of ibv_reg_mr */
    }

    /* return the errno of ibv_reg_mr when error occurs */
    MPID_NEM_IB_COM_ERR_CHKANDJUMP(*mr == 0, err,
                                   dprintf("MPID_nem_ib_com_reg_mr,cannot register memory\n"));

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ib_com_dereg_mr(struct ibv_mr *mr)
{
    int ib_errno;
    int ibcom_errno = 0;

    if (!mr) {
        goto fn_exit;
    }

    ib_errno = ibv_dereg_mr(mr);
    if (ib_errno < 0) {
        fprintf(stderr, "cannot deregister memory\n");
        goto fn_fail;
    }
#ifdef HAVE_LIBDCFA
    dprintf("MPID_nem_ib_com_dereg_mr, addr=%p\n", mr->buf);
#else
    dprintf("MPID_nem_ib_com_dereg_mr, addr=%p\n", mr->addr);
#endif

  fn_exit:
    return ibcom_errno;
  fn_fail:
    goto fn_exit;
}
