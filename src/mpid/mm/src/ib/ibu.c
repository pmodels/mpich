/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ibimpl.h"
#include "ibu.h"
#include "iba.h"
#include "psc_iba.h"
#include "blockallocator.h"
#include <stdio.h>

typedef union ibu_work_id_handle_t
{
    ib_uint64_t id;
    struct ibu_data
    {
	ib_uint32_t ptr, mem;
    } data;
} ibu_work_id_handle_t;

typedef int IBU_STATE;
#define IBU_ACCEPTING  0x0001
#define IBU_ACCEPTED   0x0002
#define IBU_CONNECTING 0x0004
#define IBU_READING    0x0008
#define IBU_WRITING    0x0010

typedef struct ibu_buffer
{
    int use_iov;
    DWORD num_bytes;
    void *buffer;
    int bufflen;
    IBU_IOV iov[IBU_IOV_MAXLEN];
    int iovlen;
    int index;
    int total;
    int (*progress_update)(int,void*);
} ibu_buffer;

typedef struct ibu_state_t
{
    IBU_STATE state;
    ib_uint32_t lkey;
    ib_qp_handle_t qp_handle;
    BlockAllocator allocator;

    ib_uint32_t mtu_size;
    ib_uint32_t dlid;
    ib_mr_handle_t mr_handle;
    ib_uint32_t dest_qp_num;

    int closing;
    int pending_operations;
    /* read and write structures */
    ibu_buffer read;
    ibu_buffer write;
    /* user pointer */
    void *user_ptr;
} ibu_state_t;

#define IBU_ERROR_MSG_LENGTH       255
#define IBU_PACKET_SIZE            (1024 * 64)
#define IBU_PACKET_COUNT           64
#define IBU_NUM_PREPOSTED_RECEIVES 32
#define IBU_MAX_CQ_ENTRIES         255

typedef struct IBU_Global {
       ib_hca_handle_t hca_handle;
        ib_pd_handle_t pd_handle;
       ib_cqd_handle_t cqd_handle;
                   int lid;
       ib_hca_attr_t * attr_p;
		   int error;
		  char err_msg[IBU_ERROR_MSG_LENGTH];
} IBU_Global;

extern IBU_Global IBU_Process;

#define DEFAULT_NUM_RETRIES 10

static int g_connection_attempts = DEFAULT_NUM_RETRIES;
static int g_num_cp_threads = 2;


/* utility allocator functions */

typedef struct BlockAllocator_struct * BlockAllocator;

BlockAllocator BlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p));
int BlockAllocFinalize(BlockAllocator *p);
void * BlockAlloc(BlockAllocator p);
int BlockFree(BlockAllocator p, void *pBlock);

struct BlockAllocator_struct
{
    void **pNextFree;
    void *(* alloc_fn)(size_t size);
    void (* free_fn)(void *p);
    struct BlockAllocator_struct *pNextAllocation;
    unsigned int nBlockSize;
    int nCount, nIncrementSize;
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Lock_t lock;
#endif
};

static int g_nLockSpinCount = 100;

#ifdef WITH_ALLOCATOR_LOCKING

typedef volatile long MPIDU_Lock_t;

#include <errno.h>
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif

static inline void MPIDU_Init_lock( MPIDU_Lock_t *lock )
{
    *(lock) = 0;
}

static inline void MPIDU_Lock( MPIDU_Lock_t *lock )
{
    int i;
    for (;;)
    {
        for (i=0; i<g_nLockSpinCount; i++)
        {
            if (*lock == 0)
            {
#ifdef HAVE_INTERLOCKEDEXCHANGE
                if (InterlockedExchange((LPLONG)lock, 1) == 0)
                {
                    /*printf("lock %x\n", lock);fflush(stdout);*/
                    MPID_PROFILE_OUT(MPIDU_BUSY_LOCK);
                    return;
                }
#elif defined(HAVE_COMPARE_AND_SWAP)
                if (compare_and_swap(lock, 0, 1) == 1)
                {
                    MPID_PROFILE_OUT(MPIDU_BUSY_LOCK);
                    return;
                }
#else
#error Atomic memory operation needed to implement busy locks
#endif
            }
        }
        MPIDU_Yield();
    }
}

static inline void MPIDU_Unlock( MPIDU_Lock_t *lock )
{
    *(lock) = 0;
}

static inline void MPIDU_Busy_wait( MPIDU_Lock_t *lock )
{
    int i;
    for (;;)
    {
        for (i=0; i<g_nLockSpinCount; i++)
            if (!*lock)
            {
                return;
            }
        MPIDU_Yield();
    }
}

static inline void MPIDU_Free_lock( MPIDU_Lock_t *lock )
{
}

/*@
   MPIDU_Compare_swap - 

   Parameters:
+  void **dest
.  void *new_val
.  void *compare_val
.  MPIDU_Lock_t *lock
-  void **original_val

   Notes:
@*/
static inline int MPIDU_Compare_swap( void **dest, void *new_val, void *compare_val,            
                        MPIDU_Lock_t *lock, void **original_val )
{
    /* dest = pointer to value to be checked (address size)
       new_val = value to set dest to if *dest == compare_val
       original_val = value of dest prior to this operation */

#ifdef HAVE_NT_LOCKS
    /* *original_val = (void*)InterlockedCompareExchange(dest, new_val, compare_val); */
    *original_val = InterlockedCompareExchangePointer(dest, new_val, compare_val);
#elif defined(HAVE_COMPARE_AND_SWAP)
    if (compare_and_swap((volatile long *)dest, (long)compare_val, (long)new_val))
        *original_val = new_val;
#else
#error Locking functions not defined
#endif

    return 0;
}
#endif /* WITH_ALLOCATOR_LOCKING */

static BlockAllocator BlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p))
{
    BlockAllocator p;
    void **ppVoid;
    int i;

    p = alloc_fn( sizeof(struct BlockAllocator_struct) + ((blocksize + sizeof(void**)) * count) );

    p->alloc_fn = alloc_fn;
    p->free_fn = free_fn;
    p->nIncrementSize = incrementsize;
    p->pNextAllocation = NULL;
    p->nCount = count;
    p->nBlockSize = blocksize;
    p->pNextFree = (void**)(p + 1);
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Init_lock(&p->lock);
#endif

    ppVoid = (void**)(p + 1);
    for (i=0; i<count-1; i++)
    {
	*ppVoid = (void*)((char*)ppVoid + sizeof(void**) + blocksize);
	ppVoid = *ppVoid;
    }
    *ppVoid = NULL;

    return p;
}

static int BlockAllocFinalize(BlockAllocator *p)
{
    if (*p == NULL)
	return 0;
    BlockAllocFinalize(&(*p)->pNextAllocation);
    if ((*p)->free_fn != NULL)
	(*p)->free_fn(*p);
    *p = NULL;
    return 0;
}

static void * BlockAlloc(BlockAllocator p)
{
    void *pVoid;
    
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Lock(&p->lock);
#endif

    pVoid = p->pNextFree + 1;
    
    if (*(p->pNextFree) == NULL)
    {
	BlockAllocator pIter = p;
	while (pIter->pNextAllocation != NULL)
	    pIter = pIter->pNextAllocation;
	pIter->pNextAllocation = BlockAllocInit(p->nBlockSize, p->nIncrementSize, p->nIncrementSize, p->alloc_fn, p->free_fn);
	p->pNextFree = pIter->pNextFree;
    }
    else
	p->pNextFree = *(p->pNextFree);

#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Unlock(&p->lock);
#endif

    return pVoid;
}

static int BlockFree(BlockAllocator p, void *pBlock)
{
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Lock(&p->lock);
#endif

    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Unlock(&p->lock);
#endif

    return 0;
}




/* utility ibu functions */

static ib_uint32_t modifyQP( ibu_t ibu, Ib_qp_state qp_state )
{
    ib_uint32_t status;
    ib_qp_attr_list_t attrList;
    ib_address_vector_t av;
    attr_rec_t *attr_rec = NULL;

    if (qp_state == IB_QP_STATE_INIT)
    {
	if ((attr_rec = (attr_rec_t *)
	     malloc(sizeof (attr_rec_t) * 5)) == NULL )
	{
	    printf("Malloc failed %d\n", __LINE__);
	    return IBU_FAIL;
	}
	    
	((attr_rec[0]).id) = IB_QP_ATTR_PRIMARY_PORT;
	((attr_rec[0]).data) = 1;
	((attr_rec[1]).id) = IB_QP_ATTR_PRIMARY_P_KEY_IX;
	((attr_rec[1]).data) = 0;
	((attr_rec[2]).id) = IB_QP_ATTR_RDMA_W_F;
	((attr_rec[2]).data) = 1;
	((attr_rec[3]).id) = IB_QP_ATTR_RDMA_R_F;
	((attr_rec[3]).data) = 1;
	((attr_rec[4]).id) = IB_QP_ATTR_ATOMIC_F;
	((attr_rec[4]).data) = 0;
	    
	attrList.attr_num = 5;
	attrList.attr_rec_p = &attr_rec[0];    
    }
    else if (qp_state == IB_QP_STATE_RTR) 
    {
	av.sl                         = 0;
	/*printf("setting dest_lid to ibu->dlid: %d\n", ibu->dlid);*/
	av.dest_lid                   = (ib_uint16_t)ibu->dlid;
	av.grh_f                      = 0;
	av.path_bits                  = 0;
	av.max_static_rate            = 1;
	av.global.flow_label          = 1;
	av.global.hop_limit           = 1;
	av.global.src_gid_index       = 0;
	av.global.traffic_class       = 1;
	    
	if ((attr_rec = (attr_rec_t *)
	     malloc(sizeof (attr_rec_t) * 6)) == NULL )
	{
	    printf("Malloc failed %d\n", __LINE__);
	    return IBU_FAIL;
	}

	((attr_rec[0]).id) = IB_QP_ATTR_PRIMARY_ADDR;
	((attr_rec[0]).data) = (int)&av;
	((attr_rec[1]).id) = IB_QP_ATTR_DEST_QPN;
	((attr_rec[1]).data) = ibu->dest_qp_num;
	((attr_rec[2]).id) = IB_QP_ATTR_RCV_PSN;
	((attr_rec[2]).data) = 0;
	((attr_rec[3]).id) = IB_QP_ATTR_MTU;
	((attr_rec[3]).data) = ibu->mtu_size;
	((attr_rec[4]).id) = IB_QP_ATTR_RDMA_READ_LIMIT;
	((attr_rec[4]).data) = 4;
	((attr_rec[5]).id) = IB_QP_ATTR_RNR_NAK_TIMER;
	((attr_rec[5]).data) = 1;
	    
	attrList.attr_num = 6;
	attrList.attr_rec_p = &attr_rec[0];
    }
    else if (qp_state == IB_QP_STATE_RTS)
    {
	if ((attr_rec = (attr_rec_t *)
	     malloc(sizeof (attr_rec_t) * 5)) == NULL )
	{
	    printf("Malloc failed %d\n", __LINE__);
	    return IBU_FAIL;
	}

	((attr_rec[0]).id)    = IB_QP_ATTR_SEND_PSN;
	((attr_rec[0]).data)  = 0; 
	((attr_rec[1]).id)    = IB_QP_ATTR_TIMEOUT;
	((attr_rec[1]).data)  = 0x7c;
	((attr_rec[2]).id)    = IB_QP_ATTR_RETRY_COUNT;
	((attr_rec[2]).data)  = 2048;
	((attr_rec[3]).id)    = IB_QP_ATTR_RNR_RETRY_COUNT;
	((attr_rec[3]).data)  = 2048;
	((attr_rec[4]).id)    = IB_QP_ATTR_DEST_RDMA_READ_LIMIT;
	((attr_rec[4]).data)  = 4;
	
	attrList.attr_num = 5; 
	attrList.attr_rec_p = &attr_rec[0];
    }
    else if (qp_state == IB_QP_STATE_RESET)
    {
	attrList.attr_num = 0;
	attrList.attr_rec_p = NULL;
    }
    else
    {
	return IBU_FAIL;
    }

    status = ib_qp_modify_us(IBU_Process.hca_handle, 
			     ibu->qp_handle, 
			     qp_state, 
			     &attrList );
    if (attr_rec)    
	free(attr_rec);
    if( status != IBU_SUCCESS )
    {
	return status;
    }

    return IBU_SUCCESS;
}

static ib_uint32_t createQP(ibu_t ibu, ibu_set_t set)
{
    ib_uint32_t status;
    ib_qp_attr_list_t attrList;
    attr_rec_t attr_rec[] = {
	{IB_QP_ATTR_SERVICE_TYPE, IB_ST_RELIABLE_CONNECTION},
	{IB_QP_ATTR_SEND_CQ, 0},
	{IB_QP_ATTR_RCV_CQ, 0},
	{IB_QP_ATTR_SEND_REQ_MAX, 0},
	{IB_QP_ATTR_RCV_REQ_MAX, 0},
	{IB_QP_ATTR_SEND_SGE_MAX, 8},
	{IB_QP_ATTR_RCV_SGE_MAX, 8},
	{IB_QP_ATTR_SIGNALING_TYPE, QP_SIGNAL_ALL}
    };

    attr_rec[1].data = (int)set;
    attr_rec[2].data = (int)set;
    attr_rec[3].data = 255;
    attr_rec[4].data = 255;

    attrList.attr_num = sizeof(attr_rec)/sizeof(attr_rec[0]);
    attrList.attr_rec_p = &attr_rec[0];

    status = ib_qp_create_us(
	IBU_Process.hca_handle,
	IBU_Process.pd_handle,
	&attrList, 
	&ibu->qp_handle, 
	&ibu->dest_qp_num, 
	NULL);
    if (status != IBA_OK)
	return status;
    return IBU_SUCCESS;
}

static ib_mr_handle_t s_mr_handle;
static ib_uint32_t    s_lkey;
static void *ib_malloc_register(size_t size)
{
    ib_uint32_t status;
    void *ptr;
    ib_uint32_t rkey;

    ptr = malloc(size);
    if (ptr == NULL)
    {
	printf("malloc(%d) failed.\n", size);
	return NULL;
    }
    status = ib_mr_register_us(
	IBU_Process.hca_handle,
	(ib_uint8_t*)ptr,
	size,
	IBU_Process.pd_handle,
	IB_ACCESS_LOCAL_WRITE,
	&s_mr_handle,
	&s_lkey, &rkey);
    if (status != IBU_SUCCESS)
    {
	printf("ib_mr_register_us failed, error %d\n", status);
	return NULL;
    }

    return ptr;
}

static void ib_free_deregister(void *p)
{
    /*ib_mr_deregister_us(IBU_Process.hca_handle, s_mr_handle);*/
    free(p);
}

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/*
static int s_cur_receive = 0;
static int s_cur_send = 0;
int g_num_receive_posted = 0;
int g_num_send_posted = 0;
*/

static int ibui_post_receive(ibu_t ibu)
{
    ib_uint32_t status;
    ib_scatter_gather_list_t sg_list;
    ib_data_segment_t data;
    ib_work_req_rcv_t work_req;
    void *mem_ptr;

    mem_ptr = BlockAlloc(ibu->allocator);

    sg_list.data_seg_p = &data;
    sg_list.data_seg_num = 1;
    data.length = IBU_PACKET_SIZE;
    data.va = (ib_uint64_t)(ib_uint32_t)mem_ptr;
    data.l_key = ibu->lkey;
    work_req.op_type = OP_RECEIVE;
    work_req.sg_list = sg_list;
    /* store the VC ptr and the mem ptr in the work id */
    ((ibu_work_id_handle_t*)&work_req.work_req_id)->data.ptr = (ib_uint32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.work_req_id)->data.mem = (ib_uint32_t)mem_ptr;

    /*
    printf("ib_post_rcv_req_us %d\n", s_cur_receive++);
    g_num_receive_posted++;
    */
    status = ib_post_rcv_req_us(IBU_Process.hca_handle, 
				ibu->qp_handle,
				&work_req);
    if (status != IBU_SUCCESS)
    {
	printf("Error: failed to post ib receive, status = %d\n", status);
	return status;
    }

    return IBU_SUCCESS;
}

typedef struct ibu_num_written_node_t
{
    int num_bytes;
    struct ibu_num_written_node_t *next;
} ibu_num_written_node_t;

static ibu_num_written_node_t *g_write_list_head = NULL;
static ibu_num_written_node_t *g_write_list_tail = NULL;

static int ibui_next_num_written()
{
    ibu_num_written_node_t *p;
    int num_bytes;

    p = g_write_list_head;
    g_write_list_head = g_write_list_head->next;
    if (g_write_list_head == NULL)
	g_write_list_tail = NULL;
    num_bytes = p->num_bytes;
    free(p);
    return num_bytes;
}

static int ibui_post_write(ibu_t ibu, void *buf, int len, int (*write_progress_update)(int, void*))
{
    ib_uint32_t status;
    ib_scatter_gather_list_t sg_list;
    ib_data_segment_t data;
    ib_work_req_send_t work_req;
    void *mem_ptr;
    ibu_num_written_node_t *p;
    int length;

    while (len)
    {
	length = min(len, IBU_PACKET_SIZE);
	len -= length;

	p = malloc(sizeof(ibu_num_written_node_t));
	p->next = NULL;
	p->num_bytes = length;
	if (g_write_list_tail)
	{
	    g_write_list_tail->next = p;
	}
	else
	{
	    g_write_list_head = p;
	}
	g_write_list_tail = p;
	
	mem_ptr = BlockAlloc(ibu->allocator);
	memcpy(mem_ptr, buf, length);
	
	sg_list.data_seg_p = &data;
	sg_list.data_seg_num = 1;
	data.length = length;
	data.va = (ib_uint64_t)(ib_uint32_t)mem_ptr;
	data.l_key = ibu->lkey;
	
	work_req.dest_address      = 0;
	work_req.dest_q_key        = 0;
	work_req.dest_qpn          = 0; /*var.m_dest_qp_num;  // not needed */
	work_req.eecn              = 0;
	work_req.ethertype         = 0;
	work_req.fence_f           = 0;
	work_req.immediate_data    = 0;
	work_req.immediate_data_f  = 0;
	work_req.op_type           = OP_SEND;
	work_req.remote_addr.va    = 0;
	work_req.remote_addr.key   = 0;
	work_req.se_f              = 0;
	work_req.sg_list           = sg_list;
	work_req.signaled_f        = 0;
	
	/* store the VC ptr and the mem ptr in the work id */
	((ibu_work_id_handle_t*)&work_req.work_req_id)->data.ptr = (ib_uint32_t)ibu;
	((ibu_work_id_handle_t*)&work_req.work_req_id)->data.mem = (ib_uint32_t)mem_ptr;
	
	/*
	printf("ib_post_send_req_us %d\n", s_cur_send++);
	g_num_send_posted++;
	*/
	status = ib_post_send_req_us( IBU_Process.hca_handle,
	    ibu->qp_handle, 
	    &work_req);
	if (status != IBU_SUCCESS)
	{
	    printf("Error: failed to post ib send, status = %d, %s\n", status, iba_errstr(status));
	    return status;
	}

	buf = (char*)buf + length;
    }

    return IBU_SUCCESS;
}

static int ibui_post_writev(ibu_t ibu, IBU_IOV *iov, int n, int (*write_progress_update)(int, void*))
{
    int i;
    for (i=0; i<n; i++)
    {
	ibui_post_write(ibu, iov[i].IBU_IOV_BUF, iov[i].IBU_IOV_LEN, NULL);
    }
    return IBU_SUCCESS;
}

static inline void init_state_struct(ibu_state_t *p)
{
    /*p->set = 0;*/
    p->user_ptr = NULL;
    p->state = 0;
    p->closing = FALSE;
    p->pending_operations = 0;
    p->read.total = 0;
    p->read.num_bytes = 0;
    p->read.buffer = NULL;
    /*p->read.iov = NULL;*/
    p->read.iovlen = 0;
    p->read.progress_update = NULL;
    p->write.total = 0;
    p->write.num_bytes = 0;
    p->write.buffer = NULL;
    /*p->write.iov = NULL;*/
    p->write.iovlen = 0;
    p->write.progress_update = NULL;
}

/* ibu functions */

static BlockAllocator g_StateAllocator;

int ibu_init()
{
    ib_uint32_t status;
    ib_uint32_t max_cq_entries = IBU_MAX_CQ_ENTRIES+1;
    ib_uint32_t attr_size;
    MPIDI_STATE_DECL(MPID_STATE_IBU_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_INIT);

    /*ib_init_us();*/

    /* Initialize globals */
    /* get a handle to the host channel adapter */
    status = ib_hca_open_us(0 , &IBU_Process.hca_handle);
    if (status != IBU_SUCCESS)
    {
	printf("ibu_init: ib_hca_open_us failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    /* get a protection domain handle */
    status = ib_pd_allocate_us(IBU_Process.hca_handle, &IBU_Process.pd_handle);
    if (status != IBU_SUCCESS)
    {
	printf("ibu_init: ib_pd_allocate_us failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    /* get a completion queue domain handle */
    status = ib_cqd_create_us(IB_Process.hca_handle, &IB_Process.cqd_handle);
#if 0 /* for some reason this function fails when it really is ok */
    if (status != IBU_SUCCESS)
    {
	printf("ib_init: ib_cqd_create_us failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
#endif

#if 0
    /* create the completion queue */
    status = ib_cq_create_us(IBU_Process.hca_handle, 
	IBU_Process.cqd_handle,
	&max_cq_entries,
	&IBU_Process.cq_handle,
	NULL);
    if (status != IBU_SUCCESS)
    {
	printf("ibu_init: ib_cq_create_us failed, error %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return -1;
    }
#endif

    /* get the lid */
    attr_size = 0;
    status = ib_hca_query_us(IBU_Process.hca_handle, NULL, 
        HCA_QUERY_HCA_STATIC | HCA_QUERY_PORT_INFO_DYNAMIC, &attr_size);
    IBU_Process.attr_p = calloc(attr_size, sizeof(ib_uint8_t));
    status = ib_hca_query_us(IBU_Process.hca_handle, IBU_Process.attr_p, 
	HCA_QUERY_HCA_STATIC | HCA_QUERY_PORT_INFO_DYNAMIC, &attr_size);
    if (status != IBU_SUCCESS)
    {
	printf("ibu_init: ib_hca_query_us(HCA_QUERY_HCA_STATIC) failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    IBU_Process.lid = IBU_Process.attr_p->port_dynamic_info_p->lid;

    /* non infiniband initialization */
    g_StateAllocator = BlockAllocInit(sizeof(ibu_state_t), 1000, 500, malloc, free);

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
    return IBU_SUCCESS;
}

int ibu_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_FINALIZE);
    /*ib_release_us();*/
    BlockAllocFinalize(&g_StateAllocator);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINALIZE);
    return IBU_SUCCESS;
}

int ibu_create_set(ibu_set_t *set)
{
    ib_uint32_t status;
    ib_uint32_t max_cq_entries = IBU_MAX_CQ_ENTRIES+1;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATE_SET);
    /* create the completion queue */
    status = ib_cq_create_us(
	IBU_Process.hca_handle, 
	IBU_Process.cqd_handle,
	&max_cq_entries,
	set,
	NULL);
    if (status != IBU_SUCCESS)
    {
	printf("ibu_init: ib_cq_create_us failed, error %d\n", status);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
    return status;
}

int ibu_destroy_set(ibu_set_t set)
{
    ib_uint32_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_DESTROY_SET);
    status = ib_cq_destroy_us(IBU_Process.hca_handle, set);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_DESTROY_SET);
    return status;
}

/*
int ibu_listen(ibu_set_t set, void * user_ptr, int *port, ibu_t *listener)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_LISTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_LISTEN);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_LISTEN);
    return IBU_SUCCESS;
}

int ibu_post_connect(ibu_set_t set, void * user_ptr, char *host, int port, ibu_t *connected)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_CONNECT);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_CONNECT);
    return IBU_SUCCESS;
}

int ibu_accept(ibu_set_t set, void * user_ptr, ibu_t listener, ibu_t *accepted)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_ACCEPT);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_ACCEPT);
    return IBU_SUCCESS;
}

int ibu_post_close(ibu_t ibu)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_CLOSE);

    ibu->closing = TRUE;
    if (ibu->pending_operations == 0)
    {
    }
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_CLOSE);
    return IBU_SUCCESS;
}
*/

int ibu_wait(ibu_set_t set, int millisecond_timeout, ibu_wait_t *out)
{
    ib_uint32_t status;
    ib_work_completion_t completion_data;
    void *mem_ptr;
    ibu_t ibu;
    /*
    int error;
    DWORD num_bytes;
    ibu_state_t *ibu;
    */
    MPIDI_STATE_DECL(MPID_STATE_IBU_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WAIT);

    for (;;) 
    {
	status = ib_completion_poll_us(
	    IB_Process.hca_handle,
	    set,
	    &completion_data);
	if (status == IBA_CQ_EMPTY)
	{
	    /* ibu_wait polls until there is something in the queue */
	    /* or the timeout has expired */
	    continue;
	}
	if (status != IBA_OK)
	{
	    printf("error: ib_completion_poll_us did not return IBA_OK\n");
	    return IBU_FAIL;
	}
	if (completion_data.status != IB_COMP_ST_SUCCESS)
	{
	    printf("error: status = %d != IB_COMP_ST_SUCCESS, %s\n", 
		completion_data.status, iba_compstr(completion_data.status));
	    return IBU_FAIL;
	}

	//if (GetQueuedCompletionStatus(set, &num_bytes, (DWORD*)&ibu, &ovl, millisecond_timeout))

	ibu = (ibu_t)(((ibu_work_id_handle_t*)&completion_data.work_req_id)->data.ptr);
	mem_ptr = (void*)(((ibu_work_id_handle_t*)&completion_data.work_req_id)->data.mem);

	switch (completion_data.op_type)
	{
	case OP_SEND:
	    /*ib_handle_written(vc_ptr, mem_ptr, ibu_next_num_written());*/
	    /* put the send packet back in the pool */
	    /*BlockFree(allocator, mem_ptr);*/
	    break;
	case OP_RECEIVE:
	    /*ib_handle_read(vc_ptr, mem_ptr, completion_data.bytes_num);*/
	    /* put the receive packet back in the pool */
	    /*BlockFree(m_allocator, mem_ptr);*/
	    /* post another receive to replace the consumed one */
	    /*ibu_post_receive(vc_ptr);*/
	    break;
	default:
	    printf("unknown ib op_type: %d\n", completion_data.op_type);
	    break;
	}


#if 0
	    if (ibu->closing && ibu->pending_operations == 0)
	    {
		out->num_bytes = 0;
		out->error = 0;
		out->op_type = IBU_OP_CLOSE;
		out->user_ptr = ibu->user_ptr;
		/*BlockFree(g_ibu_allocator, ibu);*/
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_SUCCESS;
	    }
	    if (ovl == &ibu->read.ovl)
	    {
		ibu->read.total += num_bytes;
		if (ibu->read.use_iov)
		{
		    while (num_bytes)
		    {
			if (ibu->read.iov[ibu->read.index].IBU_IOV_LEN <= num_bytes)
			{
			    num_bytes -= ibu->read.iov[ibu->read.index].IBU_IOV_LEN;
			    ibu->read.index++;
			    ibu->read.iovlen--;
			}
			else
			{
			    ibu->read.iov[ibu->read.index].IBU_IOV_LEN -= num_bytes;
			    ibu->read.iov[ibu->read.index].IBU_IOV_BUF = 
				(char*)(ibu->read.iov[ibu->read.index].IBU_IOV_BUF) + num_bytes;
			    num_bytes = 0;
			}
		    }
		    if (ibu->read.iovlen == 0)
		    {
			out->num_bytes = ibu->read.total;
			out->op_type = IBU_OP_READ;
			out->user_ptr = ibu->user_ptr;
			ibu->pending_operations--;
			if (ibu->closing && ibu->pending_operations == 0)
			{
			    printf("ibu_wait: closing ibuet(%d) after iov read completed.\n", ibu_getid(ibu));
			    shutdown(ibu->ibu, SD_BOTH);
			    closeibuet(ibu->ibu);
			    ibu->ibu = IBU_INVALID_QP;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return IBU_SUCCESS;
		    }
		    /* make the user upcall */
		    if (ibu->read.progress_update != NULL)
			ibu->read.progress_update(num_bytes, ibu->user_ptr);
		    /* post a read of the remaining data */
		    WSARecv(ibu->ibu, ibu->read.iov, ibu->read.iovlen, &ibu->read.num_bytes, &dwFlags, &ibu->read.ovl, NULL);
		}
		else
		{
		    ibu->read.buffer = (char*)(ibu->read.buffer) + num_bytes;
		    ibu->read.bufflen -= num_bytes;
		    if (ibu->read.bufflen == 0)
		    {
			out->num_bytes = ibu->read.total;
			out->op_type = IBU_OP_READ;
			out->user_ptr = ibu->user_ptr;
			ibu->pending_operations--;
			if (ibu->closing && ibu->pending_operations == 0)
			{
			    printf("ibu_wait: closing ibuet(%d) after simple read completed.\n", ibu_getid(ibu));
			    shutdown(ibu->ibu, SD_BOTH);
			    closesocket(ibu->ibu);
			    ibu->ibu = IBU_INVALID_QP;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return IBU_SUCCESS;
		    }
		    /* make the user upcall */
		    if (ibu->read.progress_update != NULL)
			ibu->read.progress_update(num_bytes, ibu->user_ptr);
		    /* post a read of the remaining data */
		    ReadFile((HANDLE)(ibu->ibu), ibu->read.buffer, ibu->read.bufflen, &ibu->read.num_bytes, &ibu->read.ovl);
		}
	    }
	    else if (ovl == &ibu->write.ovl)
	    {
		if (ibu->state & IBU_CONNECTING)
		{
		    /* insert code here to determine that the connect succeeded */
		    /* ... */
		    ibu->state ^= IBU_CONNECTING; /* remove the IBU_CONNECTING bit */
		    
		    out->op_type = IBU_OP_CONNECT;
		    out->user_ptr = ibu->user_ptr;
		    ibu->pending_operations--;
		    if (ibu->closing && ibu->pending_operations == 0)
		    {
			printf("ibu_wait: closing ibuet(%d) after connect completed.\n", ibu_getid(ibu));
			shutdown(ibu->ibu, SD_BOTH);
			closesocket(ibu->ibu);
			ibu->ibu = IBU_INVALID_QP;
		    }
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return IBU_SUCCESS;
		}
		else
		{
		    /*printf("ibu_wait: write update, total = %d + %d = %d\n", ibu->write.total, num_bytes, ibu->write.total + num_bytes);*/
		    ibu->write.total += num_bytes;
		    if (ibu->write.use_iov)
		    {
			while (num_bytes)
			{
			    if (ibu->write.iov[ibu->write.index].IBU_IOV_LEN <= num_bytes)
			    {
			    /*printf("ibu_wait: write.index %d, len %d\n", ibu->write.index, 
				ibu->write.iov[ibu->write.index].IBU_IOV_LEN);*/
				num_bytes -= ibu->write.iov[ibu->write.index].IBU_IOV_LEN;
				ibu->write.index++;
				ibu->write.iovlen--;
			    }
			    else
			    {
			    /*printf("ibu_wait: partial data written [%d].len = %d, num_bytes = %d\n", ibu->write.index,
				ibu->write.iov[ibu->write.index].IBU_IOV_LEN, num_bytes);*/
				ibu->write.iov[ibu->write.index].IBU_IOV_LEN -= num_bytes;
				ibu->write.iov[ibu->write.index].IBU_IOV_BUF =
				    (char*)(ibu->write.iov[ibu->write.index].IBU_IOV_BUF) + num_bytes;
				num_bytes = 0;
			    }
			}
			if (ibu->write.iovlen == 0)
			{
			    out->num_bytes = ibu->write.total;
			    out->op_type = IBU_OP_WRITE;
			    out->user_ptr = ibu->user_ptr;
			    ibu->pending_operations--;
			    if (ibu->closing && ibu->pending_operations == 0)
			    {
				printf("ibu_wait: closing ibuet(%d) after iov write completed.\n", ibu_getid(ibu));
				shutdown(ibu->ibu, SD_BOTH);
				closeibuet(ibu->ibu);
				ibu->ibu = IBU_INVALID_QP;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return IBU_SUCCESS;
			}
			/* make the user upcall */
			if (ibu->write.progress_update != NULL)
			    ibu->write.progress_update(num_bytes, ibu->user_ptr);
			/* post a write of the remaining data */
			printf("ibu_wait: posting write of the remaining data, vec size %d\n", ibu->write.iovlen);
			WSASend(ibu->ibu, ibu->write.iov, ibu->write.iovlen, &ibu->write.num_bytes, 0, &ibu->write.ovl, NULL);
		    }
		    else
		    {
			ibu->write.buffer = (char*)(ibu->write.buffer) + num_bytes;
			ibu->write.bufflen -= num_bytes;
			if (ibu->write.bufflen == 0)
			{
			    out->num_bytes = ibu->write.total;
			    out->op_type = IBU_OP_WRITE;
			    out->user_ptr = ibu->user_ptr;
			    ibu->pending_operations--;
			    if (ibu->closing && ibu->pending_operations == 0)
			    {
				printf("ibu_wait: closing ibuet(%d) after simple write completed.\n", ibu_getid(ibu));
				shutdown(ibu->ibu, SD_BOTH);
				closeibuet(ibu->ibu);
				ibu->ibu = IBU_INVALID_QP;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return IBU_SUCCESS;
			}
			/* make the user upcall */
			if (ibu->write.progress_update != NULL)
			    ibu->write.progress_update(num_bytes, ibu->user_ptr);
			/* post a write of the remaining data */
			WriteFile((HANDLE)(ibu->ibu), ibu->write.buffer, ibu->write.bufflen, &ibu->write.num_bytes, &ibu->write.ovl);
		    }
		}
	    }
	    else
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
#endif
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
}

int ibu_set_user_ptr(ibu_t ibu, void *user_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_SET_USER_PTR);
    if (ibu == IBU_INVALID_QP)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_SET_USER_PTR);
	return IBU_FAIL;
    }
    ibu->user_ptr = user_ptr;
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_SET_USER_PTR);
    return IBU_SUCCESS;
}

/* immediate functions */
/* infiniband has no immediate functions */
/*
int ibu_read(ibu_t ibu, void *buf, int len, int *num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_READ);
    *num_read = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_READ);
    return IBU_SUCCESS;
}

int ibu_readv(ibu_t ibu, IBU_IOV *iov, int n, int *num_read)
{
    DWORD nFlags = 0;
    MPIDI_STATE_DECL(MPID_STATE_IBU_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_READV);
    *num_read = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_READV);
    return IBU_SUCCESS;
}

int ibu_write(ibu_t ibu, void *buf, int len, int *num_written)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITE);
    *num_written = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
    return IBU_SUCCESS;
}

int ibu_writev(ibu_t ibu, IBU_IOV *iov, int n, int *num_written)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITEV);
    *num_written = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
    return IBU_SUCCESS;
}
*/

/* non-blocking functions */

int ibu_post_read(ibu_t ibu, void *buf, int len, int (*rfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_READ);
    ibu->read.total = 0;
    ibu->read.buffer = buf;
    ibu->read.bufflen = len;
    ibu->read.use_iov = FALSE;
    ibu->read.progress_update = rfn;
    ibu->state |= IBU_READING;
    ibu->pending_operations++;
    /*ReadFile((HANDLE)(ibu->ibu), buf, len, &ibu->read.num_bytes, &ibu->read.ovl);*/
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_READ);
    return IBU_SUCCESS;
}

int ibu_post_readv(ibu_t ibu, IBU_IOV *iov, int n, int (*rfn)(int, void*))
{
    DWORD flags = 0;
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_READV);
    ibu->read.total = 0;
    /*ibu->read.iov = iov;*/
    memcpy(ibu->read.iov, iov, sizeof(IBU_IOV) * n);
    ibu->read.iovlen = n;
    ibu->read.index = 0;
    ibu->read.use_iov = TRUE;
    ibu->read.progress_update = rfn;
    ibu->state |= IBU_READING;
    ibu->pending_operations++;
    /*WSARecv(ibu->ibu, ibu->read.iov, n, &ibu->read.num_bytes, &flags, &ibu->read.ovl, NULL);*/
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_READV);
    return IBU_SUCCESS;
}

int ibu_post_write(ibu_t ibu, void *buf, int len, int (*wfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_WRITE);
    ibu->write.total = 0;
    ibu->write.buffer = buf;
    ibu->write.bufflen = len;
    ibu->write.use_iov = FALSE;
    ibu->write.progress_update = wfn;
    ibu->state |= IBU_WRITING;
    ibu->pending_operations++;
    /*WriteFile((HANDLE)(ibu->ibu), buf, len, &ibu->write.num_bytes, &ibu->write.ovl);*/
    ibui_post_write(ibu, buf, len, wfn);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_WRITE);
    return IBU_SUCCESS;
}

int ibu_post_writev(ibu_t ibu, IBU_IOV *iov, int n, int (*wfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_WRITEV);
    ibu->write.total = 0;
    /*ibu->write.iov = iov;*/
    memcpy(ibu->write.iov, iov, sizeof(IBU_IOV) * n);
    ibu->write.iovlen = n;
    ibu->write.index = 0;
    ibu->write.use_iov = TRUE;
    ibu->write.progress_update = wfn;
    ibu->state |= IBU_WRITING;
    ibu->pending_operations++;
    /*
    {
	char str[1024], *s = str;
	int i;
	s += sprintf(s, "ibu_post_writev(");
	for (i=0; i<n; i++)
	    s += sprintf(s, "%d,", iov[i].IBU_IOV_LEN);
	sprintf(s, ")\n");
	printf("%s", str);
    }
    */
    /*WSASend(ibu->ibu, ibu->write.iov, n, &ibu->write.num_bytes, 0, &ibu->write.ovl, NULL);*/
    ibui_post_writev(ibu, ibu->write.iov, n, wfn);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_WRITEV);
    return IBU_SUCCESS;
}

/* extended functions */

int ibu_get_lid()
{
    return IBU_Process.lid;
}

#if 0
int ibu_getid(ibu_t ibu)
{
    return 0;
}

int ibu_easy_receive(ibu_t ibu, void *buf, int len, int *num_read)
{
    int error;
    int n;
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_IBU_EASY_RECEIVE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_EASY_RECEIVE);

    while (len)
    {
	error = ibu_read(ibu, buf, len, &n);
	if (error != IBU_SUCCESS)
	{
	    *num_read = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_EASY_RECEIVE);
	    return error;
	}
	total += n;
	buf = (char*)buf + n;
	len -= n;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_EASY_RECEIVE);
    return IBU_SUCCESS;
}

int ibu_easy_send(ibu_t ibu, void *buf, int len, int *num_written)
{
    int error;
    int n;
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_IBU_EASY_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_EASY_SEND);

    while (len)
    {
	error = ibui_write(ibu, buf, len, &n);
	if (error != IBU_SUCCESS)
	{
	    *num_written = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_EASY_SEND);
	    return error;
	}
	total += n;
	buf = (char*)buf + n;
	len -= n;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_EASY_SEND);
    return IBU_SUCCESS;
}
#endif
