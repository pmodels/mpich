/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef IBUIMPL_VAPI_H
#define IBUIMPL_VAPI_H

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct ibuBlock_t
{
    struct ibuBlock_t *next;
    VAPI_mr_hndl_t handle;
    VAPI_lkey_t lkey;
    ibu_rdma_buf_t data; /* Mellanox: changed by dafna July 7th */
} ibuBlock_t;

typedef struct ibuQueue_t
{
    struct ibuQueue_t *next_q;
    ibuBlock_t *pNextFree;
    ibuBlock_t block[IBU_PACKET_COUNT];
} ibuQueue_t;

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

typedef struct ibui_send_wqe_info_t 
{
    int length; /* Send operation length*/
    ibu_rdma_type_t RDMA_type;
    void* mem_ptr;
} ibui_send_wqe_info_t;

typedef struct ibui_send_wqe_info_fifo_t
{
    ibui_send_wqe_info_t entries[IBU_DEFAULT_MAX_WQE];
    int head;
    int tail;
    int num_of_signaled_wqes; /* used to avoid CQ polling when empty*/
} ibui_send_wqe_info_fifo_t;

typedef struct ibu_work_id_handle_t
{
    void *mem;
    ibu_t ibu;
    int length;
} ibu_work_id_handle_t;

typedef int IBU_STATE;
#define IBU_READING      0x0001
#define IBU_WRITING      0x0002
#define IBU_RDMA_WRITING 0x0004
#define IBU_RDMA_READING 0x0008

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
    ibuQueue_t *allocator;

    ibu_lid_t dlid;
    VAPI_qp_num_t qp_num, dest_qp_num;

    int closing;
    int pending_operations;
    /* read and write structures */
    ibu_buffer_t read;
    ibu_unex_read_t *unex_list;
    ibu_buffer_t write;

    /* vc pointer */
    MPIDI_VC_t *vc_ptr;
    /* unexpected queue pointer */
    struct ibu_state_t *unex_finished_queue;
    unsigned int max_inline_size;

/* Mellanox: Added by dafna July 7th */

    ibu_rdma_buf_t *local_RDMA_buf_base;   /* Reciver RDMA buffers base address*/
    ibu_mem_t local_RDMA_buf_hndl;	   /* Keys for RDMA Write*/
    int local_RDMA_head;		   /* Receiver local: Buffer Index that should be polled  */
    int local_last_updated_RDMA_limit;	   /* Receiver: tracking the pigyback updated last remote_sndbuf_tail  */
    ibu_rdma_buf_t *remote_RDMA_buf_base;  /* Base Address for RDMA Writes */
    ibu_mem_t remote_RDMA_buf_hndl;	   /* Keys for RDMA Write*/
    int remote_RDMA_head;		   /* Sender   remote index to write to */
    int remote_RDMA_limit;		   /* Sender limit for RDMA outs ops. head should not bypass this value */
    ibui_send_wqe_info_fifo_t send_wqe_info_fifo;

/* Mellanox: END Added by dafna July 7th */
} ibu_state_t;

typedef struct IBU_Global {
    VAPI_hca_hndl_t  hca_handle;
    VAPI_pd_hndl_t   pd_handle;
    VAPI_hca_port_t  hca_port;
    IB_port_t        port;
    VAPI_cqe_num_t   cq_size;
    ibu_lid_t        lid;
    ibu_state_t *    unex_finished_list;
    int              error;
    char             err_msg[IBU_ERROR_MSG_LENGTH];
    long	     offset_to_lkey;
    ibuBlockAllocator workAllocator;
    int		     num_send_cqe;
} IBU_Global;

extern IBU_Global IBU_Process;

#define GETLKEY(p) (((ibuBlock_t*)((char *)p - IBU_Process.offset_to_lkey))->lkey)

/* prototypes */
int ibui_post_ack_write(ibu_t ibu);
int ibui_post_rndv_cts_iov_reg_err(ibu_t ibu, MPID_Request * rreq);
int ibui_buffer_unex_read(ibu_t ibu, void *mem_ptr, unsigned int offset, unsigned int num_bytes);
ibu_rdma_buf_t* ibui_RDMA_buf_init(ibu_t ibu, VAPI_rkey_t *rkey);
int ibui_update_remote_RDMA_buf(ibu_t ibu, ibu_rdma_buf_t *buf, VAPI_rkey_t rkey);
int send_wqe_info_fifo_empty(int head, int tail);
void send_wqe_info_fifo_pop(ibu_t ibu, ibui_send_wqe_info_t* entry);
int send_wqe_info_fifo_full(int head, int tail);
void send_wqe_info_fifo_push(ibu_t ibu, ibu_rdma_type_t entry_type, void* mem_ptr, int length);
int ibui_signaled_completion(ibu_t ibu);

/* utility allocator functions */

ibuBlockAllocator ibuBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(size_t size), void (* free_fn)(void *p));
ibuQueue_t * ibuBlockAllocInitIB();
ibu_rdma_buf_t *ibuRDMAAllocInitIB(ibu_mem_t *handle);
int ibuBlockAllocFinalize(ibuBlockAllocator *p);
int ibuBlockAllocFinalizeIB(ibuQueue_t *p);
int ibuRDMAAllocFinalizeIB(ibu_rdma_buf_t *buf,VAPI_mr_hndl_t handle);
void * ibuBlockAlloc(ibuBlockAllocator p);
void * ibuBlockAllocIB(ibuQueue_t *p);
int ibuBlockFree(ibuBlockAllocator p, void *pBlock);
int ibuBlockFreeIB(ibuQueue_t *p, void *pBlock);

void *ib_malloc_register(size_t size, VAPI_mr_hndl_t *mhp, VAPI_lkey_t *lp, VAPI_rkey_t *rp);
void ib_free_deregister(void *p/*, VAPI_mr_hndl_t mem_handle*/);

#endif
