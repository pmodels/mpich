/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "ibu.h"
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include "mpidi_ch3_impl.h"

#ifdef USE_IB_VAPI

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define IBU_ERROR_MSG_LENGTH       255
#define IBU_PACKET_SIZE            (1024 * 64)
#define IBU_PACKET_COUNT           128
#define IBU_NUM_PREPOSTED_RECEIVES (IBU_ACK_WATER_LEVEL*3)
#define IBU_MAX_POSTED_SENDS       8192
#define IBU_MAX_DATA_SEGMENTS      100
#define IBU_ACK_WATER_LEVEL        32

#ifdef MPICH_DBG_OUTPUT
static int g_num_received = 0;
static int g_num_sent = 0;
static int g_num_posted_receives = 0;
static int g_num_posted_sends = 0;
static int g_ps = 0;
static int g_pr = 0;
#endif

#if 0
typedef struct mem_node_t
{
    void *p;
    struct mem_node_t *next;
} mem_node_t;
mem_node_t *g_pList = NULL;

void add2list(void *p)
{
    mem_node_t *node;

    node = (mem_node_t*)malloc(sizeof(mem_node_t));
    node->p = p;
    node->next = g_pList;
    g_pList = node;
}

void checklist(void *p)
{
    mem_node_t *node;
    node = g_pList;
    while (node)
    {
	if (node->p == p)
	{
	    printf("pointer %p already in global list.\n", p);fflush(stdout);
	}
	node = node->next;
    }
}

void removefromlist(void *p)
{
    mem_node_t *trailer, *iter;
    iter = trailer = g_pList;
    while (iter)
    {
	if (iter->p == p)
	{
	    if (iter == g_pList)
	    {
		g_pList = g_pList->next;
		free(iter);
		return;
	    }
	    else
	    {
		trailer->next = iter->next;
		free(iter);
		return;
	    }
	}
	if (trailer != iter)
	{
	    trailer = trailer->next;
	}
	iter = iter->next;
    }
    printf("remove error: pointer %p not in global list.\n", p);fflush(stdout);
}

void *s_base = NULL;
int s_offset = 0;
void sanity_check_recv(VAPI_rr_desc_t *work_req)
{
    VAPI_sg_lst_entry_t *data;
    int i;

    for (i=0; i<work_req->sg_lst_len; i++)
    {
	data = &work_req->sg_lst_p[i];
	if (data->len < 1 || data->len > IBU_PACKET_SIZE)
	{
	    printf("ERROR: data[%d].len = %d\n", i, data->len);fflush(stdout);
	}
	if ((void*)data->addr < s_base)
	{
	    printf("ERROR: ptr %p < %p base\n", data->addr, s_base);fflush(stdout);
	}
	if (((char*)data->addr + data->len) > ((char*)s_base + s_offset))
	{
	    printf("ERROR: ptr %p + len %d > %p base + %d offset\n",
		   data->addr, data->len, s_base, s_offset);
	    fflush(stdout);
	} 
    }
}
void sanity_check_send(VAPI_sr_desc_t *work_req)
{
    VAPI_sg_lst_entry_t *data;
    int i;

    for (i=0; i<work_req->sg_lst_len; i++)
    {
	data = &work_req->sg_lst_p[i];
	if (data->len < 1 || data->len > IBU_PACKET_SIZE)
	{
	    printf("ERROR: data[%d].len = %d\n", i, data->len);fflush(stdout);
	}
	if ((void*)data->addr < s_base)
	{
	    printf("ERROR: ptr %p < %p base\n", data->addr, s_base);fflush(stdout);
	}
	if (((char*)data->addr + data->len) > ((char*)s_base + s_offset))
	{
	    printf("ERROR: ptr %p + len %d > %p base + %d offset\n",
		   data->addr, data->len, s_base, s_offset);
	    fflush(stdout);
	} 
    }
}
#endif

typedef struct ibuBlock_t
{
    struct ibuBlock_t *next;
    VAPI_mr_hndl_t handle;
    VAPI_lkey_t lkey;
    unsigned char data[IBU_PACKET_SIZE];
} ibuBlock_t;

typedef struct ibuQueue_t
{
    struct ibuQueue_t *next_q;
    ibuBlock_t *pNextFree;
    ibuBlock_t block[IBU_PACKET_COUNT];
} ibuQueue_t;

static int g_offset;
#define GETLKEY(p) (((ibuBlock_t*)((char *)p - g_offset))->lkey)

struct ibuBlockAllocator_struct
{
    void **pNextFree;
    void *(* alloc_fn)(size_t size);
    void (* free_fn)(void *p);
    struct ibuBlockAllocator_struct *pNextAllocation;
    unsigned int nBlockSize;
    int nCount, nIncrementSize;
};

typedef struct ibuBlockAllocator_struct * ibuBlockAllocator;

#ifdef HAVE_32BIT_POINTERS

typedef union ibu_work_id_handle_t
{
    u_int64_t id;
    struct ibu_data
    {
	u_int32_t ptr, mem;
    } data;
} ibu_work_id_handle_t;

#else

typedef struct ibu_work_id_handle_t
{
    void *ptr, *mem;
} ibu_work_id_handle_t;

ibuBlockAllocator g_workAllocator = NULL;

#endif

typedef int IBU_STATE;
#define IBU_READING    0x0001
#define IBU_WRITING    0x0010

typedef struct ibu_buffer_t
{
    int use_iov;
    unsigned int num_bytes;
    void *buffer;
    unsigned int bufflen;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int iovlen;
    int index;
    int total;
} ibu_buffer_t;

typedef struct ibu_unex_read_t
{
    void *mem_ptr, *mem_ptr_orig;
    unsigned char *buf;
    unsigned int length;
    struct ibu_unex_read_t *next;
} ibu_unex_read_t;

typedef struct ibu_num_written_node_t
{
    int num_bytes;
    struct ibu_num_written_node_t *next;
} ibu_num_written_node_t;

typedef struct ibu_state_t
{
    IBU_STATE state;
    VAPI_qp_hndl_t qp_handle;
    /*ibuBlockAllocator allocator;*/
    ibuQueue_t *allocator;

    IB_lid_t dlid;
    VAPI_qp_num_t qp_num, dest_qp_num;

    int closing;
    int pending_operations;
    /* read and write structures */
    ibu_buffer_t read;
    ibu_unex_read_t *unex_list;
    ibu_buffer_t write;
    int nAvailRemote, nUnacked;
    /* vc pointer */
    MPIDI_VC *vc_ptr;
    /*void *user_ptr;*/
    /* unexpected queue pointer */
    struct ibu_state_t *unex_finished_queue;
} ibu_state_t;

typedef struct IBU_Global {
    VAPI_hca_hndl_t  hca_handle;
    VAPI_pd_hndl_t   pd_handle;
    VAPI_hca_port_t  hca_port;
    int              port;
    VAPI_cqe_num_t   cq_size;
    IB_lid_t         lid;
    ibu_state_t *    unex_finished_list;
    int              error;
    char             err_msg[IBU_ERROR_MSG_LENGTH];
    /* hack to get around zero sized messages */
    void *           ack_mem_ptr;
    /*VAPI_mr_hndl_t   ack_mr_handle;*/
    VAPI_lkey_t      ack_lkey;
} IBU_Global;

IBU_Global IBU_Process;

typedef struct ibu_num_written_t
{
    void *mem_ptr;
    int length;
} ibu_num_written_t;

static ibu_num_written_t g_num_bytes_written_stack[IBU_MAX_POSTED_SENDS];
static int g_cur_write_stack_index = 0;

/* local prototypes */
static int ibui_post_receive(ibu_t ibu);
static int ibui_post_receive_unacked(ibu_t ibu);
#if 0
static int ibui_post_write(ibu_t ibu, void *buf, int len);
static int ibui_post_writev(ibu_t ibu, MPID_IOV *iov, int n);
#endif
static int ibui_post_ack_write(ibu_t ibu);

/* utility allocator functions */

static ibuBlockAllocator ibuBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(size_t size), void (* free_fn)(void *p));
static ibuQueue_t * ibuBlockAllocInitIB();
static int ibuBlockAllocFinalize(ibuBlockAllocator *p);
static int ibuBlockAllocFinalizeIB(ibuQueue_t *p);
static void * ibuBlockAlloc(ibuBlockAllocator p);
static void * ibuBlockAllocIB(ibuQueue_t *p);
static int ibuBlockFree(ibuBlockAllocator p, void *pBlock);
static int ibuBlockFreeIB(ibuQueue_t *p, void *pBlock);
static void *ib_malloc_register(size_t size, VAPI_mr_hndl_t *mhp, VAPI_lkey_t *lp, VAPI_rkey_t *rp);
static void ib_free_deregister(void *p);

static ibuBlockAllocator ibuBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(size_t size), void (* free_fn)(void *p))
{
    ibuBlockAllocator p;
    void **ppVoid;
    int i;

    p = alloc_fn( sizeof(struct ibuBlockAllocator_struct) + ((blocksize + sizeof(void**)) * count) );
    if (p == NULL)
    {
	return NULL;
    }

    p->alloc_fn = alloc_fn;
    p->free_fn = free_fn;
    p->nIncrementSize = incrementsize;
    p->pNextAllocation = NULL;
    p->nCount = count;
    p->nBlockSize = blocksize;
    p->pNextFree = (void**)(p + 1);

    ppVoid = (void**)(p + 1);
    for (i=0; i<count-1; i++)
    {
	*ppVoid = (void*)((char*)ppVoid + sizeof(void**) + blocksize);
	ppVoid = *ppVoid;
    }
    *ppVoid = NULL;

    return p;
}

static ibuQueue_t *ibuBlockAllocInitIB()
{
    ibuQueue_t *q;
    int i;
    ibuBlock_t b[2];
    VAPI_mr_hndl_t handle;
    VAPI_lkey_t lkey;
    VAPI_rkey_t rkey;

    q = (ibuQueue_t*)ib_malloc_register(sizeof(ibuQueue_t), &handle, &lkey, &rkey);
    if (q == NULL)
    {
	return NULL;
    }
    q->next_q = NULL;
    for (i=0; i<IBU_PACKET_COUNT; i++)
    {
	q->block[i].next = &q->block[i+1];
	q->block[i].handle = handle;
	q->block[i].lkey = lkey;
    }
    q->block[IBU_PACKET_COUNT-1].next = NULL;
    q->pNextFree = &q->block[0];
    g_offset = (char*)&b[1].data - (char*)&b[1];
    return q;
}

static int ibuBlockAllocFinalize(ibuBlockAllocator *p)
{
    if (*p == NULL)
	return 0;
    ibuBlockAllocFinalize(&(*p)->pNextAllocation);
    if ((*p)->free_fn != NULL)
	(*p)->free_fn(*p);
    *p = NULL;
    return 0;
}

static int ibuBlockAllocFinalizeIB(ibuQueue_t *p)
{
    if (p == NULL)
	return 0;
    ibuBlockAllocFinalizeIB(p->next_q);
    ib_free_deregister(p);
}

static void * ibuBlockAlloc(ibuBlockAllocator p)
{
    void *pVoid;
    
    if (p->pNextFree == NULL)
    {
	ibuBlockAllocator q;
	void **ppVoid;
	int i;

	q = p->alloc_fn( sizeof(struct ibuBlockAllocator_struct) + ((p->nBlockSize + sizeof(void**)) * p->nCount) );
	if (q == NULL)
	{
	    MPIU_DBG_PRINTF(("ibuBlockAlloc returning NULL\n"));
	    return NULL;
	}

	q->alloc_fn = p->alloc_fn;
	q->free_fn = p->free_fn;
	q->nIncrementSize = p->nIncrementSize;
	q->pNextAllocation = NULL;
	q->nCount = p->nCount;
	q->nBlockSize = p->nBlockSize;
	q->pNextFree = (void**)(q + 1);

	ppVoid = (void**)(q + 1);
	for (i=0; i<p->nCount-1; i++)
	{
	    *ppVoid = (void*)((char*)ppVoid + sizeof(void**) + p->nBlockSize);
	    ppVoid = *ppVoid;
	}
	*ppVoid = NULL;

	p->pNextAllocation = q;
	p->pNextFree = q->pNextFree;
    }

    pVoid = p->pNextFree + 1;
    p->pNextFree = *(p->pNextFree);

    return pVoid;
}

static void * ibuBlockAllocIB(ibuQueue_t *p)
{
    void *pVoid;

    if (p->pNextFree == NULL)
    {
	ibuQueue_t *q;
	int i;
	ibuBlock_t b[2];
	VAPI_mr_hndl_t handle;
	VAPI_lkey_t lkey;
	VAPI_rkey_t rkey;

	q = (ibuQueue_t*)ib_malloc_register(sizeof(ibuQueue_t), &handle, &lkey, &rkey);
	if (q == NULL)
	{
	    MPIU_DBG_PRINTF(("ibuBlockAllocIB returning NULL\n"));
	    return NULL;
	}
	q->next_q = NULL;
	for (i=0; i<IBU_PACKET_COUNT; i++)
	{
	    q->block[i].next = &q->block[i+1];
	    q->block[i].handle = handle;
	    q->block[i].lkey = lkey;
	}
	q->block[IBU_PACKET_COUNT-1].next = NULL;
	q->pNextFree = &q->block[0];
	g_offset = (char*)&b[1].data - (char*)&b[1];

	p->next_q = q;
	p->pNextFree = q->pNextFree;
    } 
    pVoid = p->pNextFree->data;
    p->pNextFree = p->pNextFree->next;

#if 0
    checklist(pVoid);
    add2list(pVoid);
#endif

    return pVoid;
}

static int ibuBlockFree(ibuBlockAllocator p, void *pBlock)
{
    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

    return 0;
}

static int ibuBlockFreeIB(ibuQueue_t *p, void *pBlock)
{
    ibuBlock_t *b;
#if 0
    removefromlist(pBlock);
    checklist(pBlock);
#endif

    b = (ibuBlock_t *)((char *)pBlock - g_offset);
    b->next = p->pNextFree;
    p->pNextFree = b;
    return 0;
}

/* utility ibu functions */

#undef FUNCNAME
#define FUNCNAME modifyQP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static VAPI_ret_t modifyQP( ibu_t ibu, VAPI_qp_state_t qp_state )
{
    VAPI_ret_t status;
    VAPI_qp_attr_t qp_attr;
    VAPI_qp_cap_t qp_cap;
    VAPI_qp_attr_mask_t qp_attr_mask;
    MPIDI_STATE_DECL(MPID_STATE_IBU_MODIFYQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_MODIFYQP);

    MPIU_DBG_PRINTF(("entering modifyQP\n"));
    if (qp_state == VAPI_INIT)
    {
	QP_ATTR_MASK_CLR_ALL(qp_attr_mask);
	qp_attr.qp_state = VAPI_INIT;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_STATE);
	qp_attr.pkey_ix = 0;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_PKEY_IX);
	qp_attr.port = IBU_Process.port;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_PORT);
	qp_attr.remote_atomic_flags = VAPI_EN_REM_WRITE | VAPI_EN_REM_READ;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_REMOTE_ATOMIC_FLAGS);
	MPIU_DBG_PRINTF(("moving to INIT on port %d\n", IBU_Process.port));
    }
    else if (qp_state == VAPI_RTR) 
    {
	QP_ATTR_MASK_CLR_ALL(qp_attr_mask);
	qp_attr.qp_state         = VAPI_RTR;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_STATE);
	qp_attr.qp_ous_rd_atom   = 1;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_OUS_RD_ATOM);
	qp_attr.path_mtu         = MTU1024;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_PATH_MTU);
	qp_attr.rq_psn           = 0;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_RQ_PSN);
	qp_attr.pkey_ix          = 0;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_PKEY_IX);
	qp_attr.min_rnr_timer    = 0;/*5;*/
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_MIN_RNR_TIMER);
	qp_attr.dest_qp_num = ibu->dest_qp_num;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_DEST_QP_NUM);
	qp_attr.av.sl            = 0;
	qp_attr.av.grh_flag      = 0;
	qp_attr.av.static_rate   = 0;
	qp_attr.av.src_path_bits = 0;
	qp_attr.av.dlid = ibu->dlid;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_AV);
	MPIU_DBG_PRINTF(("moving to RTR qp(%d:%d => %d:%d)\n", IBU_Process.lid, ibu->qp_num, ibu->dlid, ibu->dest_qp_num));
    }
    else if (qp_state == VAPI_RTS)
    {
	QP_ATTR_MASK_CLR_ALL(qp_attr_mask);
	qp_attr.qp_state         = VAPI_RTS;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_STATE);
	qp_attr.sq_psn           = 0;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_SQ_PSN);
	qp_attr.timeout          = /*0x20;*/ 10;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_TIMEOUT);
	qp_attr.retry_count      = 1; /*5;*/
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_RETRY_COUNT);
	qp_attr.rnr_retry        = 3; /*1;*/
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_RNR_RETRY);
	qp_attr.ous_dst_rd_atom  = 1; /*255;*/
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_OUS_DST_RD_ATOM);
	MPIU_DBG_PRINTF(("moving to RTS\n"));
    }
    else if (qp_state == VAPI_RESET)
    {
	QP_ATTR_MASK_CLR_ALL(qp_attr_mask);
	qp_attr.qp_state = VAPI_RESET;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_STATE);
    }
    else
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	return IBU_FAIL;
    }

    status = VAPI_modify_qp(IBU_Process.hca_handle, 
			     ibu->qp_handle, 
			    &qp_attr,
			    &qp_attr_mask, 
			    &qp_cap );
    if( status != VAPI_OK )
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	return status;
    }

    MPIU_DBG_PRINTF(("exiting modifyQP\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
    return IBU_SUCCESS;
}

static VAPI_ret_t createQP(ibu_t ibu, ibu_set_t set)
{
    VAPI_ret_t status;
    VAPI_qp_init_attr_t qp_init_attr;
    VAPI_qp_prop_t qp_prop;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATEQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATEQP);
    MPIU_DBG_PRINTF(("entering createQP\n"));
    qp_init_attr.cap.max_oust_wr_rq = 10000; /*DEFAULT_MAX_WQE;*/
    qp_init_attr.cap.max_oust_wr_sq = 10000; /*DEFAULT_MAX_WQE;*/
    qp_init_attr.cap.max_sg_size_rq = 8;
    qp_init_attr.cap.max_sg_size_sq = 8;
    qp_init_attr.pd_hndl = IBU_Process.pd_handle;
    qp_init_attr.rdd_hndl = 0;
    qp_init_attr.rq_cq_hndl = set;
    qp_init_attr.sq_cq_hndl = set;
    qp_init_attr.rq_sig_type = /*VAPI_SIGNAL_ALL_WR;*/ VAPI_SIGNAL_REQ_WR;
    qp_init_attr.sq_sig_type = /*VAPI_SIGNAL_ALL_WR;*/ VAPI_SIGNAL_REQ_WR;
    qp_init_attr.ts_type = IB_TS_RC;

    status = VAPI_create_qp(IBU_Process.hca_handle, &qp_init_attr, &ibu->qp_handle, &qp_prop);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("VAPI_create_qp failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
	return status;
    }
    ibu->qp_num = qp_prop.qp_num;
    MPIU_DBG_PRINTF(("exiting createQP\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ib_malloc_register
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void *ib_malloc_register(size_t size, VAPI_mr_hndl_t *mhp, VAPI_lkey_t *lp, VAPI_rkey_t *rp)
{
    VAPI_ret_t status;
    void *ptr;
    VAPI_mrw_t mem, mem_out;
    MPIDI_STATE_DECL(MPID_STATE_IB_MALLOC_REGISTER);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MALLOC_REGISTER);

    MPIU_DBG_PRINTF(("entering ib_malloc_register\n"));

    /*printf("ib_malloc_register(%d) called\n", size);*/

    ptr = MPIU_Malloc(size);
    if (ptr == NULL)
    {
	MPIU_Internal_error_printf("ib_malloc_register: MPIU_Malloc(%d) failed.\n", size);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
	return NULL;
    }
    memset(&mem, 0, sizeof(VAPI_mrw_t));
    memset(&mem_out, 0, sizeof(VAPI_mrw_t));
    mem.type = VAPI_MR;
    mem.start = (VAPI_virt_addr_t)ptr;
    mem.size = size;
    mem.pd_hndl = IBU_Process.pd_handle;
    mem.acl = VAPI_EN_LOCAL_WRITE | VAPI_EN_REMOTE_WRITE;
    mem.l_key = 0;
    mem.r_key = 0;
    status = VAPI_register_mr(
	IBU_Process.hca_handle,
	&mem,
	mhp,
	&mem_out);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ib_malloc_register: VAPI_register_mr failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
	return NULL;
    }
    *lp = mem_out.l_key;
    *rp = mem_out.r_key;

/*
    s_mr_handle = *mhp;
    s_lkey = mem_out.l_key;
*/
#if 0
    s_base = ptr;
    s_offset = size;
#endif

    MPIU_DBG_PRINTF(("exiting ib_malloc_register\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
    return ptr;
}

#undef FUNCNAME
#define FUNCNAME ib_free_deregister
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void ib_free_deregister(void *p)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_FREE_DEREGISTER);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_FREE_DEREGISTER);
    MPIU_DBG_PRINTF(("entering ib_free_derigister\n"));
    MPIU_Free(p);
    MPIU_DBG_PRINTF(("exiting ib_free_derigster\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IB_FREE_DEREGISTER);
}

#undef FUNCNAME
#define FUNCNAME ibu_start_qp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ibu_t ibu_start_qp(ibu_set_t set, int *qp_num_ptr)
{
    VAPI_ret_t status;
    ibu_t p;
    MPIDI_STATE_DECL(MPID_STATE_IBU_START_QP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_START_QP);
    MPIU_DBG_PRINTF(("entering ibu_start_qp\n"));
    p = (ibu_t)MPIU_Malloc(sizeof(ibu_state_t));
    if (p == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_START_QP);
	return NULL;
    }

    memset(p, 0, sizeof(ibu_state_t));
    p->state = 0;
    p->allocator = ibuBlockAllocInitIB(); /*IBU_PACKET_SIZE, IBU_PACKET_COUNT,
				     IBU_PACKET_COUNT,
				     ib_malloc_register, ib_free_deregister);*/

    /*MPIDI_DBG_PRINTF((60, FCNAME, "creating the queue pair\n"));*/
    /* Create the queue pair */
    status = createQP(p, set);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_create_qp: createQP failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_START_QP);
	return NULL;
    }
    *qp_num_ptr = p->qp_num;
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_START_QP);
    return p;
}

#undef FUNCNAME
#define FUNCNAME ibu_finish_qp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_finish_qp(ibu_t p, int dest_lid, int dest_qp_num)
{
    VAPI_ret_t status;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_IBU_FINISH_QP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_FINISH_QP);
    MPIU_DBG_PRINTF(("entering ibu_finish_qp\n"));


    p->dest_qp_num = dest_qp_num;
    p->dlid = dest_lid;
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(INIT)"));*/
    status = modifyQP(p, VAPI_INIT);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_create_qp: modifyQP(INIT) failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(RTR)"));*/
    status = modifyQP(p, VAPI_RTR);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_create_qp: modifyQP(RTR) failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }

    status = modifyQP(p, VAPI_RTS);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_create_qp: modifyQP(RTS) failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }

    /* pre post some receives on each connection */
    p->nAvailRemote = 0;
    p->nUnacked = 0;
    for (i=0; i<IBU_NUM_PREPOSTED_RECEIVES; i++)
    {
	ibui_post_receive_unacked(p);
	p->nAvailRemote++; /* assumes the other side is executing this same code */
    }
    p->nAvailRemote -= 5; /* remove one from nAvailRemote so a ack packet can always get through */

    MPIU_DBG_PRINTF(("exiting ibu_create_qp\n"));    
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_post_receive_unacked
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_receive_unacked(ibu_t ibu)
{
    VAPI_ret_t status;
    VAPI_sg_lst_entry_t data;
    VAPI_rr_desc_t work_req;
    void *mem_ptr;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);

    MPIU_DBG_PRINTF(("entering ibui_post_receive_unacked\n"));
    mem_ptr = ibuBlockAllocIB(ibu->allocator);
    if (mem_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);
	return IBU_FAIL;
    }

#ifdef HAVE_32BIT_POINTERS
    ((ibu_work_id_handle_t*)&work_req.id)->data.ptr = (u_int32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.id)->data.mem = (u_int32_t)mem_ptr;
#else
    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
    *((ibu_work_id_handle_t**)&work_req.id) = id_ptr;
    if (id_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);
	return IBU_FAIL;
    }
    id_ptr->ptr = (void*)ibu;
    id_ptr->mem = (void*)mem_ptr;
#endif
    work_req.opcode = VAPI_RECEIVE;
    work_req.comp_type = VAPI_SIGNALED;
    work_req.sg_lst_p = &data;
    work_req.sg_lst_len = 1;
    data.addr = (VAPI_virt_addr_t)(MT_virt_addr_t)mem_ptr;
    data.len = IBU_PACKET_SIZE;
    data.lkey = GETLKEY(mem_ptr);
    /*assert(data.lkey == s_lkey);*/

    /*sanity_check_recv(&work_req);*/
    MPIDI_DBG_PRINTF((60, FCNAME, "calling VAPI_post_rr"));
    status = VAPI_post_rr(IBU_Process.hca_handle, 
				ibu->qp_handle,
				&work_req);
    if (status != VAPI_OK)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	MPIU_Internal_error_printf("%s: Error: failed to post ib receive, status = %s\n", FCNAME, VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);
	return status;
    }
    MPIU_DBG_PRINTF(("", g_pr++));
    MPIU_DBG_PRINTF(("___posted_r: %d\n", ++g_num_posted_receives));

    MPIU_DBG_PRINTF(("exiting ibui_post_receive_unacked\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_post_receive
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_receive(ibu_t ibu)
{
    VAPI_ret_t status;
    VAPI_sg_lst_entry_t data;
    VAPI_rr_desc_t work_req;
    void *mem_ptr;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_RECEIVE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_RECEIVE);

    MPIU_DBG_PRINTF(("entering ibui_post_receive\n"));
    mem_ptr = ibuBlockAllocIB(ibu->allocator);
    if (mem_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
	return IBU_FAIL;
    }

#ifdef HAVE_32BIT_POINTERS
    ((ibu_work_id_handle_t*)&work_req.id)->data.ptr = (u_int32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.id)->data.mem = (u_int32_t)mem_ptr;
#else
    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
    *((ibu_work_id_handle_t**)&work_req.id) = id_ptr;
    if (id_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
	return IBU_FAIL;
    }
    id_ptr->ptr = (void*)ibu;
    id_ptr->mem = (void*)mem_ptr;
#endif
    work_req.opcode = VAPI_RECEIVE;
    work_req.comp_type = VAPI_SIGNALED;
    work_req.sg_lst_p = &data;
    work_req.sg_lst_len = 1;
    data.addr = (VAPI_virt_addr_t)mem_ptr;
    data.len = IBU_PACKET_SIZE;
    data.lkey = GETLKEY(mem_ptr);
    /*assert(data.lkey == s_lkey);*/

    /*sanity_check_recv(&work_req);*/
    MPIDI_DBG_PRINTF((60, FCNAME, "calling VAPI_post_rr"));
    status = VAPI_post_rr(IBU_Process.hca_handle, 
				ibu->qp_handle,
				&work_req);
    if (status != VAPI_OK)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	MPIU_Internal_error_printf("%s: Error: failed to post ib receive, status = %s\n", FCNAME, VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
	return status;
    }
    MPIU_DBG_PRINTF(("", g_pr++));
    MPIU_DBG_PRINTF(("___posted_r: %d\n", ++g_num_posted_receives));
    if (++ibu->nUnacked > IBU_ACK_WATER_LEVEL)
    {
	MPIU_DBG_PRINTF(("nUnacked = %d, sending ack.\n", ibu->nUnacked));
	ibui_post_ack_write(ibu);
    }
#ifdef MPICH_DBG_OUTPUT
    else
    {
	MPIU_DBG_PRINTF(("nUnacked = %d\n", ibu->nUnacked));
    }
#endif

    MPIU_DBG_PRINTF(("exiting ibui_post_receive\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_post_ack_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_ack_write(ibu_t ibu)
{
    VAPI_ret_t status;
    VAPI_sr_desc_t work_req;
    VAPI_sg_lst_entry_t data;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_ACK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_ACK_WRITE);

    MPIU_DBG_PRINTF(("entering ibui_post_ack_write\n"));
    if (IBU_Process.ack_mem_ptr == NULL)
    {
	IBU_Process.ack_mem_ptr = ibuBlockAllocIB(ibu->allocator);
	IBU_Process.ack_lkey = GETLKEY(IBU_Process.ack_mem_ptr);
    }
    data.len = 1;
    data.addr = (VAPI_virt_addr_t)(MT_virt_addr_t)IBU_Process.ack_mem_ptr;
    data.lkey = IBU_Process.ack_lkey;

    work_req.opcode = VAPI_SEND_WITH_IMM;
    work_req.comp_type = VAPI_SIGNALED;
    work_req.sg_lst_p = &data;/*NULL;*/
    work_req.sg_lst_len = 1;/*0;*/
    work_req.imm_data = ibu->nUnacked;
    work_req.fence = 0;
    work_req.remote_ah = 0;
    work_req.remote_qp = 0;
    work_req.remote_qkey = 0;
    work_req.ethertype = 0;
    work_req.eecn = 0;
    work_req.set_se = 0;
    work_req.remote_addr = 0;
    work_req.r_key = 0;
    work_req.compare_add = 0;
    work_req.swap = 0;
#ifdef HAVE_32BIT_POINTERS
    ((ibu_work_id_handle_t*)&work_req.id)->data.ptr = (u_int32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.id)->data.mem = (u_int32_t)-1;
#else
    work_req.id = (u_int64_t)ibuBlockAlloc(g_workAllocator);
    if ((void*)work_req.id == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
	return IBU_FAIL;
    }
    ((ibu_work_id_handle_t*)work_req.id)->ptr = (void*)ibu;
    ((ibu_work_id_handle_t*)work_req.id)->mem = (void*)-1;
#endif

    /*sanity_check_send(&work_req);*/
    MPIDI_DBG_PRINTF((60, FCNAME, "VAPI_post_sr(%d byte ack)", ibu->nUnacked));
    status = VAPI_post_sr( IBU_Process.hca_handle,
	ibu->qp_handle, 
	&work_req);
    if (status != VAPI_OK)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
	return status;
    }
    MPIU_DBG_PRINTF(("", g_ps++));
    MPIU_DBG_PRINTF(("___posted_s: %d\n", ++g_num_posted_sends));
    ibu->nUnacked = 0;

    MPIU_DBG_PRINTF(("exiting ibui_post_ack_write\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
    return IBU_SUCCESS;
}

/* ibu functions */

#undef FUNCNAME
#define FUNCNAME ibu_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_write(ibu_t ibu, void *buf, int len, int *num_bytes_ptr)
{
    VAPI_ret_t status;
    VAPI_sg_lst_entry_t data;
    VAPI_sr_desc_t work_req;
    void *mem_ptr;
    int length;
    int total = 0;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITE);
    MPIU_DBG_PRINTF(("entering ibu_write\n"));
    while (len)
    {
	length = min(len, IBU_PACKET_SIZE);
	len -= length;

	if (ibu->nAvailRemote < 1)
	{
	    /*printf("ibu_write: no remote packets available\n");fflush(stdout);*/
	    MPIDI_DBG_PRINTF((60, FCNAME, "no more remote packets available"));
	    *num_bytes_ptr = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return IBU_SUCCESS;
	}

	mem_ptr = ibuBlockAllocIB(ibu->allocator);
	if (mem_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAlloc returned NULL\n"));
	    *num_bytes_ptr = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return IBU_SUCCESS;
	}
	memcpy(mem_ptr, buf, length);
	total += length;
	
	MPIDI_DBG_PRINTF((60, FCNAME, "g_write_stack[%d].length = %d\n", g_cur_write_stack_index, length));
	g_num_bytes_written_stack[g_cur_write_stack_index].length = length;
	g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr = mem_ptr;
	g_cur_write_stack_index++;

	data.len = length;
	data.addr = (VAPI_virt_addr_t)(MT_virt_addr_t)mem_ptr;
	data.lkey = GETLKEY(mem_ptr);
	/*assert(data.lkey == s_lkey);*/
	
	work_req.opcode = VAPI_SEND;
	work_req.comp_type = VAPI_SIGNALED;
	work_req.sg_lst_p = &data;
	work_req.sg_lst_len = 1;
	work_req.imm_data = 0;
	work_req.fence = 0;
	work_req.remote_ah = 0;
	work_req.remote_qp = 0;
	work_req.remote_qkey = 0;
	work_req.ethertype = 0;
	work_req.eecn = 0;
	work_req.set_se = 0;
	work_req.remote_addr = 0;
	work_req.r_key = 0;
	work_req.compare_add = 0;
	work_req.swap = 0;

#ifdef HAVE_32BIT_POINTERS
	/* store the ibu ptr and the mem ptr in the work id */
	((ibu_work_id_handle_t*)&work_req.id)->data.ptr = (u_int32_t)ibu;
	((ibu_work_id_handle_t*)&work_req.id)->data.mem = (u_int32_t)mem_ptr;
#else
	id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
	*((ibu_work_id_handle_t**)&work_req.id) = id_ptr;
	if (id_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return IBU_FAIL;
	}
	id_ptr->ptr = (void*)ibu;
	id_ptr->mem = (void*)mem_ptr;
#endif

	/*sanity_check_send(&work_req);*/
	MPIDI_DBG_PRINTF((60, FCNAME, "calling VAPI_post_sr(%d bytes)", length));
	status = VAPI_post_sr( IBU_Process.hca_handle,
	    ibu->qp_handle, 
	    &work_req);
	if (status != VAPI_OK)
	{
	    MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	    MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, VAPI_strerror(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return -1;
	}
	MPIU_DBG_PRINTF(("", g_ps++));
	MPIU_DBG_PRINTF(("___posted_s: %d\n", ++g_num_posted_sends));
	ibu->nAvailRemote--;
	MPIU_DBG_PRINTF(("send posted, nAvailRemote: %d, nUnacked: %d\n", ibu->nAvailRemote, ibu->nUnacked));

	buf = (char*)buf + length;
    }

    *num_bytes_ptr = total;

    MPIU_DBG_PRINTF(("exiting ibu_write\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_writev(ibu_t ibu, MPID_IOV *iov, int n, int *num_bytes_ptr)
{
    VAPI_ret_t status;
    VAPI_sg_lst_entry_t data;
    VAPI_sr_desc_t work_req;
    void *mem_ptr;
    unsigned int len, msg_size;
    int total = 0;
    unsigned int num_avail;
    unsigned char *buf;
    int cur_index;
    unsigned int cur_len;
    unsigned char *cur_buf;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITEV);
    MPIU_DBG_PRINTF(("entering ibu_writev\n"));

    cur_index = 0;
    cur_len = iov[0].MPID_IOV_LEN;
    cur_buf = iov[0].MPID_IOV_BUF;
    do
    {
	if (ibu->nAvailRemote < 1)
	{
	    /*printf("ibu_writev: no remote packets available\n");fflush(stdout);*/
	    MPIDI_DBG_PRINTF((60, FCNAME, "no more remote packets available."));
	    *num_bytes_ptr = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_SUCCESS;
	}
	mem_ptr = ibuBlockAllocIB(ibu->allocator);
	if (mem_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAlloc returned NULL."));
	    *num_bytes_ptr = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_SUCCESS;
	}
	buf = mem_ptr;
	num_avail = IBU_PACKET_SIZE;
	/*MPIU_DBG_PRINTF(("iov length: %d\n", n));*/
	for (; cur_index < n && num_avail; )
	{
	    len = min (num_avail, cur_len);
	    num_avail -= len;
	    total += len;
	    /*MPIU_DBG_PRINTF(("copying %d bytes to ib buffer - num_avail: %d\n", len, num_avail));*/
	    memcpy(buf, cur_buf, len);
	    buf += len;
	    
	    if (cur_len == len)
	    {
		cur_index++;
		cur_len = iov[cur_index].MPID_IOV_LEN;
		cur_buf = iov[cur_index].MPID_IOV_BUF;
	    }
	    else
	    {
		cur_len -= len;
		cur_buf += len;
	    }
	}
	msg_size = IBU_PACKET_SIZE - num_avail;
	
	MPIDI_DBG_PRINTF((60, FCNAME, "g_write_stack[%d].length = %d\n", g_cur_write_stack_index, msg_size));
	g_num_bytes_written_stack[g_cur_write_stack_index].length = msg_size;
	g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr = mem_ptr;
	g_cur_write_stack_index++;

	data.len = msg_size;
	assert(data.len);
	data.addr = (VAPI_virt_addr_t)mem_ptr;
	data.lkey = GETLKEY(mem_ptr);
	/*assert(data.lkey == s_lkey);*/
	
	work_req.opcode = VAPI_SEND;
	work_req.comp_type = VAPI_SIGNALED;
	work_req.sg_lst_p = &data;
	work_req.sg_lst_len = 1;
	work_req.imm_data = 0;
	work_req.fence = 0;
	work_req.remote_ah = 0;
	work_req.remote_qp = 0;
	work_req.remote_qkey = 0;
	work_req.ethertype = 0;
	work_req.eecn = 0;
	work_req.set_se = 0;
	work_req.remote_addr = 0;
	work_req.r_key = 0;
	work_req.compare_add = 0;
	work_req.swap = 0;
	
#ifdef HAVE_32BIT_POINTERS
	/* store the ibu ptr and the mem ptr in the work id */
	((ibu_work_id_handle_t*)&work_req.id)->data.ptr = (u_int32_t)ibu;
	((ibu_work_id_handle_t*)&work_req.id)->data.mem = (u_int32_t)mem_ptr;
#else
	id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
	*((ibu_work_id_handle_t**)&work_req.id) = id_ptr;
	if (id_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_FAIL;
	}
	id_ptr->ptr = (void*)ibu;
	id_ptr->mem = (void*)mem_ptr;
#endif

	/*sanity_check_send(&work_req);*/
	MPIDI_DBG_PRINTF((60, FCNAME, "VAPI_post_sr(%d bytes)", msg_size));
	status = VAPI_post_sr( IBU_Process.hca_handle,
	    ibu->qp_handle, 
	    &work_req);
	if (status != VAPI_OK)
	{
	    MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	    MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, VAPI_strerror(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_FAIL;
	}
	MPIU_DBG_PRINTF(("", g_ps++));
	MPIU_DBG_PRINTF(("___posted_s: %d\n", ++g_num_posted_sends));
	ibu->nAvailRemote--;
	MPIU_DBG_PRINTF(("send posted, nAvailRemote: %d, nUnacked: %d\n", ibu->nAvailRemote, ibu->nUnacked));

    } while (cur_index < n);

    *num_bytes_ptr = total;

    MPIU_DBG_PRINTF(("exiting ibu_writev\n"));    
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_init()
{
    VAPI_ret_t status;
    EVAPI_hca_profile_t sugg_profile;
    VAPI_hca_id_t id = "InfiniHost0";
    VAPI_hca_vendor_t vendor;
    VAPI_hca_cap_t hca_cap;
    VAPI_rkey_t rkey;
    MPIDI_STATE_DECL(MPID_STATE_IBU_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_INIT);
    MPIU_DBG_PRINTF(("entering ibu_init\n"));

    /* Initialize globals */
    status = EVAPI_get_hca_hndl(id, &IBU_Process.hca_handle);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_init: EVAPI_get_hca_hndl failed, status %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    status = VAPI_query_hca_cap(IBU_Process.hca_handle, &vendor, &hca_cap);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_init: VAPI_query_hca_cap failed, status %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    IBU_Process.port = 1;
    IBU_Process.cq_size = hca_cap.max_num_ent_cq;
    MPIU_DBG_PRINTF(("cq size: %d\n", IBU_Process.cq_size));
    /* get a protection domain handle */
    status = VAPI_alloc_pd(IBU_Process.hca_handle, &IBU_Process.pd_handle);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_init: VAPI_alloc_pd failed, status %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }

    /* get the lid */
    status = VAPI_query_hca_port_prop(IBU_Process.hca_handle,
				      (IB_port_t)1, &IBU_Process.hca_port);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_init: VAPI_query_hca_port_prop failed, status %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    IBU_Process.lid = IBU_Process.hca_port.lid;

    /*
    MPIU_DBG_PRINTF(("infiniband:\n mtu: %d\n msg_size: %d\n",
	IBU_Process.attr_p->port_static_info_p->mtu,
	IBU_Process.attr_p->port_static_info_p->msg_size));
    */

    /*IBU_Process.ack_mem_ptr = ib_malloc_register(4096, &IBU_Process.ack_mr_handle, &IBU_Process.ack_lkey, &rkey);*/
    IBU_Process.ack_mem_ptr = NULL;

    /* non infiniband initialization */
    IBU_Process.unex_finished_list = NULL;
#ifndef HAVE_32BIT_POINTERS
    g_workAllocator = ibuBlockAllocInit(sizeof(ibu_work_id_handle_t), 256, 256, malloc, free);
#endif
    MPIU_DBG_PRINTF(("exiting ibu_init\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_FINALIZE);
    MPIU_DBG_PRINTF(("entering ibu_finalize\n"));
#ifdef HAVE_32BIT_POINTERS
    ibuBlockAllocFinalize(&g_workAllocator);
#endif
    MPIU_DBG_PRINTF(("exiting ibu_finalize\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINALIZE);
    return IBU_SUCCESS;
}

void FooBar(VAPI_hca_hndl_t hca_handle, VAPI_cq_hndl_t cq_handle, void *p)
{
    MPIU_Error_printf("Help me I'm drowning\n");
    fflush(stdout);
}

void FooBar2(VAPI_hca_hndl_t hca_handle, VAPI_event_record_t *event, void *p)
{
    MPIU_Error_printf("Help me I'm drowning in events\n");
    fflush(stdout);
}

#undef FUNCNAME
#define FUNCNAME ibu_create_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_create_set(ibu_set_t *set)
{
    VAPI_ret_t status;
    VAPI_cqe_num_t max_cq_entries = IBU_Process.cq_size;
    EVAPI_compl_handler_hndl_t ch;
    EVAPI_async_handler_hndl_t ah;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATE_SET);
    MPIU_DBG_PRINTF(("entering ibu_create_set\n"));
    /* create the completion queue */
    status = VAPI_create_cq(
	IBU_Process.hca_handle, 
	IBU_Process.cq_size,
	set,
	&max_cq_entries);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_create_set: VAPI_create_cq failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
	return status;
    }

    MPIU_DBG_PRINTF(("exiting ibu_create_set\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
    return status;
}

#undef FUNCNAME
#define FUNCNAME ibu_destroy_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_destroy_set(ibu_set_t set)
{
    VAPI_ret_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_DESTROY_SET);
    MPIU_DBG_PRINTF(("entering ibu_destroy_set\n"));
    status = VAPI_destroy_cq(IBU_Process.hca_handle, set);
    MPIU_DBG_PRINTF(("exiting ibu_destroy_set\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_DESTROY_SET);
    return status;
}

#undef FUNCNAME
#define FUNCNAME ibui_buffer_unex_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_buffer_unex_read(ibu_t ibu, void *mem_ptr, unsigned int offset, unsigned int num_bytes)
{
    ibu_unex_read_t *p;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_BUFFER_UNEX_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_BUFFER_UNEX_READ);
    MPIU_DBG_PRINTF(("entering ibui_buffer_unex_read\n"));

    MPIDI_DBG_PRINTF((60, FCNAME, "%d bytes\n", num_bytes));

    p = (ibu_unex_read_t *)MPIU_Malloc(sizeof(ibu_unex_read_t));
    p->mem_ptr = mem_ptr;
    p->buf = (unsigned char *)mem_ptr + offset;
    p->length = num_bytes;
    p->next = ibu->unex_list;
    ibu->unex_list = p;

    MPIU_DBG_PRINTF(("exiting ibui_buffer_unex_read\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_BUFFER_UNEX_READ);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_read_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_read_unex(ibu_t ibu)
{
    unsigned int len;
    ibu_unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_READ_UNEX);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_READ_UNEX);
    MPIU_DBG_PRINTF(("entering ibui_read_unex\n"));

    assert(ibu->unex_list);

    /* copy the received data */
    while (ibu->unex_list)
    {
	len = min(ibu->unex_list->length, ibu->read.bufflen);
	memcpy(ibu->read.buffer, ibu->unex_list->buf, len);
	/* advance the user pointer */
	ibu->read.buffer = (char*)(ibu->read.buffer) + len;
	ibu->read.bufflen -= len;
	ibu->read.total += len;
	if (len != ibu->unex_list->length)
	{
	    ibu->unex_list->length -= len;
	    ibu->unex_list->buf += len;
	}
	else
	{
	    /* put the receive packet back in the pool */
	    if (ibu->unex_list->mem_ptr == NULL)
	    {
		MPIU_Internal_error_printf("ibui_read_unex: mem_ptr == NULL\n");
	    }
	    assert(ibu->unex_list->mem_ptr != NULL);
	    ibuBlockFreeIB(ibu->allocator, ibu->unex_list->mem_ptr);
	    /* MPIU_Free the unexpected data node */
	    temp = ibu->unex_list;
	    ibu->unex_list = ibu->unex_list->next;
	    MPIU_Free(temp);
	    /* post another receive to replace the consumed one */
	    ibui_post_receive(ibu);
	}
	/* check to see if the entire message was received */
	if (ibu->read.bufflen == 0)
	{
	    /* place this ibu in the finished list so it will be completed by ibu_wait */
	    ibu->state &= ~IBU_READING;
	    ibu->unex_finished_queue = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = ibu;
	    /* post another receive to replace the consumed one */
	    /*ibui_post_receive(ibu);*/
	    MPIDI_DBG_PRINTF((60, FCNAME, "finished read saved in IBU_Process.unex_finished_list\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READ_UNEX);
	    return IBU_SUCCESS;
	}
    }
    MPIU_DBG_PRINTF(("exiting ibui_read_unex\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READ_UNEX);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_readv_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_readv_unex(ibu_t ibu)
{
    unsigned int num_bytes;
    ibu_unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_READV_UNEX);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_READV_UNEX);
    MPIU_DBG_PRINTF(("entering ibui_readv_unex"));

    while (ibu->unex_list)
    {
	while (ibu->unex_list->length && ibu->read.iovlen)
	{
	    num_bytes = min(ibu->unex_list->length, ibu->read.iov[ibu->read.index].MPID_IOV_LEN);
	    MPIDI_DBG_PRINTF((60, FCNAME, "copying %d bytes\n", num_bytes));
	    /* copy the received data */
	    memcpy(ibu->read.iov[ibu->read.index].MPID_IOV_BUF, ibu->unex_list->buf, num_bytes);
	    ibu->read.total += num_bytes;
	    ibu->unex_list->buf += num_bytes;
	    ibu->unex_list->length -= num_bytes;
	    /* update the iov */
	    ibu->read.iov[ibu->read.index].MPID_IOV_LEN -= num_bytes;
	    ibu->read.iov[ibu->read.index].MPID_IOV_BUF = 
		(char*)(ibu->read.iov[ibu->read.index].MPID_IOV_BUF) + num_bytes;
	    if (ibu->read.iov[ibu->read.index].MPID_IOV_LEN == 0)
	    {
		ibu->read.index++;
		ibu->read.iovlen--;
	    }
	}

	if (ibu->unex_list->length == 0)
	{
	    /* put the receive packet back in the pool */
	    if (ibu->unex_list->mem_ptr == NULL)
	    {
		MPIU_Internal_error_printf("ibui_readv_unex: mem_ptr == NULL\n");
	    }
	    assert(ibu->unex_list->mem_ptr != NULL);
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockFreeIB(mem_ptr)"));
	    ibuBlockFreeIB(ibu->allocator, ibu->unex_list->mem_ptr);
	    /* MPIU_Free the unexpected data node */
	    temp = ibu->unex_list;
	    ibu->unex_list = ibu->unex_list->next;
	    MPIU_Free(temp);
	    /* replace the consumed read descriptor */
	    ibui_post_receive(ibu);
	}
	
	if (ibu->read.iovlen == 0)
	{
	    ibu->state &= ~IBU_READING;
	    ibu->unex_finished_queue = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = ibu;
	    MPIDI_DBG_PRINTF((60, FCNAME, "finished read saved in IBU_Process.unex_finished_list\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READV_UNEX);
	    return IBU_SUCCESS;
	}
    }
    MPIU_DBG_PRINTF(("exiting ibui_readv_unex\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READV_UNEX);
    return IBU_SUCCESS;
}

/*#define PRINT_IBU_WAIT*/
#ifdef PRINT_IBU_WAIT
#define MPIU_DBG_PRINTFX(a) MPIU_DBG_PRINTF(a)
#else
#define MPIU_DBG_PRINTFX(a)
#endif

char * op2str(int opcode)
{
    static char str[20];
    switch(opcode)
    {
    case VAPI_CQE_SQ_SEND_DATA:
	return "VAPI_CQE_SQ_SEND_DATA";
    case VAPI_CQE_SQ_RDMA_WRITE:
	return "VAPI_CQE_SQ_RDMA_WRITE";
    case VAPI_CQE_SQ_RDMA_READ:
	return "VAPI_CQE_SQ_RDMA_READ";
    case VAPI_CQE_SQ_COMP_SWAP:
	return "VAPI_CQE_SQ_COMP_SWAP";
    case VAPI_CQE_SQ_FETCH_ADD:
	return "VAPI_CQE_SQ_FETCH_ADD";
    case VAPI_CQE_SQ_BIND_MRW:
	return "VAPI_CQE_SQ_BIND_MRW";
    case VAPI_CQE_RQ_SEND_DATA:
	return "VAPI_CQE_RQ_SEND_DATA";
    case VAPI_CQE_RQ_RDMA_WITH_IMM:
	return "VAPI_CQE_RQ_RDMA_WITH_IMM";
    case VAPI_CQE_INVAL_OPCODE:
	return "VAPI_CQE_INVAL_OPCODE";
    }
    sprintf(str, "%d", opcode);
    return str;
}

void PrintWC(VAPI_wc_desc_t *p)
{
    printf("Work Completion Descriptor:\n");
    printf(" id: %d\n", (int)p->id);
    printf(" opcode: %u = %s\n",
	   p->opcode, VAPI_cqe_opcode_sym(p->opcode));
    printf(" byte_len: %d\n", p->byte_len);
    printf(" imm_data_valid: %d\n", (int)p->imm_data_valid);
    printf(" imm_data: %u\n", (unsigned int)p->imm_data);
    printf(" remote_node_addr:\n");
    printf("  type: %u = %s\n",
	   p->remote_node_addr.type,
	   VAPI_remote_node_addr_sym(p->remote_node_addr.type));
    printf("  slid: %d\n", (int)p->remote_node_addr.slid);
    printf("  sl: %d\n", (int)p->remote_node_addr.sl);
    printf("  qp: %d\n", (int)p->remote_node_addr.qp_ety.qp);
    printf("  loc_eecn: %d\n", (int)p->remote_node_addr.ee_dlid.loc_eecn);
    printf(" grh_flag: %d\n", (int)p->grh_flag);
    printf(" pkey_ix: %d\n", p->pkey_ix);
    printf(" status: %u = %s\n",
	   (int)p->status, VAPI_wc_status_sym(p->status));
    printf(" vendor_err_syndrome: %d\n", p->vendor_err_syndrome);
    printf(" free_res_count: %d\n", p->free_res_count);
    fflush(stdout);
}

#undef FUNCNAME
#define FUNCNAME ibu_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_wait(ibu_set_t set, int millisecond_timeout, void **vc_pptr, int *num_bytes_ptr, ibu_op_t *op_ptr)
{
    int i;
    VAPI_ret_t status;
    VAPI_wc_desc_t completion_data;
    void *mem_ptr;
    char *iter_ptr;
    ibu_t ibu;
    int num_bytes;
    unsigned int offset;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
#ifdef USE_INLINE_PKT_RECEIVE
    MPIDI_VC *recv_vc_ptr;
    void *mem_ptr_orig;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBU_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WAIT);
    MPIU_DBG_PRINTFX(("entering ibu_wait\n"));
    for (;;) 
    {
	if (IBU_Process.unex_finished_list)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "returning previously received %d bytes", IBU_Process.unex_finished_list->read.total));
	    /* remove this ibu from the finished list */
	    ibu = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = IBU_Process.unex_finished_list->unex_finished_queue;
	    ibu->unex_finished_queue = NULL;

	    *num_bytes_ptr = ibu->read.total;
	    *op_ptr = IBU_OP_READ;
	    *vc_pptr = ibu->vc_ptr;
	    ibu->pending_operations--;
	    if (ibu->closing && ibu->pending_operations == 0)
	    {
		ibu = IBU_INVALID_QP;
	    }
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 1\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_SUCCESS;
	}

	status = VAPI_poll_cq(
	    IBU_Process.hca_handle,
	    set,
	    &completion_data);
	if (status == VAPI_EAGAIN || status == VAPI_CQ_EMPTY)
	{
	    /*usleep(1);*/
	    /* ibu_wait polls until there is something in the queue */
	    /* or the timeout has expired */
	    if (millisecond_timeout == 0)
	    {
		*num_bytes_ptr = 0;
		*vc_pptr = NULL;
		*op_ptr = IBU_OP_TIMEOUT;
		MPIU_DBG_PRINTFX(("exiting ibu_wait 2\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_SUCCESS;
	    }
	    continue;
	}
	if (status != VAPI_OK)
	{
	    MPIU_Internal_error_printf("%s: error: VAPI_poll_cq did not return VAPI_OK, %s\n", FCNAME, VAPI_strerror(status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 3\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	/*
	if (completion_data.status != VAPI_SUCCESS)
	{
	    MPIU_Internal_error_printf("%s: error: status = %s != VAPI_SUCCESS\n", 
		FCNAME, VAPI_strerror(completion_data.status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	*/

#ifdef HAVE_32BIT_POINTERS
	ibu = (ibu_t)(((ibu_work_id_handle_t*)&completion_data.id)->data.ptr);
	mem_ptr = (void*)(((ibu_work_id_handle_t*)&completion_data.id)->data.mem);
#else
	id_ptr = *((ibu_work_id_handle_t**)&completion_data.id);
	ibu = (ibu_t)(id_ptr->ptr);
	mem_ptr = (void*)(id_ptr->mem);
	ibuBlockFree(g_workAllocator, (void*)id_ptr);
#endif
#ifdef USE_INLINE_PKT_RECEIVE
	mem_ptr_orig = mem_ptr;
#endif
	switch (completion_data.opcode)
	{
	case VAPI_CQE_SQ_SEND_DATA:
	    if (completion_data.status != VAPI_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: send completion status = %s\n",
                    FCNAME, VAPI_wc_status_sym(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTF(("at time of error: total_s: %d, total_r: %d, posted_r: %d, posted_s: %d, g_pr: %d, g_ps: %d\n", g_num_sent, g_num_received, g_num_posted_receives, g_num_posted_sends, g_pr, g_ps));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
	    MPIU_DBG_PRINTF(("", g_ps--));
	    if (mem_ptr == (void*)-1)
	    {
		/*printf("ack sent\n");fflush(stdout);*/
		/* flow control ack completed, no user data so break out here */
		MPIU_DBG_PRINTF(("ack sent.\n"));
		break;
	    }
	    MPIU_DBG_PRINTF(("___total_s: %d\n", ++g_num_sent));
	    g_cur_write_stack_index--;
	    num_bytes = g_num_bytes_written_stack[g_cur_write_stack_index].length;
	    MPIDI_DBG_PRINTF((60, FCNAME, "send num_bytes = %d\n", num_bytes));
	    if (num_bytes < 0)
	    {
		i = num_bytes;
		num_bytes = 0;
		for (; i<0; i++)
		{
		    g_cur_write_stack_index--;
		    MPIDI_DBG_PRINTF((60, FCNAME, "num_bytes += %d\n", g_num_bytes_written_stack[g_cur_write_stack_index].length));
		    num_bytes += g_num_bytes_written_stack[g_cur_write_stack_index].length;
		    if (g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr == NULL)
			MPIU_Internal_error_printf("ibu_wait: write stack has NULL mem_ptr at location %d\n", g_cur_write_stack_index);
		    assert(g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr != NULL);
		    ibuBlockFreeIB(ibu->allocator, g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr);
		}
	    }
	    else
	    {
		if (mem_ptr == NULL)
		    MPIU_Internal_error_printf("ibu_wait: send mem_ptr == NULL\n");
		assert(mem_ptr != NULL);
		ibuBlockFreeIB(ibu->allocator, mem_ptr);
	    }

	    *num_bytes_ptr = num_bytes;
	    *op_ptr = IBU_OP_WRITE;
	    *vc_pptr = ibu->vc_ptr;
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 5\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_SUCCESS;
	    break;
	case VAPI_CQE_RQ_SEND_DATA:
	    if (completion_data.status != VAPI_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: recv completion status = %s\n",
                    FCNAME, VAPI_wc_status_sym(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTF(("at time of error: total_s: %d, total_r: %d, posted_r: %d, posted_s: %d, g_pr: %d, g_ps: %d\n", g_num_sent, g_num_received, g_num_posted_receives, g_num_posted_sends, g_pr, g_ps));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
	    MPIU_DBG_PRINTF(("", g_pr--));
	    if (completion_data.imm_data_valid)
	    {
		ibu->nAvailRemote += completion_data.imm_data;
		MPIDI_DBG_PRINTF((60, FCNAME, "%d packets acked, nAvailRemote now = %d", completion_data.imm_data, ibu->nAvailRemote));
		ibuBlockFreeIB(ibu->allocator, mem_ptr);
		ibui_post_receive_unacked(ibu);
		assert(completion_data.byte_len == 1); /* check this after the printfs to see if the immediate data is correct */
		break;
	    }
	    MPIU_DBG_PRINTF(("___total_r: %d\n", ++g_num_received));
	    num_bytes = completion_data.byte_len;
#ifdef USE_INLINE_PKT_RECEIVE
	    recv_vc_ptr = ibu->vc_ptr;
	    if (recv_vc_ptr->ch.reading_pkt)
	    {
		if (((MPIDI_CH3_Pkt_t *)mem_ptr)->type < MPIDI_CH3_PKT_END_CH3)
		{
		    /*printf("P");fflush(stdout);*/
		    recv_vc_ptr->ch.recv_active = NULL;
		    MPIDI_CH3U_Handle_recv_pkt(recv_vc_ptr, mem_ptr);
		    if (recv_vc_ptr->ch.recv_active == NULL)
		    {
			MPIU_DBG_PRINTF(("packet %d with no data handled.\n", g_num_received));
			recv_vc_ptr->ch.reading_pkt = TRUE;
		    }
#ifdef MPICH_DBG_OUTPUT
		    else
		    {
			MPIU_DBG_PRINTF(("packet %d with data handled.\n", g_num_received));
		    }
#endif
		}
		else
		{
		    MPIDI_err_printf("MPIDI_CH3I_ibu_wait", "unhandled packet type: %d\n", ((MPIDI_CH3_Pkt_t*)mem_ptr)->type);
		}
		mem_ptr = (unsigned char *)mem_ptr + sizeof(MPIDI_CH3_Pkt_t);
		num_bytes -= sizeof(MPIDI_CH3_Pkt_t);

		if (num_bytes == 0)
		{
		    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
		    ibui_post_receive(ibu);
		    break;
		}
	    }
/*
	    else
	    {
		printf("not handling pkt.\n");fflush(stdout);
	    }
*/
#endif
	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes\n", num_bytes));
	    /*MPIDI_DBG_PRINTF((60, FCNAME, "ibu_wait(recv finished %d bytes)", num_bytes));*/
	    if (!(ibu->state & IBU_READING))
	    {
#ifdef USE_INLINE_PKT_RECEIVE
		ibui_buffer_unex_read(ibu, mem_ptr_orig, sizeof(MPIDI_CH3_Pkt_t), num_bytes);
#else
		ibui_buffer_unex_read(ibu, mem_ptr, 0, num_bytes);
#endif
		break;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read update, total = %d + %d = %d\n", ibu->read.total, num_bytes, ibu->read.total + num_bytes));
	    if (ibu->read.use_iov)
	    {
		iter_ptr = mem_ptr;
		while (num_bytes && ibu->read.iovlen > 0)
		{
		    if ((int)ibu->read.iov[ibu->read.index].MPID_IOV_LEN <= num_bytes)
		    {
			/* copy the received data */
			memcpy(ibu->read.iov[ibu->read.index].MPID_IOV_BUF, iter_ptr,
			    ibu->read.iov[ibu->read.index].MPID_IOV_LEN);
			iter_ptr += ibu->read.iov[ibu->read.index].MPID_IOV_LEN;
			/* update the iov */
			num_bytes -= ibu->read.iov[ibu->read.index].MPID_IOV_LEN;
			ibu->read.index++;
			ibu->read.iovlen--;
		    }
		    else
		    {
			/* copy the received data */
			memcpy(ibu->read.iov[ibu->read.index].MPID_IOV_BUF, iter_ptr, num_bytes);
			iter_ptr += num_bytes;
			/* update the iov */
			ibu->read.iov[ibu->read.index].MPID_IOV_LEN -= num_bytes;
			ibu->read.iov[ibu->read.index].MPID_IOV_BUF = 
			    (char*)(ibu->read.iov[ibu->read.index].MPID_IOV_BUF) + num_bytes;
			num_bytes = 0;
		    }
		}
		offset = (unsigned char*)iter_ptr - (unsigned char*)mem_ptr;
		ibu->read.total += offset;
		if (num_bytes == 0)
		{
		    /* put the receive packet back in the pool */
		    if (mem_ptr == NULL)
			MPIU_Internal_error_printf("ibu_wait: read mem_ptr == NULL\n");
		    assert(mem_ptr != NULL);
#ifdef USE_INLINE_PKT_RECEIVE
		    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
#else
		    ibuBlockFreeIB(ibu->allocator, mem_ptr);
#endif
		    ibui_post_receive(ibu);
		}
		else
		{
		    /* save the unused but received data */
#ifdef USE_INLINE_PKT_RECEIVE
		    ibui_buffer_unex_read(ibu, mem_ptr_orig, offset, num_bytes);
#else
		    ibui_buffer_unex_read(ibu, mem_ptr, offset, num_bytes);
#endif
		}
		if (ibu->read.iovlen == 0)
		{
		    ibu->state &= ~IBU_READING;
		    *num_bytes_ptr = ibu->read.total;
		    *op_ptr = IBU_OP_READ;
		    *vc_pptr = ibu->vc_ptr;
		    ibu->pending_operations--;
		    if (ibu->closing && ibu->pending_operations == 0)
		    {
			MPIDI_DBG_PRINTF((60, FCNAME, "closing ibuet after iov read completed."));
			ibu = IBU_INVALID_QP;
		    }
		    MPIU_DBG_PRINTFX(("exiting ibu_wait 6\n"));
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return IBU_SUCCESS;
		}
	    }
	    else
	    {
		if ((unsigned int)num_bytes > ibu->read.bufflen)
		{
		    /* copy the received data */
		    memcpy(ibu->read.buffer, mem_ptr, ibu->read.bufflen);
		    ibu->read.total = ibu->read.bufflen;
#ifdef USE_INLINE_PKT_RECEIVE
		    ibui_buffer_unex_read(ibu, mem_ptr_orig, ibu->read.bufflen, num_bytes - ibu->read.bufflen);
#else
		    ibui_buffer_unex_read(ibu, mem_ptr, ibu->read.bufflen, num_bytes - ibu->read.bufflen);
#endif
		    ibu->read.bufflen = 0;
		}
		else
		{
		    /* copy the received data */
		    memcpy(ibu->read.buffer, mem_ptr, num_bytes);
		    ibu->read.total += num_bytes;
		    /* advance the user pointer */
		    ibu->read.buffer = (char*)(ibu->read.buffer) + num_bytes;
		    ibu->read.bufflen -= num_bytes;
		    /* put the receive packet back in the pool */
#ifdef USE_INLINE_PKT_RECEIVE
		    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
#else
		    ibuBlockFreeIB(ibu->allocator, mem_ptr);
#endif
		    ibui_post_receive(ibu);
		}
		if (ibu->read.bufflen == 0)
		{
		    ibu->state &= ~IBU_READING;
		    *num_bytes_ptr = ibu->read.total;
		    *op_ptr = IBU_OP_READ;
		    *vc_pptr = ibu->vc_ptr;
		    ibu->pending_operations--;
		    if (ibu->closing && ibu->pending_operations == 0)
		    {
			MPIDI_DBG_PRINTF((60, FCNAME, "closing ibu after simple read completed."));
			ibu = IBU_INVALID_QP;
		    }
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    MPIU_DBG_PRINTFX(("exiting ibu_wait 7\n"));
		    return IBU_SUCCESS;
		}
	    }
	    break;
	default:
	    if (completion_data.status != VAPI_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: unknown completion status = %s != VAPI_SUCCESS\n", 
		    FCNAME, VAPI_strerror(completion_data.status));
		MPIU_DBG_PRINTF(("at time of error: total_s: %d, total_r: %d, posted_r: %d, posted_s: %d\n", g_num_sent, g_num_received, g_num_posted_receives, g_num_posted_sends));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
	    MPIU_Internal_error_printf("%s: unknown ib opcode: %s\n", FCNAME, op2str(completion_data.opcode));
	    return IBU_FAIL;
	    break;
	}
    }

    MPIU_DBG_PRINTFX(("exiting ibu_wait 8\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_set_vc_ptr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_set_vc_ptr(ibu_t ibu, void *vc_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_SET_USER_PTR);
    MPIU_DBG_PRINTF(("entering ibu_set_vc_ptr\n"));
    if (ibu == IBU_INVALID_QP)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_SET_USER_PTR);
	return IBU_FAIL;
    }
    ibu->vc_ptr = vc_ptr;
    MPIU_DBG_PRINTF(("exiting ibu_set_vc_ptr\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_SET_USER_PTR);
    return IBU_SUCCESS;
}

/* non-blocking functions */

#undef FUNCNAME
#define FUNCNAME ibu_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_read(ibu_t ibu, void *buf, int len)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_READ);
    MPIU_DBG_PRINTF(("entering ibu_post_read\n"));
    ibu->read.total = 0;
    ibu->read.buffer = buf;
    ibu->read.bufflen = len;
    ibu->read.use_iov = FALSE;
    ibu->state |= IBU_READING;
#ifdef USE_INLINE_PKT_RECEIVE
    ibu->vc_ptr->ch.reading_pkt = FALSE;
#endif
    ibu->pending_operations++;
    /* copy any pre-received data into the buffer */
    if (ibu->unex_list)
	ibui_read_unex(ibu);
    MPIU_DBG_PRINTF(("exiting ibu_post_read\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_READ);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_post_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_readv(ibu_t ibu, MPID_IOV *iov, int n)
{
#ifdef MPICH_DBG_OUTPUT
    char str[1024] = "ibu_post_readv: ";
    char *s;
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_READV);
    MPIU_DBG_PRINTF(("entering ibu_post_readv\n"));
#ifdef MPICH_DBG_OUTPUT
    s = &str[16];
    for (i=0; i<n; i++)
    {
	s += sprintf(s, "%d,", iov[i].MPID_IOV_LEN);
    }
    MPIDI_DBG_PRINTF((60, FCNAME, "%s\n", str));
#endif
    ibu->read.total = 0;
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    /*ibu->read.iov = iov;*/
    memcpy(ibu->read.iov, iov, sizeof(MPID_IOV) * n);
    ibu->read.iovlen = n;
    ibu->read.index = 0;
    ibu->read.use_iov = TRUE;
    ibu->state |= IBU_READING;
#ifdef USE_INLINE_PKT_RECEIVE
    ibu->vc_ptr->ch.reading_pkt = FALSE;
#endif
    ibu->pending_operations++;
    /* copy any pre-received data into the iov */
    if (ibu->unex_list)
	ibui_readv_unex(ibu);
    MPIU_DBG_PRINTF(("exiting ibu_post_readv\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_READV);
    return IBU_SUCCESS;
}

/* extended functions */

#undef FUNCNAME
#define FUNCNAME ibu_get_lid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_get_lid()
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_GET_LID);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_GET_LID);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_GET_LID);
    return IBU_Process.lid;
}

#endif /* USE_IB_VAPI */
