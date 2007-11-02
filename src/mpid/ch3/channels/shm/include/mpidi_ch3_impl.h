/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidi_ch3i_shm_conf.h"
#include "mpidimpl.h"
#include "mpidu_process_locks.h"

/* Redefine MPIU_CALL since the shm channel should be self-contained.
   This only affects the building of a dynamically loadable library for 
   the sock channel, and then only when debugging is enabled */
#undef MPIU_CALL
#define MPIU_CALL(context,funccall) context##_##funccall

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <assert.h>

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
#ifdef HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
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

#elif defined(HAVE___ASM_AND_PENTIUM_ASM)
#ifdef HAVE___ASM_AND_X86_SFENCE
#define MPID_WRITE_BARRIER() __asm sfence
#else
#define MPID_WRITE_BARRIER()
#endif
#ifdef HAVE___ASM_AND_X86_LFENCE
#define MPID_READ_BARRIER() __asm __emit 0x0f __asm __emit 0xae __asm __emit 0xe8
#else
#define MPID_READ_BARRIER()
#endif
#ifdef HAVE___ASM_AND_X86_MFENCE
#define MPID_READ_WRITE_BARRIER() __asm __emit 0x0f __asm __emit 0xae __asm __emit 0xf0
#else
#define MPID_READ_WRITE_BARRIER()
#endif

#elif defined(HAVE_GCC_ASM_SPARC_MEMBAR)
#define MPID_WRITE_BARRIER() __asm__ __volatile__ ( "membar #StoreLoad | #StoreStore" : : : "memory" );
#define MPID_READ_BARRIER() __asm__ __volatile__ ( "membar #LoadLoad | #LoadStore" : : : "memory" );
#define MPID_READ_WRITE_BARRIER() __asm__ __volatile__ ( "membar #StoreLoad | #StoreStore | #LoadLoad | #LoadStore" : : : "memory" );

#elif defined(HAVE_GCC_ASM_SPARC_STBAR)
#define MPID_WRITE_BARRIER() __asm__ __volatile__ ( "stbar" : : : "memory" );
#define MPID_READ_BARRIER()  __asm__ __volatile__ ( "stbar" : : : "memory" );
#define MPID_READ_WRITE_BARRIER() __asm__ __volatile__ ( "stbar" : : : "memory" );

#elif defined(HAVE_SOLARIS_ASM_SPARC_MEMBAR)
#define MPID_WRITE_BARRIER() __asm ( "membar #StoreLoad | #StoreStore");
#define MPID_READ_BARRIER()  __asm ( "membar #LoadLoad | #LoadStore");
#define MPID_READ_WRITE_BARRIER() __asm ( "membar #StoreLoad | #StoreStore | #LoadLoad | #LoadStore");

#elif defined(HAVE_SOLARIS_ASM_SPARC_STBAR)
#define MPID_WRITE_BARRIER() __asm ( "stbar" );
#define MPID_READ_BARRIER() __asm ( "stbar" );
#define MPID_READ_WRITE_BARRIER() __asm ( "stbar" );

#else
/* FIXME: We need to ensure read/write barriers for correctness, if the 
   processor might possibly reorder */
#define MPID_WRITE_BARRIER()
#define MPID_READ_BARRIER()
#define MPID_READ_WRITE_BARRIER()
#endif

#define MPIDI_CH3I_NUM_PACKETS          16
#define MPIDI_CH3I_PACKET_SIZE         (16*1024)
#define MPIDI_CH3I_PKT_AVAILABLE        0
#define MPIDI_CH3I_PKT_USED             1
#define MPIDI_CH3I_SPIN_COUNT_DEFAULT   100
#define MPIDI_CH3I_YIELD_COUNT_DEFAULT  5000
#define MPIDI_SHM_EAGER_LIMIT           10240
#ifdef HAVE_SHARED_PROCESS_READ
#define MPIDI_SHM_RNDV_LIMIT            10240
#endif
#define MPID_SHMEM_PER_PROCESS          1048576

/* FIXME: The ssm channel uses these names instead.  We'll switch to them 
   eventually (since there is a great deal of commonality between the ssm and
   shm code, and that commonality should be reflected in common support 
   routines) */
#define MPIDI_CH3I_PKT_EMPTY            MPIDI_CH3I_PKT_AVAILABLE
#define MPIDI_CH3I_PKT_FILLED           MPIDI_CH3I_PKT_USED


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

typedef struct MPIDI_CH3I_SHM_Unex_read_s
{
    struct MPIDI_CH3I_SHM_Packet_t *pkt_ptr;
    unsigned char *buf;
    unsigned int length;
    int src;
    struct MPIDI_CH3I_SHM_Unex_read_s *next;
} MPIDI_CH3I_SHM_Unex_read_t;

typedef struct MPIDI_CH3I_VC
{
    struct MPIDI_CH3I_SHM_Queue_t * shm, * read_shmq, * write_shmq;
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    struct MPID_Request * req;
    int shm_reading_pkt;
    int shm_state;
    MPIDI_CH3I_SHM_Buffer_t read;
#ifdef USE_SHM_UNEX
    MPIDI_CH3I_SHM_Unex_read_t *unex_list;
    struct MPIDI_VC *unex_finished_next;
#endif
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
    HANDLE hSharedProcessHandle;
#else
    int nSharedProcessID;
    int nSharedProcessFileDescriptor;
#endif
#endif
} MPIDI_CH3I_VC;

/* This structure uses the avail field to signal that the data is available 
   for reading.
   The code fills the data and then sets the avail field.
   This assumes that declaring avail to be volatile causes the compiler to 
   insert a
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
    MPIDI_VC_t *unex_finished_list, *vc;
}
MPIDI_CH3I_Process_t;

extern MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

#ifdef HAVE_SHARED_PROCESS_READ
typedef struct MPIDI_CH3I_Shared_process
{
    int nRank;
#ifdef HAVE_WINDOWS_H
    DWORD nPid;
#else
    int nPid;
#endif
    BOOL bFinished;
} MPIDI_CH3I_Shared_process_t;
#endif

typedef struct MPIDI_Process_group_s
{
    volatile int ref_count;
    int nShmEagerLimit;
#ifdef HAVE_SHARED_PROCESS_READ
    int nShmRndvLimit;
    MPIDI_CH3I_Shared_process_t *pSHP;
#ifdef HAVE_WINDOWS_H
    HANDLE *pSharedProcessHandles;
#else
    int *pSharedProcessIDs;
    int *pSharedProcessFileDescriptors;
#endif
#endif
    void *addr;
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
    int nShmWaitSpinCount;
    int nShmWaitYieldCount;
} MPIDI_CH3I_PG;
/* MPIDI_CH3I_Process_group_t; */

/* #define MPIDI_CH3_PG_DECL MPIDI_CH3I_Process_group_t ch; */


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
	vcch->sendq_head = req;					\
    }									\
    vcch->sendq_tail = req;						\
}

#define MPIDI_CH3I_SendQ_enqueue_head(vcch, req)			     \
{									     \
    /* MT - not thread safe! */						     \
    req->dev.next = vcch->sendq_head;					     \
    if (vcch->sendq_tail == NULL)					     \
    {									     \
	vcch->sendq_tail = req;					     \
    }									     \
    vcch->sendq_head = req;						     \
}

#define MPIDI_CH3I_SendQ_dequeue(vcch)					\
{									\
    /* MT - not thread safe! */						\
    vcch->sendq_head = vcch->sendq_head->dev.next;			\
    if (vcch->sendq_head == NULL)					\
    {									\
	vcch->sendq_tail = NULL;					\
    }									\
}

#define MPIDI_CH3I_SendQ_head(vcch) (vcch->sendq_head)

#define MPIDI_CH3I_SendQ_empty(vcch) (vcch->sendq_head == NULL)

typedef enum shm_wait_e
{
SHM_WAIT_TIMEOUT,
SHM_WAIT_READ,
SHM_WAIT_WRITE,
SHM_WAIT_WAKEUP
} shm_wait_t;

int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_Request_adjust_iov(MPID_Request *, MPIDI_msg_sz_t);

int MPIDI_CH3I_SHM_Get_mem(MPIDI_PG_t *pg, int nTotalSize, int nRank, int nNproc, BOOL bUseShm);
int MPIDI_CH3I_SHM_Release_mem(MPIDI_PG_t *pg, BOOL bUseShm);

int MPIDI_CH3I_SHM_read_progress(MPIDI_VC_t *vc, int millisecond_timeout, MPIDI_VC_t **vc_pptr, int *num_bytes_ptr, shm_wait_t *shm_out);
int MPIDI_CH3I_SHM_post_read(MPIDI_VC_t *vc, void *buf, int len, int (*read_progress_update)(int, void*));
int MPIDI_CH3I_SHM_post_readv(MPIDI_VC_t *vc, MPID_IOV *iov, int n, int (*read_progress_update)(int, void*));
int MPIDI_CH3I_SHM_write(MPIDI_VC_t *vc, void *buf, int len, int *num_bytes_ptr);
int MPIDI_CH3I_SHM_writev(MPIDI_VC_t *vc, MPID_IOV *iov, int n, int *num_bytes_ptr);
int MPIDI_CH3I_SHM_rdma_readv(MPIDI_VC_t *vc, MPID_Request *rreq);
int MPIDI_CH3I_SHM_rdma_writev(MPIDI_VC_t *vc, MPID_Request *sreq);

int MPIDI_CH3I_SHM_Progress_init(void);

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
