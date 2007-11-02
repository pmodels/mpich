/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidi_ch3i_ssm_conf.h"
#include "mpidi_ch3_conf.h"
#include "mpidimpl.h"
#include "mpidu_process_locks.h"
#include "ch3i_progress.h"
#include "ch3usock.h"

/* Redefine MPIU_CALL since the ssm channel should be self-contained.
   This only affects the building of a dynamically loadable library for 
   the sock channel, and then only when debugging is enabled */
#undef MPIU_CALL
#define MPIU_CALL(context,funccall) context##_##funccall

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
/* FIXME: We should only include the ones of these that we use within this channel */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
/* FIXME: Note the use of the USE_SYSV_MQ to only include this header includes
   when needed.  The other headers should be similarly organized */
#if defined(HAVE_SYS_MSG_H) && defined(USE_SYSV_MQ)
/* Needed for msgsnd and msgrcv */
#include <sys/msg.h>
#endif
#ifdef HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_MQUEUE_H
#include <mqueue.h>
#endif
#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif

#ifdef HAVE_GCC_AND_PENTIUM_ASM
#ifdef HAVE_GCC_ASM_AND_X86_SFENCE
#define MPID_WRITE_BARRIER() __asm__ __volatile__  ( "sfence" ::: "memory" )
#else
#define MPID_WRITE_BARRIER()
#endif
#ifdef HAVE_GCC_ASM_AND_X86_LFENCE
#define MPID_READ_BARRIER() __asm__ __volatile__  ( ".byte 0x0f, 0xae, 0xe8" ::: "memory" )
#else
#define MPID_READ_BARRIER()
#endif
#ifdef HAVE_GCC_ASM_AND_X86_MFENCE
#define MPID_READ_WRITE_BARRIER() __asm__ __volatile__  ( ".byte 0x0f, 0xae, 0xf0" ::: "memory" )
#else
#define MPID_READ_WRITE_BARRIER()
#endif

#elif defined(HAVE_MASM_AND_X86)
#define MPID_WRITE_BARRIER()
#define MPID_READ_BARRIER() __asm { __asm _emit 0x0f __asm _emit 0xae __asm _emit 0xe8 }
#define MPID_READ_WRITE_BARRIER()

#else
#define MPID_WRITE_BARRIER()
#define MPID_READ_BARRIER()
#define MPID_READ_WRITE_BARRIER()
#endif

#ifndef HAVE_WINDOWS_H
#define USE_SINGLE_MSG_QUEUE
#endif

#define MPIDI_SHM_EAGER_LIMIT          (128*1024)
#ifdef HAVE_SHARED_PROCESS_READ
#define MPIDI_SHM_RNDV_LIMIT           (15*1024)
#endif
#define MPIDI_CH3I_NUM_PACKETS          16
#define MPIDI_CH3I_PACKET_SIZE         (16*1024)
#define MPIDI_CH3I_PKT_EMPTY            0
#define MPIDI_CH3I_PKT_FILLED           1
#ifndef MPIDI_CH3I_SPIN_COUNT_DEFAULT
#define MPIDI_CH3I_SPIN_COUNT_DEFAULT   250
#endif
#define MPIDI_CH3I_YIELD_COUNT_DEFAULT  5000
#define MPIDI_CH3I_MSGQ_ITERATIONS 250

/* For these next two defines, see the ch3_shm.c file in 
   ch3/util/shmbase */
#define MPIDI_CH3_SHM_SHARES_PKTARRAY     1
#define MPIDI_CH3_SHM_SCALABLE_READQUEUES 1

/* This structure uses the avail field to signal that the data is available for reading.
   The code fills the data and then sets the avail field.
   This assumes that declaring avail to be volatile causes the compiler to insert a
   write barrier when the avail location is written to.
   */
typedef struct MPIDI_CH3I_SHM_Packet_t
{
    volatile int avail;
    /*char pad_avail[60];*/ /* keep the avail flag on a separate cache line */
    volatile int num_bytes;
    int offset;
    /* insert stuff here to align data? */
    int pad;/*char pad_data[56];*/ /* cache align the data */
    char data[MPIDI_CH3I_PACKET_SIZE];
    /* insert stuff here to pad the packet up to an aligned boundary? */
} MPIDI_CH3I_SHM_Packet_t;

typedef struct MPIDI_CH3I_SHM_Queue_t
{
    int head_index;
    int tail_index;
    /* insert stuff here to align the packets? */
    /*char pad[56];*/ /* cache align the data */
    MPIDI_CH3I_SHM_Packet_t packet[MPIDI_CH3I_NUM_PACKETS];
} MPIDI_CH3I_SHM_Queue_t;

typedef struct MPIDI_CH3I_Process_s
{
    MPIDI_VC_t *shm_reading_list, *shm_writing_list;
    int num_cpus;
}
MPIDI_CH3I_Process_t;

extern MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

/* for MAXHOSTNAMELEN under Linux ans OSX */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

typedef struct MPIDI_CH3I_BootstrapQ_struct * MPIDI_CH3I_BootstrapQ;

typedef struct MPIDI_Process_group_s
{
    int nShmEagerLimit;
#ifdef HAVE_SHARED_PROCESS_READ
    int nShmRndvLimit;
#endif
    int nShmWaitSpinCount;
    int nShmWaitYieldCount;
    MPIDI_CH3I_BootstrapQ bootstrapQ;
    char shm_hostname[MAXHOSTNAMELEN];
#ifdef MPIDI_CH3_USES_SHM_NAME
    char * shm_name;
#endif
    /*struct MPIDI_Process_group_s *next;*/
} MPIDI_CH3I_PG;
/* MPIDI_CH3I_Process_group_t; */

/* #define MPIDI_CH3_PG_DECL MPIDI_CH3I_Process_group_t ch; */

/* This structure requires the iovec structure macros to be defined */
typedef struct MPIDI_CH3I_SHM_Buffer_t
{
    int use_iov;
    unsigned int num_bytes;
    void *buffer;
    unsigned int bufflen;
#ifdef USE_SHM_IOV_COPY
    MPID_IOV iov[MPID_IOV_LIMIT];
#else
    MPID_IOV *iov;
#endif
    int iovlen;
    int index;
    int total;
} MPIDI_CH3I_SHM_Buffer_t;

typedef struct MPIDI_CH3I_Shmem_block_request_result
{
    int error;
    void *addr;
    unsigned int size;
#ifdef USE_POSIX_SHM
    char key[MPIDI_MAX_SHM_NAME_LENGTH];
    int id;
#elif defined (USE_SYSV_SHM)
    int key;
    int id;
#elif defined (USE_WINDOWS_SHM)
    char key[MPIDI_MAX_SHM_NAME_LENGTH];
    HANDLE id;
#else
#error *** No shared memory mapping variables specified ***
#endif
    char name[MPIDI_MAX_SHM_NAME_LENGTH];
} MPIDI_CH3I_Shmem_block_request_result;

typedef struct MPIDI_CH3I_VC
{
    struct MPIDI_CH3I_SHM_Queue_t * shm, * read_shmq, * write_shmq;
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    struct MPID_Request * req;
    MPIDI_CH3I_VC_state_t state;
    int shm_read_connected;
    struct MPIDU_Sock *sock;
    struct MPIDI_CH3I_Connection * conn;
    int port_name_tag;
    BOOL bShm;
    MPIDI_CH3I_Shmem_block_request_result shm_write_queue_info, shm_read_queue_info;
    int shm_reading_pkt;
    int shm_state;
    MPIDI_CH3I_SHM_Buffer_t read;
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
    HANDLE hSharedProcessHandle;
#else
    int nSharedProcessID;
    int nSharedProcessFileDescriptor;
#endif
#endif
    struct MPIDI_VC *shm_next_reader, *shm_next_writer;
} MPIDI_CH3I_VC;

/* The following define a few different modes of progress, often varying 
   what the process does while waiting for messages to arrive, and how often
   it checks each source of messages (since shared memory is so much faster
   than sockets) */
/*#define USE_FIXED_ACTIVE_PROGRESS*/
/*#define USE_FIXED_SPIN_WAITS*/
#ifndef MPID_CPU_TICK
#define USE_FIXED_SPIN_WAITS
#else
#define USE_ADAPTIVE_PROGRESS
#endif

#ifdef USE_FIXED_ACTIVE_PROGRESS
#define MPIDI_INC_WRITE_ACTIVE() MPIDI_CH3I_shm_write_active++; MPIDI_CH3I_active_flag |= MPID_CH3I_SHM_BIT
#define MPIDI_DEC_WRITE_ACTIVE() MPIDI_CH3I_shm_write_active--
#else
#define MPIDI_INC_WRITE_ACTIVE()
#define MPIDI_DEC_WRITE_ACTIVE()
#endif

/* FIXME: We need to move these queue routines out of the 
   header files to allow dynamic loading/selection of channels,
   and to provide better modularity of the ch3 code */
/*#define USE_VALIDATING_QUEUE_MACROS*/
#ifdef USE_VALIDATING_QUEUE_MACROS

#define MPIDI_CH3I_SendQ_enqueue(vcch, req)				\
{									\
    MPID_Request *iter;							\
    iter = vcch->sendq_head;						\
    while (iter) {							\
	if (iter == req) {						\
	    /*abort();*/ break; /*printf("Error: enqueueing request twice.\n");fflush(stdout);*/\
	}								\
	iter = iter->dev.next;						\
    }									\
    if (!iter) {							\
    /* MT - not thread safe! */						\
    req->dev.next = NULL;						\
    if (vcch->sendq_tail != NULL)					\
    {									\
	vcch->sendq_tail->dev.next = req;				\
    }									\
    else								\
    {									\
	/* increment number of active writes when posting to an empty queue */\
	MPIDI_INC_WRITE_ACTIVE();					\
	vcch->sendq_head = req;					\
    }									\
    vcch->sendq_tail = req;						\
    }									\
}

#define MPIDI_CH3I_SendQ_enqueue_head(vcch, req)				\
{									\
    MPID_Request *iter;							\
    iter = vcch->sendq_head;						\
    while (iter) {							\
	if (iter == req) {						\
	    /*abort();*/ break; /*printf("Error: head enqueueing request twice.\n");fflush(stdout);*/\
	}								\
	iter = iter->dev.next;						\
    }									\
    /* MT - not thread safe! */						\
    if (!iter) {							\
    req->dev.next = vcch->sendq_head; /*sendq_tail;*/			\
    if (vcch->sendq_tail == NULL)					\
    {									\
	/* increment number of active writes when posting to an empty queue */\
	MPIDI_INC_WRITE_ACTIVE();					\
	vcch->sendq_tail = req;					\
    }									\
    vcch->sendq_head = req;						\
    }									\
}

#else

#define MPIDI_CH3I_SendQ_enqueue(vcch, req)				\
{									\
    /* MT - not thread safe! */						\
    req->dev.next = NULL;						\
    if (vcch->sendq_tail != NULL)					\
    {									\
	vcch->sendq_tail->dev.next = req;				\
    }									\
    else								\
    {									\
	/* increment number of active writes when posting to an empty queue */\
	MPIDI_INC_WRITE_ACTIVE();					\
	vcch->sendq_head = req;					\
    }									\
    vcch->sendq_tail = req;						\
}

#define MPIDI_CH3I_SendQ_enqueue_head(vcch, req)				\
{									\
    /* MT - not thread safe! */						\
    req->dev.next = vcch->sendq_head;					\
    if (vcch->sendq_tail == NULL)					\
    {									\
	/* increment number of active writes when posting to an empty queue */\
	MPIDI_INC_WRITE_ACTIVE();					\
	vcch->sendq_tail = req;					\
    }									\
    vcch->sendq_head = req;						\
}

#endif

#define MPIDI_CH3I_SendQ_dequeue(vcch)					\
{									\
    /* MT - not thread safe! */						\
    vcch->sendq_head = vcch->sendq_head->dev.next;			\
    if (vcch->sendq_head == NULL)					\
    {									\
	/* decrement number of active writes when a queue becomes empty */\
	MPIDI_DEC_WRITE_ACTIVE();					\
	vcch->sendq_tail = NULL;					\
    }									\
}

#define MPIDI_CH3I_SendQ_head(vcch) (vcch->sendq_head)

#define MPIDI_CH3I_SendQ_empty(vcch) (vcch->sendq_head == NULL)

#define MPIDU_MAX_SHM_BLOCK_SIZE ((unsigned int)2*1024*1024*1024)

typedef struct MPIDI_CH3I_Shmem_queue_info
{
    int  pg_rank, pid;
    /* FIXME: Why 100? */
    char pg_id[100];
    int type;
    MPIDI_CH3I_Shmem_block_request_result info;
} MPIDI_CH3I_Shmem_queue_info;

int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t *);
int MPIDI_CH3I_Shm_connect(MPIDI_VC_t *, const char *, int *);
int MPIDI_CH3I_SSM_VC_post_read(MPIDI_VC_t *, MPID_Request *);
int MPIDI_CH3I_SSM_VC_post_write(MPIDI_VC_t *, MPID_Request *);

/* FIXME: These should be known only by the code that is managing
   the business cards */
#define MPIDI_CH3I_SHM_HOST_KEY          "shm_host"
#define MPIDI_CH3I_SHM_QUEUE_KEY         "shm_queue"
#define MPIDI_CH3I_SHM_QUEUE_NAME_KEY    "shm_name"
#define MPIDI_CH3I_SHM_PID_KEY           "shm_pid"

#define MPIDI_BOOTSTRAP_NAME_LEN 100
#define BOOTSTRAP_MAX_NUM_MSGS 2048
#define BOOTSTRAP_MAX_MSG_SIZE sizeof(MPIDI_CH3I_Shmem_queue_info)

int MPIDI_CH3I_BootstrapQ_create_unique_name(char *name, int length);
int MPIDI_CH3I_BootstrapQ_create_named(MPIDI_CH3I_BootstrapQ *queue_ptr, const char *name, const int initialize);
int MPIDI_CH3I_BootstrapQ_create(MPIDI_CH3I_BootstrapQ *queue_ptr);
int MPIDI_CH3I_BootstrapQ_tostring(MPIDI_CH3I_BootstrapQ queue, char *name, int length);
int MPIDI_CH3I_BootstrapQ_destroy(MPIDI_CH3I_BootstrapQ queue);
int MPIDI_CH3I_BootstrapQ_unlink(MPIDI_CH3I_BootstrapQ queue);
int MPIDI_CH3I_BootstrapQ_attach(char *name, MPIDI_CH3I_BootstrapQ * queue_ptr);
int MPIDI_CH3I_BootstrapQ_detach(MPIDI_CH3I_BootstrapQ queue);
int MPIDI_CH3I_BootstrapQ_send_msg(MPIDI_CH3I_BootstrapQ queue, void *buffer, int length);
int MPIDI_CH3I_BootstrapQ_recv_msg(MPIDI_CH3I_BootstrapQ queue, void *buffer, int length, int *num_bytes_ptr, BOOL blocking);

typedef enum shm_wait_e
{
SHM_WAIT_TIMEOUT,
SHM_WAIT_READ,
SHM_WAIT_WRITE,
SHM_WAIT_WAKEUP
} shm_wait_t;


int MPIDI_CH3I_SHM_Get_mem(int size, MPIDI_CH3I_Shmem_block_request_result *pOutput);
int MPIDI_CH3I_SHM_Get_mem_named(int size, MPIDI_CH3I_Shmem_block_request_result *pInOutput);
int MPIDI_CH3I_SHM_Attach_to_mem(MPIDI_CH3I_Shmem_block_request_result *pInput, MPIDI_CH3I_Shmem_block_request_result *pOutput);
int MPIDI_CH3I_SHM_Unlink_mem(MPIDI_CH3I_Shmem_block_request_result *p);
int MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Shmem_block_request_result *p);
int MPIDI_CH3I_SHM_read_progress(MPIDI_VC_t *vc, int millisecond_timeout, MPIDI_VC_t **vc_pptr, int *num_bytes_ptr,shm_wait_t *shm_out);
int MPIDI_CH3I_SHM_post_read(MPIDI_VC_t *vc, void *buf, int len, int (*read_progress_update)(int, void*));
int MPIDI_CH3I_SHM_post_readv(MPIDI_VC_t *vc, MPID_IOV *iov, int n, int (*read_progress_update)(int, void*));
int MPIDI_CH3I_SHM_write(MPIDI_VC_t *vc, void *buf, int len, int *num_bytes_ptr);
int MPIDI_CH3I_SHM_writev(MPIDI_VC_t *vc, MPID_IOV *iov, int n, int *num_bytes_ptr);
#ifdef MPIDI_CH3_CHANNEL_RNDV
int MPIDI_CH3I_SHM_rdma_readv(MPIDI_VC_t *vc, MPID_Request *rreq);
int MPIDI_CH3I_SHM_rdma_writev(MPIDI_VC_t *vc, MPID_Request *sreq);
#endif
void MPIDI_CH3I_SHM_Remove_vc_references(MPIDI_VC_t *vc);
void MPIDI_CH3I_SHM_Add_to_reader_list(MPIDI_VC_t *vc);
void MPIDI_CH3I_SHM_Add_to_writer_list(MPIDI_VC_t *vc);

#define MPID_CH3I_SHM_BIT             0x01
#define MPID_CH3I_SOCK_BIT            0x02

extern int MPIDI_CH3I_active_flag;
extern int MPIDI_CH3I_shm_read_active, MPIDI_CH3I_shm_write_active;
extern int MPIDI_CH3I_sock_read_active, MPIDI_CH3I_sock_write_active;

#define MPICH_MSG_QUEUE_NAME    "/mpich_msg_queue"
#define MPICH_MSG_QUEUE_PREFIX  "mpich2q"
#define MPICH_MSG_QUEUE_ID      12345

#ifdef USE_MQSHM
int MPIDI_CH3I_mqshm_create(const char *name, int initialize, int *id);
int MPIDI_CH3I_mqshm_close(int id);
int MPIDI_CH3I_mqshm_unlink(int id);
int MPIDI_CH3I_mqshm_send(const int id, const void *buffer, const int length, const int tag, int *num_sent, int blocking);
int MPIDI_CH3I_mqshm_receive(const int id, const int tag, void *buffer, const int maxlen, int *length, const int blocking);
#endif

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
