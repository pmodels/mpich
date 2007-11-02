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

#ifdef USE_IB_IBAL

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define IBU_ERROR_MSG_LENGTH       255
#define IBU_PACKET_SIZE            (1024 * 64)
#define IBU_PACKET_COUNT           128
#define IBU_NUM_PREPOSTED_RECEIVES (IBU_ACK_WATER_LEVEL*3)
#define IBU_MAX_CQ_ENTRIES         255
#define IBU_MAX_POSTED_SENDS       8192
#define IBU_MAX_DATA_SEGMENTS      100
#define IBU_ACK_WATER_LEVEL        32

#define TRACE_IBU

#if 0
#define GETLKEY(p) (((ibmem_t*)p) - 1)->lkey
typedef struct ibmem_t
{
    ib_mr_handle_t handle;
    uint32_t lkey;
    uint32_t rkey;
} ibmem_t;
#endif

typedef struct ibuBlock_t
{
    struct ibuBlock_t *next;
    ib_mr_handle_t handle;
    uint32_t lkey;
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
    uint64_t id;
    struct ibu_data
    {
	uint32_t ptr, mem;
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
#define IBU_WRITING    0x0002

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
    void *mem_ptr;
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
    ib_qp_handle_t qp_handle;
    ibuQueue_t * allocator;

    ib_net16_t dlid;
    uint32_t qp_num, dest_qp_num;

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
    ib_al_handle_t   al_handle;
    ib_ca_handle_t   hca_handle;
    ib_pd_handle_t   pd_handle;
    int              cq_size;
    ib_net16_t       lid;
    int              port;
    ibu_state_t *    unex_finished_list;
    int              error;
    char             err_msg[IBU_ERROR_MSG_LENGTH];
    /* hack to get around zero sized messages */
    void *           ack_mem_ptr;
    ib_mr_handle_t   ack_mr_handle;
    uint32_t         ack_lkey;
#ifdef TRACE_IBU
    int outstanding_recvs, outstanding_sends, total_recvs, total_sends;
#endif
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
static int ibuBlockFreeIB(ibuQueue_t * p, void *pBlock);
static void *ib_malloc_register(size_t size, ib_mr_handle_t *mhp, uint32_t *lp, uint32_t *rp);
static void ib_free_deregister(void *p);

static ibuBlockAllocator ibuBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(size_t size), void (* free_fn)(void *p))
{
    ibuBlockAllocator p;
    void **ppVoid;
    int i;

    p = alloc_fn( sizeof(struct ibuBlockAllocator_struct) + ((blocksize + sizeof(void**)) * count) );

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

static ibuQueue_t * ibuBlockAllocInitIB()
{
    ibuQueue_t *q;
    int i;
    ibuBlock_t b[2];
    ib_mr_handle_t handle;
    uint32_t lkey;
    uint32_t rkey;

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
	MPIU_DBG_PRINTF(("ibuBlockAlloc returning NULL\n"));
	return NULL;
    }

    pVoid = p->pNextFree + 1;
    p->pNextFree = *(p->pNextFree);

    return pVoid;
}

static void * ibuBlockAllocIB(ibuQueue_t * p)
{
    void *pVoid;

    if (p->pNextFree == NULL)
    {
	MPIU_DBG_PRINTF(("ibuBlockAlloc returning NULL\n"));
	return NULL;
    }

    pVoid = p->pNextFree->data;
    p->pNextFree = p->pNextFree->next;

    return pVoid;
}

static int ibuBlockFree(ibuBlockAllocator p, void *pBlock)
{
    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

    return 0;
}

static int ibuBlockFreeIB(ibuQueue_t * p, void *pBlock)
{
    ibuBlock_t *b;

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
static ib_api_status_t modifyQP( ibu_t ibu, ib_qp_state_t qp_state )
{
    ib_api_status_t status;
    ib_qp_mod_t qp_mod;
    MPIDI_STATE_DECL(MPID_STATE_IBU_MODIFYQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_MODIFYQP);

    memset(&qp_mod, 0, sizeof(qp_mod));
    MPIU_DBG_PRINTF(("entering modifyQP\n"));
    if (qp_state == IB_QPS_INIT)
    {
	qp_mod.req_state = IB_QPS_INIT;
	qp_mod.state.init.qkey = 0x0;
	qp_mod.state.init.pkey_index = 0x0;
	qp_mod.state.init.primary_port = IBU_Process.port;
	qp_mod.state.init.access_ctrl = IB_AC_LOCAL_WRITE;
    }
    else if (qp_state == IB_QPS_RTR) 
    {
	qp_mod.req_state = IB_QPS_RTR;
	qp_mod.state.rtr.rq_psn = cl_hton32(0x00000001);
	qp_mod.state.rtr.dest_qp = ibu->dest_qp_num;
	qp_mod.state.rtr.resp_res = 1;
	qp_mod.state.rtr.pkey_index = 0x0;
	qp_mod.state.rtr.rnr_nak_timeout = 7;
	qp_mod.state.rtr.pkey_index = 0x0;
	qp_mod.state.rtr.opts = IB_MOD_QP_PRIMARY_AV;
	qp_mod.state.rtr.primary_av.sl = 0;
	qp_mod.state.rtr.primary_av.dlid = ibu->dlid;
	qp_mod.state.rtr.primary_av.grh_valid = 0;
	qp_mod.state.rtr.primary_av.static_rate = 0;
	qp_mod.state.rtr.primary_av.path_bits = 0;
	qp_mod.state.rtr.primary_av.conn.path_mtu = IB_MTU_1024;
	qp_mod.state.rtr.primary_av.conn.local_ack_timeout = 7;
	qp_mod.state.rtr.primary_av.conn.seq_err_retry_cnt = 7;
	qp_mod.state.rtr.primary_av.conn.rnr_retry_cnt = 7;
	/*qp_mod.state.rtr.primary_av.port_num = IBU_Process.port;*/
    }
    else if (qp_state == IB_QPS_RTS)
    {
	qp_mod.req_state = IB_QPS_RTS;
	qp_mod.state.rts.sq_psn = cl_hton32(0x00000001);
	qp_mod.state.rts.retry_cnt = 7;
	qp_mod.state.rts.rnr_retry_cnt = 7;
	qp_mod.state.rts.rnr_nak_timeout = 7;
	qp_mod.state.rts.local_ack_timeout = 0x20;
	qp_mod.state.rts.init_depth = 7;
	qp_mod.state.rts.opts = 0;
    }
    else if (qp_state == IB_QPS_RESET)
    {
	qp_mod.req_state = IB_QPS_RESET;
	qp_mod.state.reset.timewait = 0;
    }
    else
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	return IBU_FAIL;
    }

    status = ib_modify_qp(ibu->qp_handle, &qp_mod);
    if( status != IB_SUCCESS )
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	return status;
    }

    MPIU_DBG_PRINTF(("exiting modifyQP\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
    return IBU_SUCCESS;
}

static ib_api_status_t createQP(ibu_t ibu, ibu_set_t set)
{
    ib_api_status_t status;
    ib_qp_create_t qp_init_attr;
    ib_qp_attr_t qp_prop;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATEQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATEQP);
    MPIU_DBG_PRINTF(("entering createQP\n"));
    qp_init_attr.qp_type = IB_QPT_RELIABLE_CONN;
    qp_init_attr.h_rdd = 0;
    qp_init_attr.sq_depth = 10000;
    qp_init_attr.rq_depth = 10000;
    qp_init_attr.sq_sge = 8;
    qp_init_attr.rq_sge = 8;
    qp_init_attr.h_sq_cq = set;
    qp_init_attr.h_rq_cq = set;
    qp_init_attr.sq_signaled = FALSE; /*TRUE;*/

    status = ib_create_qp(IBU_Process.pd_handle, &qp_init_attr, NULL, NULL, &ibu->qp_handle);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("ib_create_qp failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
	return status;
    }
    status = ib_query_qp(ibu->qp_handle, &qp_prop);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("ib_query_qp failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
	return status;
    }
    MPIU_DBG_PRINTF(("qp: num = %d, dest_num = %d\n",
		     cl_ntoh32(qp_prop.num),
		     cl_ntoh32(qp_prop.dest_num)));
    ibu->qp_num = qp_prop.num;

    MPIU_DBG_PRINTF(("exiting createQP\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ib_malloc_register
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void *ib_malloc_register(size_t size, ib_mr_handle_t *mhp, uint32_t *lp, uint32_t *rp)
{
    ib_api_status_t status;
    void *ptr;
    ib_mr_create_t mem;
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
    mem.vaddr = ptr;
    mem.length = (uint64_t)size;
    mem.access_ctrl = IB_AC_LOCAL_WRITE | IB_AC_RDMA_WRITE;
    status = ib_reg_mem(IBU_Process.pd_handle, &mem, lp, rp, mhp);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ib_malloc_register: ib_reg_mem failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
	return NULL;
    }

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
    ib_api_status_t status;
    ibu_t p;
    MPIDI_STATE_DECL(MPID_STATE_IBU_START_QP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_START_QP);
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
	MPIU_Internal_error_printf("ibu_create_qp: createQP failed, error %s\n", ib_get_err_str(status));
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
    int mpi_errno;
    ib_api_status_t status;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_IBU_FINISH_QP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_FINISH_QP);

    MPIU_DBG_PRINTF(("entering ibu_finish_qp\n"));

    p->dest_qp_num = dest_qp_num;
    p->dlid = dest_lid;
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(INIT)"));*/
    status = modifyQP(p, IB_QPS_INIT);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: modifyQP(INIT) failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(RTR)"));*/
    status = modifyQP(p, IB_QPS_RTR);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: modifyQP(RTR) failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }

    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(RTS)"));*/
    status = modifyQP(p, IB_QPS_RTS);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: modifyQP(RTS) failed, error %s\n", ib_get_err_str(status));
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
    p->nAvailRemote--; /* remove one from nAvailRemote so a ack packet can always get through */

    MPIU_DBG_PRINTF(("exiting ibu_finish_qp\n"));    
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_post_receive_unacked
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_receive_unacked(ibu_t ibu)
{
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_recv_wr_t work_req;
    void *mem_ptr;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);

    /*MPIU_DBG_PRINTF(("entering ibui_post_receive_unacked\n"));*/
    mem_ptr = ibuBlockAllocIB(ibu->allocator);
    if (mem_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAllocIB returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);
	return IBU_FAIL;
    }

#ifdef HAVE_32BIT_POINTERS
    ((ibu_work_id_handle_t*)&work_req.wr_id)->data.ptr = (u_int32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.wr_id)->data.mem = (u_int32_t)mem_ptr;
#else
    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
    *((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
    if (id_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);
	return IBU_FAIL;
    }
    id_ptr->ptr = (void*)ibu;
    id_ptr->mem = (void*)mem_ptr;
#endif
    work_req.p_next = NULL;
    work_req.num_ds = 1;
    work_req.ds_array = &data;
    data.vaddr = mem_ptr;
    data.length = IBU_PACKET_SIZE;
    data.lkey = GETLKEY(mem_ptr);

    /*MPIDI_DBG_PRINTF((60, FCNAME, "calling ib_post_recv"));*/
    MPIU_DBG_PRINTF(("."));
    status = ib_post_recv(ibu->qp_handle, &work_req, NULL);
    if (status != IB_SUCCESS)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	MPIU_Internal_error_printf("%s: Error: failed to post ib receive, status = %s\n", FCNAME, ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);
	return status;
    }
#ifdef TRACE_IBU
    IBU_Process.outstanding_recvs++;
#endif

    /*MPIU_DBG_PRINTF(("exiting ibui_post_receive_unacked\n"));*/
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE_UNACKED);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_post_receive
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_receive(ibu_t ibu)
{
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_recv_wr_t work_req;
    void *mem_ptr;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_RECEIVE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_RECEIVE);

    /*MPIU_DBG_PRINTF(("entering ibui_post_receive\n"));*/
    mem_ptr = ibuBlockAllocIB(ibu->allocator);
    if (mem_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAllocIB returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
	return IBU_FAIL;
    }

#ifdef HAVE_32BIT_POINTERS
    ((ibu_work_id_handle_t*)&work_req.wr_id)->data.ptr = (u_int32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.wr_id)->data.mem = (u_int32_t)mem_ptr;
#else
    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
    *((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
    if (id_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
	return IBU_FAIL;
    }
    id_ptr->ptr = (void*)ibu;
    id_ptr->mem = (void*)mem_ptr;
#endif
    work_req.p_next = NULL;
    work_req.num_ds = 1;
    work_req.ds_array = &data;
    data.vaddr = mem_ptr;
    data.length = IBU_PACKET_SIZE;
    data.lkey = GETLKEY(mem_ptr);

    /*MPIDI_DBG_PRINTF((60, FCNAME, "calling ib_post_recv"));*/
    MPIU_DBG_PRINTF(("*"));

    status = ib_post_recv(ibu->qp_handle, &work_req, NULL);
    if (status != IB_SUCCESS)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	MPIU_Internal_error_printf("%s: Error: failed to post ib receive, status = %s\n", FCNAME, ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
	return status;
    }
#ifdef TRACE_IBU
    IBU_Process.outstanding_recvs++;
#endif
    if (++ibu->nUnacked > IBU_ACK_WATER_LEVEL)
    {
	ibui_post_ack_write(ibu);
    }

    /*MPIU_DBG_PRINTF(("exiting ibui_post_receive\n"));*/
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_post_ack_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_ack_write(ibu_t ibu)
{
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_send_wr_t work_req;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_ACK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_ACK_WRITE);

    /*MPIU_DBG_PRINTF(("entering ibui_post_ack_write\n"));*/
    data.vaddr = IBU_Process.ack_mem_ptr;
    data.length = 1;
    data.lkey = IBU_Process.ack_lkey;

    work_req.p_next = NULL;
    work_req.wr_type = WR_SEND;
    work_req.send_opt = IB_SEND_OPT_IMMEDIATE | IB_SEND_OPT_SIGNALED;
    work_req.num_ds = 1;
    work_req.ds_array = &data;
    work_req.immediate_data = ibu->nUnacked;
#ifdef HAVE_32BIT_POINTERS
    /* store the ibu ptr and the mem ptr in the work id */
    ((ibu_work_id_handle_t*)&work_req.wr_id)->data.ptr = (u_int32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.wr_id)->data.mem = (u_int32_t)-1;
#else
    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
    *((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
    if (id_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
	return IBU_FAIL;
    }
    id_ptr->ptr = (void*)ibu;
    id_ptr->mem = (void*)-1;
#endif

    MPIDI_DBG_PRINTF((60, FCNAME, "ib_post_send(ack = %d)", ibu->nUnacked));
    /*printf("ib_post_send(ack = %d)\n", ibu->nUnacked);fflush(stdout);*/
    status = ib_post_send(ibu->qp_handle, &work_req, NULL);
    if (status != IB_SUCCESS)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
	return status;
    }
#ifdef TRACE_IBU
    IBU_Process.outstanding_sends++;
#endif
    ibu->nUnacked = 0;

    /*MPIU_DBG_PRINTF(("exiting ibui_post_ack_write\n"));*/
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
    return IBU_SUCCESS;
}

#if 0
#undef FUNCNAME
#define FUNCNAME ibui_post_ack_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_ack_write(ibu_t ibu)
{
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_send_wr_t work_req;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_ACK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_ACK_WRITE);

    /*MPIU_DBG_PRINTF(("entering ibui_post_ack_write\n"));*/
    work_req.p_next = NULL;
    work_req.wr_type = WR_SEND;
    work_req.send_opt = IB_SEND_OPT_IMMEDIATE | IB_SEND_OPT_SIGNALED;
    data.vaddr = NULL;
    data.length = 0;
    data.lkey = GETLKEY(mem_ptr);
    work_req.num_ds = 1;
    work_req.ds_array = &data;
    work_req.immediate_data = ibu->nUnacked;
#ifdef HAVE_32BIT_POINTERS
    ((ibu_work_id_handle_t*)&work_req.wr_id)->data.ptr = (u_int32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.wr_id)->data.mem = (u_int32_t)-1;
#else
    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
    *((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
    if (id_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
	return IBU_FAIL;
    }
    id_ptr->ptr = (void*)ibu;
    id_ptr->mem = (void*)-1;
#endif
    
    MPIDI_DBG_PRINTF((60, FCNAME, "ib_post_send(ack = %d)", ibu->nUnacked));
    printf("ib_post_send(ack = %d)\n", ibu->nUnacked);fflush(stdout);
    status = ib_post_send(ibu->qp_handle, &work_req, NULL);
    if (status != IB_SUCCESS)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
	return status;
    }
#ifdef TRACE_IBU
    IBU_Process.outstanding_sends++;
#endif
    ibu->nUnacked = 0;

    /*MPIU_DBG_PRINTF(("exiting ibui_post_ack_write\n"));*/
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
    return IBU_SUCCESS;
}
#endif

/* ibu functions */

#undef FUNCNAME
#define FUNCNAME ibu_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_write(ibu_t ibu, void *buf, int len, int *num_bytes_ptr)
{
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_send_wr_t work_req;
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
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAllocIB returned NULL\n"));
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

	data.vaddr = mem_ptr;
	data.length = length;
	data.lkey = GETLKEY(mem_ptr);
	
	work_req.p_next = NULL;
	work_req.wr_type = WR_SEND;
	work_req.send_opt = IB_SEND_OPT_SIGNALED;
	work_req.num_ds = 1;
	work_req.ds_array = &data;
	work_req.immediate_data = 0;
#ifdef HAVE_32BIT_POINTERS
	/* store the ibu ptr and the mem ptr in the work id */
	((ibu_work_id_handle_t*)&work_req.wr_id)->data.ptr = (u_int32_t)ibu;
	((ibu_work_id_handle_t*)&work_req.wr_id)->data.mem = (u_int32_t)mem_ptr;
#else
	id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
	*((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
	if (id_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return IBU_FAIL;
	}
	id_ptr->ptr = (void*)ibu;
	id_ptr->mem = (void*)mem_ptr;
#endif
	
	MPIDI_DBG_PRINTF((60, FCNAME, "calling ib_post_send(%d bytes)", length));
	status = ib_post_send(ibu->qp_handle, &work_req, NULL);
	if (status != IB_SUCCESS)
	{
	    MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	    MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, ib_get_err_str(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return IBU_FAIL;
	}
#ifdef TRACE_IBU
	IBU_Process.outstanding_sends++;
#endif
	ibu->nAvailRemote--;

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
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_send_wr_t work_req;
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
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAllocIB returned NULL."));
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
	
	data.vaddr = mem_ptr;
	data.length = msg_size;
	data.lkey = GETLKEY(mem_ptr);

	work_req.p_next = NULL;
	work_req.wr_type = WR_SEND;
	work_req.send_opt = IB_SEND_OPT_SIGNALED;
	work_req.num_ds = 1;
	work_req.ds_array = &data;
	work_req.immediate_data = 0;
#ifdef HAVE_32BIT_POINTERS
	/* store the ibu ptr and the mem ptr in the work id */
	((ibu_work_id_handle_t*)&work_req.wr_id)->data.ptr = (u_int32_t)ibu;
	((ibu_work_id_handle_t*)&work_req.wr_id)->data.mem = (u_int32_t)mem_ptr;
#else
	id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(g_workAllocator);
	*((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
	if (id_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_FAIL;
	}
	id_ptr->ptr = (void*)ibu;
	id_ptr->mem = (void*)mem_ptr;
#endif
	
	MPIDI_DBG_PRINTF((60, FCNAME, "ib_post_send(%d bytes)", msg_size));
	status = ib_post_send(ibu->qp_handle, &work_req, NULL);
	if (status != IB_SUCCESS)
	{
	    MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	    MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, ib_get_err_str(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_SUCCESS;
	}
#ifdef TRACE_IBU
	IBU_Process.outstanding_sends++;
#endif
	ibu->nAvailRemote--;
	
    } while (cur_index < n);

    *num_bytes_ptr = total;

    MPIU_DBG_PRINTF(("exiting ibu_writev\n"));    
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_get_first_active_ca
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_get_first_active_ca()
{
    int mpi_errno;
    ib_api_status_t	status;
    intn_t		guid_count;
    intn_t		i;
    uint32_t		port;
    ib_net64_t		p_ca_guid_array[12];
    ib_ca_attr_t	*p_ca_attr;
    size_t		bsize;
    ib_port_attr_t      *p_port_attr;
    ib_ca_handle_t      hca_handle;

    status = ib_get_ca_guids( IBU_Process.al_handle, NULL, &guid_count );
    if(status != IB_INSUFFICIENT_MEMORY)
    {
	MPIU_Internal_error_printf( "[%d] ib_get_ca_guids failed [%s]\n", __LINE__, ib_get_err_str(status));
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**get_guids", 0);
	return mpi_errno;
    }
    /*printf("Total number of CA's = %d\n", (uint32_t)guid_count);fflush(stdout);*/
    if(guid_count == 0) 
    {
	MPIU_Internal_error_printf("no channel adapters available.\n");
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**noca", 0);
	return mpi_errno;
    }
    if (guid_count > 12)
    {
	guid_count = 12;
    }

    status = ib_get_ca_guids(IBU_Process.al_handle, p_ca_guid_array, &guid_count);
    if( status != IB_SUCCESS )
    {
	MPIU_Internal_error_printf("[%d] ib_get_ca_guids failed [%s]\n", __LINE__, ib_get_err_str(status));
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ca_guids", "**ca_guids %s", ib_get_err_str(status));
	return mpi_errno;
    }

    /* walk guid table */
    for( i = 0; i < guid_count; i++ )
    {
	status = ib_open_ca( IBU_Process.al_handle, p_ca_guid_array[i], 
				NULL, NULL, &hca_handle );
	if(status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf( "[%d] ib_open_ca failed [%s]\n", __LINE__, 
		    ib_get_err_str(status));
	    continue;
	}		

	/*printf( "GUID = %"PRIx64"\n", p_ca_guid_array[i]);fflush(stdout);*/

	/* Query the CA */
	bsize = 0;
	status = ib_query_ca( hca_handle, NULL, &bsize );
	if(status != IB_INSUFFICIENT_MEMORY)
	{
	    MPIU_Internal_error_printf( "[%d] ib_query_ca failed [%s]\n", __LINE__, 
		    ib_get_err_str(status));
	    ib_close_ca(hca_handle, NULL);
	    continue;
	}

	/* Allocate the memory needed for query_ca */
	p_ca_attr = (ib_ca_attr_t *)cl_zalloc( bsize );
	if( !p_ca_attr )
	{
	    MPIU_Internal_error_printf( "[%d] not enough memory\n", __LINE__); 
	    ib_close_ca(hca_handle, NULL);
	    continue;
	}
		
	status = ib_query_ca( hca_handle, p_ca_attr, &bsize );
	if(status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf( "[%d] ib_query_ca failed [%s]\n", __LINE__,
			ib_get_err_str(status));
	    ib_close_ca(hca_handle, NULL);
	    cl_free( p_ca_attr );
	    continue;
	}

	/* scan for active port */
	for( port = 0; port < p_ca_attr->num_ports; port++ )
	{
	    p_port_attr = &p_ca_attr->p_port_attr[port];
	    
	    /* is there an active port? */
	    if( p_port_attr->link_state == IB_LINK_ACTIVE )
	    {
		/* yes, is there a port_guid or lid we should attach to? */
		/*
		printf("port %d active with lid %d\n", 
		       p_port_attr->port_num,
		       cl_ntoh16(p_port_attr->lid));
		fflush(stdout);
		*/
		/* get a protection domain handle */
		status = ib_alloc_pd(hca_handle, IB_PDT_NORMAL,
				     NULL, &IBU_Process.pd_handle);
		if (status != IB_SUCCESS)
		{
		    MPIU_Internal_error_printf("get_first_ca: ib_alloc_pd failed, status %s\n", ib_get_err_str(status));
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pd_alloc", "**pd_alloc %s", ib_get_err_str(status));
		    return mpi_errno;
		}
		IBU_Process.port = p_port_attr->port_num;
		IBU_Process.lid = p_port_attr->lid;
		MPIU_DBG_PRINTF(("port = %d, lid = %d, mtu = %d, max_cqes = %d, maxmsg = %d, link = %s\n",
		     p_port_attr->port_num,
		     cl_ntoh16(p_port_attr->lid),
		     p_port_attr->mtu,
		     p_ca_attr->max_cqes,
		     p_port_attr->max_msg_size,
		     ib_get_port_state_str(p_port_attr->link_state)));
		IBU_Process.hca_handle = hca_handle;
		IBU_Process.cq_size = p_ca_attr->max_cqes;
		cl_free( p_ca_attr );
		return MPI_SUCCESS;
	    }
	}

	/* free allocated mem */
	cl_free( p_ca_attr );
	ib_close_ca(hca_handle, NULL);
	
    }

    MPIU_Internal_error_printf("no channel adapters available.\n");
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**noca", 0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME ibu_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_init()
{
    ib_api_status_t status;
    ib_net64_t al_guid;
    uintn_t num_guids;
    size_t ca_size;
    void *ca_attr_ptr;
    uint32_t rkey;
    MPIDI_STATE_DECL(MPID_STATE_IBU_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_INIT);
    MPIU_DBG_PRINTF(("entering ibu_init\n"));

    /* Initialize globals */

#ifdef TRACE_IBU
    IBU_Process.outstanding_recvs = 0;
    IBU_Process.outstanding_sends = 0;
    IBU_Process.total_recvs = 0;
    IBU_Process.total_sends = 0;
#endif
    IBU_Process.cq_size = IBU_MAX_CQ_ENTRIES;
    /* get a handle to the infiniband access layer */
    status = ib_open_al(&IBU_Process.al_handle);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_init: ib_open_al failed, status %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    /* wait for 50 ms before querying al. This fixes a potential race 
       condition in al where ib_query is not ready with port information
       on faster systems
    */
    cl_thread_suspend( 50 );
    status = ibui_get_first_active_ca();
    if (status != MPI_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_init: get_first_active_ca failed.\n");
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }

    IBU_Process.ack_mem_ptr = ib_malloc_register(4096, &IBU_Process.ack_mr_handle, &IBU_Process.ack_lkey, &rkey);

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
    ib_close_ca(IBU_Process.hca_handle, NULL);
    ib_close_al(IBU_Process.al_handle);
    MPIU_DBG_PRINTF(("exiting ibu_finalize\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINALIZE);
    return IBU_SUCCESS;
}

void FooBar(void *p)
{
    MPIU_Internal_error_printf("FooBar should never be called.\n");
}

#undef FUNCNAME
#define FUNCNAME ibu_create_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_create_set(ibu_set_t *set)
{
    ib_api_status_t status;
    ib_cq_create_t cq_attr;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATE_SET);
    MPIU_DBG_PRINTF(("entering ibu_create_set\n"));
    /* create the completion queue */
    cq_attr.size = IBU_Process.cq_size;
    cq_attr.pfn_comp_cb = FooBar; /* completion routine */
    cq_attr.h_wait_obj = NULL; /* client specific wait object */
    status = ib_create_cq(IBU_Process.pd_handle, &cq_attr, NULL, NULL, set);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_create_set: ib_create_cq failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
	return status;
    }
    /*
    status = ib_rearm_cq(*set, TRUE);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("%s: error: ib_rearm_cq failed, %s\n", FCNAME, ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
	return IBU_FAIL;
    }
    */

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
    ib_api_status_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_DESTROY_SET);
    MPIU_DBG_PRINTF(("entering ibu_destroy_set\n"));
    status = ib_destroy_cq(set, NULL);
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

char * op2str(int wc_type)
{
    static char str[20];
    switch(wc_type)
    {
    case IB_WC_SEND:
	return "IB_WC_SEND";
    case IB_WC_RDMA_WRITE:
	return "IB_WC_RDMA_WRITE";
    case IB_WC_RECV:
	return "IB_WC_RECV";
    case IB_WC_RDMA_READ:
	return "IB_WC_RDMA_READ";
    case IB_WC_MW_BIND:
	return "IB_WC_MW_BIND";
    case IB_WC_FETCH_ADD:
	return "IB_WC_FETCH_ADD";
    case IB_WC_COMPARE_SWAP:
	return "IB_WC_COMPARE_SWAP";
    case IB_WC_RECV_RDMA_WRITE:
	return "IB_WC_RECV_RDMA_WRITE";
    }
    sprintf(str, "%d", wc_type);
    return str;
}

/* Port me to ibal
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
*/

#undef FUNCNAME
#define FUNCNAME ibu_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_wait(ibu_set_t set, int millisecond_timeout, void **vc_pptr, int *num_bytes_ptr, ibu_op_t *op_ptr)
{
    int i;
    ib_api_status_t status;
    ib_wc_t *p_in, *p_complete;
    ib_wc_t completion_data;
    void *mem_ptr;
    char *iter_ptr;
    ibu_t ibu;
    int num_bytes;
    unsigned int offset;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    int num_cq_entries;
#ifdef TRACE_IBU
    static int total_num_completions = 0;
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

	p_complete = NULL;
	completion_data.p_next = NULL;
	p_in = &completion_data;

	/*
	status = ib_rearm_cq(set, FALSE);
	if (status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf("%s: error: ib_rearm_cq failed, %s\n", FCNAME, ib_get_err_str(status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait -1\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	*/

	status = ib_poll_cq(set, &p_in, &p_complete); 
	if (status == IB_NOT_FOUND && p_complete == NULL)
	{
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
	if (status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf("%s: error: ib_poll_cq did not return IB_SUCCESS, %s\n", FCNAME, ib_get_err_str(status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 3\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	/*
	if (completion_data.status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf("%s: completion status = %s != IB_SUCCESS\n", 
		FCNAME, ib_get_err_str(completion_data.status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	*/

	/*printf("ib_poll_cq returned %d\n", completion_data.wc_type);fflush(stdout);*/

#ifdef HAVE_32BIT_POINTERS
	ibu = (ibu_t)(((ibu_work_id_handle_t*)&completion_data.wr_id)->data.ptr);
	mem_ptr = (void*)(((ibu_work_id_handle_t*)&completion_data.wr_id)->data.mem);
#else
	id_ptr = *((ibu_work_id_handle_t**)&completion_data.wr_id);
	ibu = (ibu_t)(id_ptr->ptr);
	mem_ptr = (void*)(id_ptr->mem);
	ibuBlockFree(g_workAllocator, (void*)id_ptr);
#endif
#ifdef USE_INLINE_PKT_RECEIVE
	mem_ptr_orig = mem_ptr;
#endif

	switch (completion_data.wc_type)
	{
	case IB_WC_SEND:
	    if (completion_data.status != IB_SUCCESS)
	    {
		num_cq_entries = 0;
		status = ib_query_cq(set, &num_cq_entries);
#ifdef TRACE_IBU
		MPIU_Internal_error_printf("%s: send completion status = %d = %s != IB_SUCCESS, cq size = %d, total completions = %d\n", 
					   FCNAME, completion_data.status, ib_get_err_str(completion_data.status), num_cq_entries, total_num_completions);
		MPIU_Internal_error_printf("oustanding recvs: %d, total_recvs %d\noutstanding sends: %d, total_sends: %d\n", IBU_Process.outstanding_recvs, IBU_Process.total_recvs, IBU_Process.outstanding_sends, IBU_Process.total_sends);
#else
		MPIU_Internal_error_printf("%s: send completion status = %d = %s != IB_SUCCESS, cq size = %d\n", 
					   FCNAME, completion_data.status, ib_get_err_str(completion_data.status), num_cq_entries);
#endif
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
#ifdef TRACE_IBU
	    total_num_completions++;
	    IBU_Process.outstanding_sends--;
	    IBU_Process.total_sends++;
#endif
	    if (mem_ptr == (void*)-1)
	    {
		/* flow control ack completed, no user data so break out here */
		break;
	    }
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
	case IB_WC_RECV:
	    if (completion_data.status != IB_SUCCESS)
	    {
		num_cq_entries = 0;
		status = ib_query_cq(set, &num_cq_entries);
#ifdef TRACE_IBU
		MPIU_Internal_error_printf("%s: recv completion status = %d = %s != IB_SUCCESS, cq size = %d, total completions = %d\n", 
					   FCNAME, completion_data.status, ib_get_err_str(completion_data.status), num_cq_entries, total_num_completions);
		MPIU_Internal_error_printf("oustanding recvs: %d, total_recvs %d\noutstanding sends: %d, total_sends: %d\n", IBU_Process.outstanding_recvs, IBU_Process.total_recvs, IBU_Process.outstanding_sends, IBU_Process.total_sends);
#else
		MPIU_Internal_error_printf("%s: recv completion status = %d = %s != IB_SUCCESS, cq size = %d\n", 
					   FCNAME, completion_data.status, ib_get_err_str(completion_data.status), num_cq_entries);
#endif
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
#ifdef TRACE_IBU
	    total_num_completions++;
	    IBU_Process.outstanding_recvs--;
	    IBU_Process.total_recvs++;
#endif
	    if (completion_data.recv.conn.recv_opt & IB_RECV_OPT_IMMEDIATE)
	    {
		ibu->nAvailRemote += completion_data.recv.conn.immediate_data;
		MPIDI_DBG_PRINTF((60, FCNAME, "%d packets acked, nAvailRemote now = %d", completion_data.recv.conn.immediate_data, ibu->nAvailRemote));
		ibuBlockFreeIB(ibu->allocator, mem_ptr);
		ibui_post_receive_unacked(ibu);
		assert(completion_data.length == 1); /* check this after the printfs to see if the immediate data is correct */
		break;
	    }
	    num_bytes = completion_data.length;
#ifdef USE_INLINE_PKT_RECEIVE
	    recv_vc_ptr = (MPIDI_VC *)(ibu->vc_ptr);
	    if (recv_vc_ptr->ch.reading_pkt)
	    {
		if (((MPIDI_CH3_Pkt_t *)mem_ptr)->type < MPIDI_CH3_PKT_END_CH3)
		{
		    /*printf("P");fflush(stdout);*/
		    MPIDI_CH3U_Handle_recv_pkt(recv_vc_ptr, mem_ptr);
		    if (recv_vc_ptr->ch.recv_active == NULL)
		    {
			recv_vc_ptr->ch.reading_pkt = TRUE;
		    }
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
		    continue;
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
	    if (completion_data.status != IB_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: unknown completion status = %s != IB_SUCCESS\n", 
					   FCNAME, ib_get_err_str(completion_data.status));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
	    MPIU_Internal_error_printf("%s: unknown ib wc_type: %s\n", FCNAME, op2str(completion_data.wc_type));
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

#if 0
#undef FUNCNAME
#define FUNCNAME ibu_post_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_write(ibu_t ibu, void *buf, int len)
{
    int num_bytes;
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_WRITE);
    MPIU_DBG_PRINTF(("entering ibu_post_write\n"));
    /*
    ibu->write.total = 0;
    ibu->write.buffer = buf;
    ibu->write.bufflen = len;
    ibu->write.use_iov = FALSE;
    ibu->state |= IBU_WRITING;
    ibu->pending_operations++;
    */
    ibu->state |= IBU_WRITING;

    num_bytes = ibui_post_write(ibu, buf, len, wfn);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "returning %d\n", num_bytes));
    return num_bytes;
}
#endif

#if 0
#undef FUNCNAME
#define FUNCNAME ibu_post_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_writev(ibu_t ibu, MPID_IOV *iov, int n)
{
    int num_bytes;
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_WRITEV);
    MPIU_DBG_PRINTF(("entering ibu_post_writev\n"));
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    /*ibu->write.iov = iov;*/
    /*
    memcpy(ibu->write.iov, iov, sizeof(MPID_IOV) * n);
    ibu->write.iovlen = n;
    ibu->write.index = 0;
    ibu->write.use_iov = TRUE;
    */
    ibu->state |= IBU_WRITING;
    /*
    {
	char str[1024], *s = str;
	int i;
	s += sprintf(s, "ibu_post_writev(");
	for (i=0; i<n; i++)
	    s += sprintf(s, "%d,", iov[i].MPID_IOV_LEN);
	sprintf(s, ")\n");
	MPIU_DBG_PRINTF(("%s", str));
    }
    */
    num_bytes = ibui_post_writev(ibu, iov, n, wfn);
    MPIU_DBG_PRINTF(("exiting ibu_post_writev\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_WRITEV);
    return IBU_SUCCESS;
}
#endif

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

#endif /* USE_IB_IBAL */
