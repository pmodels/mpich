/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDIMPL_H
#define MPIDIMPL_H

#include "mpiimpl.h"
#include "bsocket.h"
#include "sock.h"
#include "blockallocator.h"
#include <assert.h>

#ifdef USE_MPE_PROFILING
#include "mm_mpe_prof.h"
#endif

#ifdef MPICH_DEV_BUILD
#define INVALID_POINTER ((void*)0xcdcdcdcd)
#endif

#define MM_KVS_NAME_LENGTH 100
/* key used by spawners and spawnees to get the port by which they can connect to each other */
#define MPICH_PARENT_PORT_KEY     "MPI_Parent_port"
/* key used to tell comm_accept that it doesn't need to transfer pmi databases */
#define MPICH_PMI_SAME_DOMAIN_KEY "PMI_SAME_DOMAIN"
/* key used to inform spawned processes that their parent is mpiexec and not another mpi application */
#define MPICH_EXEC_IS_PARENT_KEY  "MPIEXECSpawned"

typedef struct OpenPortNode {
                   char  port_name[MPI_MAX_PORT_NAME];
                    int  bfd;
    struct OpenPortNode *next;
} OpenPortNode;

typedef enum MM_METHOD { 
    MM_NULL_METHOD,
    MM_UNBOUND_METHOD,
    MM_PACKER_METHOD,
    MM_UNPACKER_METHOD,
#ifdef WITH_METHOD_SHM
    MM_SHM_METHOD, 
#endif
#ifdef WITH_METHOD_TCP
    MM_TCP_METHOD, 
#endif
#ifdef WITH_METHOD_SOCKET
    MM_SOCKET_METHOD,
#endif
#ifdef WITH_METHOD_IB
    MM_IB_METHOD,
#endif
#ifdef WITH_METHOD_VIA
    MM_VIA_METHOD,
#endif
#ifdef WITH_METHOD_VIA_RDMA
    MM_VIA_RDMA_METHOD,
#endif
#ifdef WITH_METHOD_NEW
    MM_NEW_METHOD,
#endif
    MM_END_MARKER_METHOD
} MM_METHOD;

typedef struct MPID_PerProcess {
      MPID_Thread_lock_t lock;
      MPID_Thread_lock_t qlock;
         struct MM_Car * posted_q_head;    /* unmatched posted read operations */
         struct MM_Car * posted_q_tail;
         struct MM_Car * unex_q_head;      /* active un-matched read operations */
         struct MM_Car * unex_q_tail;
      MPID_Thread_lock_t cqlock;
         struct MM_Car * cq_head;          /* completion queue head */
         struct MM_Car * cq_tail;          /* completion queue tail */
                  char * pmi_kvsname; /*[MM_KVS_NAME_LENGTH];*/
             MPID_Comm * comm_parent;
          OpenPortNode * port_list;
          BlockAllocator VCTable_allocator; /* memory allocator for vc tables */
          BlockAllocator VC_allocator;      /* memory allocator for vc's */
       struct MPIDI_VC * packer_vc_ptr;
       struct MPIDI_VC * unpacker_vc_ptr;
               MM_METHOD method_order[MM_END_MARKER_METHOD];
	             int num_ordered_methods;
} MPID_PerProcess;

extern MPID_PerProcess MPID_Process;

typedef union VC_Method_data
{
#ifdef WITH_METHOD_SOCKET
    struct vc_socket
    {
	struct MPIDI_VC *connect_vc_ptr;
	int state;
	sock_t sock;
    } socket;
#endif
#ifdef WITH_METHOD_TCP
    struct vc_tcp
    {
        int connected;
        int connecting;
        int reject_received;
        int bfd;
	int (*read)(struct MPIDI_VC *vc_ptr);
	int bytes_of_header_read;
    } tcp;
#endif
#ifdef WITH_METHOD_SHM
    struct vc_shm
    {
        void *shm_ptr;
    } shm;
#endif
#ifdef WITH_METHOD_IB
    struct vc_ib
    {
	int reading_header;
	IB_Info info;
    } ib;
#endif
#ifdef WITH_METHOD_VIA
    struct vc_via
    {
        VI_Info info;
    } via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    struct vc_via_rdma
    {
        VI_Info info;
    } via_rdma;
#endif
#ifdef WITH_METHOD_NEW
    struct vc_new
    {
        int data;
    }
#endif
} VC_Method_data;

typedef struct MPIDI_VC_functions
{
    int (*post_read)(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
    int (*enqueue_read_at_head)(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
    int (*merge_with_unexpected)(MM_Car *posted_car_ptr, MM_Car *unex_car_ptr);
    int (*merge_with_posted)(MM_Car *pkt_car_ptr, MM_Car *posted_car_ptr);
    int (*merge_unexpected_data)(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr, char *buffer, int length);
    int (*post_write)(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
    int (*enqueue_write_at_head)(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
    int (*reset_car)(struct MM_Car *car_ptr);
    int (*setup_packet_car)(struct MPIDI_VC *vc_ptr, MM_CAR_TYPE read_write, int src_dest, struct MM_Car *car_ptr);
    int (*post_read_pkt)(struct MPIDI_VC *vc_ptr);
} MPIDI_VC_functions;

typedef struct MPIDI_VC
{
 MPID_Thread_lock_t lock;
          MM_METHOD method;
       volatile int ref_count;
    struct MM_Car * writeq_head;
    struct MM_Car * writeq_tail;
    struct MM_Car * readq_head;
    struct MM_Car * readq_tail;
             char * pmi_kvsname; /* name of the key_value database where the remote process put its business card */
                int rank; /* the rank of the remote process relative to MPI_COMM_WORLD in the key_value database described by pmi_kvsname */
 MPIDI_VC_functions fn;
  struct MPIDI_VC * read_next_ptr;
  struct MPIDI_VC * write_next_ptr;
             MM_Car pkt_car; /* used to enqueue the read of the packet */
     VC_Method_data data;
} MPIDI_VC;

typedef struct MPIDI_VCRT
{
    volatile int ref_count;
     MPIDI_VC ** table_ptr;
           int * lid_ptr;
} MPIDI_VCRT;

/*** multi-method prototypes ***/
#include "mm_packer.h"
#include "mm_unpacker.h"
#ifdef WITH_METHOD_SHM
#include "mm_shm.h"
#endif
#ifdef WITH_METHOD_TCP
#include "mm_tcp.h"
#endif
#ifdef WITH_METHOD_SOCKET
#include "mm_socket.h"
#endif
#ifdef WITH_METHOD_IB
#include "mm_ib.h"
#endif
#ifdef WITH_METHOD_VIA
#include "mm_via.h"
#endif
#ifdef WITH_METHOD_VIA_RDMA
#include "mm_via_rdma.h"
#endif
#ifdef WITH_METHOD_NEW
#include "mm_new.h"
#endif
/* connect/accept */
           int mm_open_port(MPID_Info *, char *);
           int mm_close_port(char *);
           int mm_accept(MPID_Info *, char *);
           int mm_connect(MPID_Info *, char *);
           int mm_send(int, char *, int);
           int mm_recv(int, char *, int);
           int mm_close(int);

/* requests */
MPID_Request * mm_request_alloc(void);
          void mm_request_free(MPID_Request *request_ptr);

/* communication agent/action requests */
          void mm_car_init(void);
          void mm_car_finalize(void);
      MM_Car * mm_car_alloc(void);
          void mm_car_free(MM_Car *car_ptr);

/* virtual connections */
          void mm_vc_init(void);
          void mm_vc_finalize(void);
    MPIDI_VC * mm_vc_from_communicator(MPID_Comm *comm_ptr, int rank);
    MPIDI_VC * mm_vc_from_context(int comm_context, int rank);
    MPIDI_VC * mm_vc_alloc(MM_METHOD method);
    MPIDI_VC * mm_vc_connect_alloc(MPID_Comm *comm_ptr, int rank);
           int mm_vc_free(MPIDI_VC *ptr);

/* buffer */
           int mm_choose_buffer(MPID_Request *request_ptr);
           int mm_reset_cars(MPID_Request *request_ptr);
           int mm_get_buffers_tmp(MPID_Request *request_ptr);
	   int mm_release_buffers_tmp(MPID_Request *request_ptr);
           int mm_get_buffers_vec(MPID_Request *request_ptr);
	   int vec_buffer_init(MPID_Request *request_ptr);
	   int tmp_buffer_init(MPID_Request *request_ptr);
	   int simple_buffer_init(MPID_Request *request_ptr);

	   int mm_make_progress(void);

/* queues */
           int mm_post_recv(MM_Car *car_ptr);
           int mm_post_send(MM_Car *car_ptr);
           int mm_cq_test(void);
           int mm_cq_wait(void);
           int mm_cq_enqueue(MM_Car *car_ptr);
	   int mm_create_post_unex(MM_Car *unex_head_car_ptr);
	   int mm_enqueue_request_to_send(MM_Car *unex_head_car_ptr);
	   int mm_post_rndv_clear_to_send(MM_Car *posted_car_ptr, MM_Car *rndv_rts_car_ptr);
	   int mm_post_rndv_data_send(MM_Car *rndv_cts_car_ptr);

/* requests */
/*
          void mm_inc_cc(MPID_Request *request_ptr);
          void mm_dec_cc(MPID_Request *request_ptr);
	  void mm_dec_atomic(int *pcounter);
	  void mm_inc_atomic(int *pcounter);
*/
#define mm_inc_cc(request_ptr) (*((request_ptr)->cc_ptr))++
#define mm_dec_cc(request_ptr) (*((request_ptr)->cc_ptr))--
#define mm_inc_cc_atomic(request_ptr) (*((request_ptr)->cc_ptr))++
#define mm_dec_cc_atomic(request_ptr) (*((request_ptr)->cc_ptr))--
/*
#define mm_inc_cc(request_ptr) { printf("mm_inc_cc(%d,", (*((request_ptr)->cc_ptr))++); printf("%d)\n", *((request_ptr)->cc_ptr)); fflush(stdout); }
#define mm_dec_cc(request_ptr) { printf("mm_dec_cc(%d,", (*((request_ptr)->cc_ptr))--); printf("%d)\n", *((request_ptr)->cc_ptr)); fflush(stdout); }
*/
#define mm_dec_atomic(pcounter) (*(pcounter))++
#define mm_inc_atomic(pcounter) (*(pcounter))--

/*
What is an xfer block? - A block is defined by an init call, followed by one or more
xfer operations, and finally a start call.
Priority in a block is defined by the order of operations issued in a block.
Lower priority operations may make progress as long as they can be preempted
by higher priority operations.

Progress on blocks may occur in parallel.  Blocks are independent.

dependencies:
1) completion
2) progress
3) resource (buffer)

Xfer blocks are used to create graphs of cars. Multiple cars within an xfer 
block may be aggregated in the order issued.

mop = MPI operation
Do not add xfer_mop_send_op because it is not necessary at the xfer level.

The xfer...mop functions generate cars like this:
(recv_mop) - (send)
(recv) - (mop_send)

The mop_send exists at the car level, not the xfer level.

xfer_recv_forward_op generates these cars:
(recv) - (send)

recv_mop can be used for accumulate

send_mop could cause remote operations to occur.  We will not use send_mop 
currently.
*/

int xfer_init               (int tag, MPID_Comm *comm_ptr, MPID_Request **request_pptr);
int xfer_recv_op            (MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_recv_mop_op        (MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_recv_forward_op    (MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src, int dest);
int xfer_recv_mop_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src, int dest);
int xfer_forward_op         (MPID_Request *request_ptr, int size, int src, int dest);
int xfer_send_op            (MPID_Request *request_ptr, const void *buf, int count, MPI_Datatype dtype, int first, int last, int dest);
int xfer_replicate_op       (MPID_Request *request_ptr, int dest);
int xfer_start              (MPID_Request *request_ptr);

/* Here are the xfer functions broken into scatter and gather routines.
int xfer_gather_init(int dest, int tag, MPID_Comm *comm_ptr, MPID_Request **request_pptr);
int xfer_gather_recv_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_gather_recv_mop_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_gather_recv_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_gather_recv_mop_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_gather_forward_op(MPID_Request *request_ptr, int size);
int xfer_gather_send_op(MPID_Request *request_ptr, const void *buf, int count, MPI_Datatype dtype, int first, int last);
int xfer_gather_start(MPID_Request *request_ptr);

int xfer_scatter_init(int src, int tag, MPID_Comm *comm_ptr, MPID_Request **request_pptr);
int xfer_scatter_recv_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last);
int xfer_scatter_recv_mop_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last);
int xfer_scatter_recv_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int dest);
int xfer_scatter_recv_mop_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int dest);
int xfer_scatter_forward_op(MPID_Request *request_ptr, int size, int dest);
int xfer_scatter_send_op(MPID_Request *request_ptr, const void *buf, int count, MPI_Datatype dtype, int first, int last, int dest);
int xfer_scatter_replicate_op(MPID_Request *request_ptr, int dest);
int xfer_scatter_start(MPID_Request *request_ptr);
*/

#endif
