/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/onesided/mpidi_onesided.h
 * \brief ???
 */
#include <mpidimpl.h>
#include "opa_primitives.h"

#ifndef __src_onesided_mpidi_onesided_h__
#define __src_onesided_mpidi_onesided_h__

pami_rget_simple_t zero_rget_parms;
pami_get_simple_t zero_get_parms;
pami_rput_simple_t zero_rput_parms;
pami_put_simple_t zero_put_parms;
pami_rput_typed_t zero_rput_typed_parms;
pami_rget_typed_t zero_rget_typed_parms;
pami_send_t   zero_send_parms;
pami_send_immediate_t   zero_send_immediate_parms;
pami_recv_t   zero_recv_parms;
pami_rmw_t   zero_rmw_parms;

#define MPIDI_QUICKSLEEP     usleep(1);
#define MAX_NUM_CTRLSEND  1024          /* no more than 1024 outstanding control sends */

#define MPIDI_SHM_MUTEX_LOCK(win) {                                                        \
    int mpi_errno=0;                                                                       \
    do {                                                                                   \
        pthread_mutex_t *shm_mutex = (pthread_mutex_t *) &win->mpid.shm->ctrl->mutex_lock; \
        int rval = pthread_mutex_lock(shm_mutex);                                          \
        MPIU_ERR_CHKANDJUMP1(rval, mpi_errno, MPI_ERR_OTHER, "**pthread_lock",             \
                             "**pthread_lock %s", strerror(rval));                         \
    } while (0); }


#define MPIDI_SHM_MUTEX_UNLOCK(win) {                                                      \
    int mpi_errno=0;                                                                       \
    do {                                                                                   \
        pthread_mutex_t *shm_mutex = (pthread_mutex_t *) &win->mpid.shm->ctrl->mutex_lock; \
        int rval = pthread_mutex_unlock(shm_mutex);                                        \
        MPIU_ERR_CHKANDJUMP1(rval, mpi_errno, MPI_ERR_OTHER, "**pthread_unlock",           \
                             "**pthread_unlock %s", strerror(rval));                       \
    } while (0); }


#define MPIDI_SHM_MUTEX_INIT(win)                                                          \
    do {                                                                                   \
        int rval=0;                                                                        \
        int mpi_errno=0;                                                                   \
        pthread_mutexattr_t attr;                                                          \
        pthread_mutex_t *shm_mutex = (pthread_mutex_t *) &win->mpid.shm->ctrl->mutex_lock; \
                                                                                           \
        rval = pthread_mutexattr_init(&attr);                                              \
        MPIU_ERR_CHKANDJUMP1(rval, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",            \
                             "**pthread_mutex %s", strerror(rval));                        \
        rval = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);                \
        MPIU_ERR_CHKANDJUMP1(rval, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",            \
                             "**pthread_mutex %s", strerror(rval));                        \
        rval = pthread_mutex_init(shm_mutex, &attr);                                       \
        MPIU_ERR_CHKANDJUMP1(rval, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",            \
                             "**pthread_mutex %s", strerror(rval));                        \
        rval = pthread_mutexattr_destroy(&attr);                                           \
        MPIU_ERR_CHKANDJUMP1(rval, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",            \
                             "**pthread_mutex %s", strerror(rval));                        \
    } while (0);

#define MPIDI_SHM_MUTEX_DESTROY(win) {                                                      \
    int mpi_errno=0;                                                                        \
    do {                                                                                    \
        pthread_mutex_t *shm_mutex = (pthread_mutex_t *) &(win)->mpid.shm->ctrl->mutex_lock;\
        int rval = pthread_mutex_destroy(shm_mutex);                                        \
        MPIU_ERR_CHKANDJUMP1(rval, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",             \
                             "**pthread_mutex %s", strerror(rval));                         \
    } while (0); }

#define MPIDI_ACQUIRE_SHARED_LOCK(win) {                                                    \
         MPIDI_SHM_MUTEX_LOCK(win);                                                         \
         OPA_fetch_and_add_int((OPA_int_t *)&win->mpid.shm->ctrl->active,(int) 1);          \
}

#define MPIDI_RELEASE_SHARED_LOCK(win) {                                                    \
         MPIDI_SHM_MUTEX_UNLOCK(win);                                                       \
         OPA_fetch_and_add_int((OPA_int_t *)&(win->mpid.shm->ctrl->active),(int) -1);       \
}




/**
 * \brief One-sided Message Types
 */
typedef enum
  {
    MPIDI_WIN_MSGTYPE_LOCKACK,     /**< Lock acknowledge      */
    MPIDI_WIN_MSGTYPE_LOCKALLACK,  /**< Lock all acknowledge  */
    MPIDI_WIN_MSGTYPE_LOCKREQ,     /**< Lock window           */
    MPIDI_WIN_MSGTYPE_LOCKALLREQ,  /**< Lock all window       */
    MPIDI_WIN_MSGTYPE_UNLOCK,      /**< Unlock window         */
    MPIDI_WIN_MSGTYPE_UNLOCKALL,   /**< Unlock window         */

    MPIDI_WIN_MSGTYPE_COMPLETE,    /**< End a START epoch     */
    MPIDI_WIN_MSGTYPE_POST,        /**< Begin POST epoch      */
  } MPIDI_Win_msgtype_t;

typedef enum
  {
    MPIDI_WIN_REQUEST_ACCUMULATE,
    MPIDI_WIN_REQUEST_GET,
    MPIDI_WIN_REQUEST_PUT,
    MPIDI_WIN_REQUEST_GET_ACCUMULATE,
    MPIDI_WIN_REQUEST_RACCUMULATE,
    MPIDI_WIN_REQUEST_RGET,
    MPIDI_WIN_REQUEST_RPUT,
    MPIDI_WIN_REQUEST_RGET_ACCUMULATE,
    MPIDI_WIN_REQUEST_COMPARE_AND_SWAP,
    MPIDI_WIN_REQUEST_FETCH_AND_OP,
  } MPIDI_Win_requesttype_t;

typedef enum
  {
    MPIDI_ACCU_OPS_SAME_OP,
    MPIDI_ACCU_OPS_SAME_OP_NO_OP
  } MPIDI_Win_info_arg_vals_accumulate_ops;


typedef struct
{
  MPIDI_Win_msgtype_t type;
  MPID_Win *win;
  int      rank;          /* MPI rank */
  void     *flagAddr;
  union
  {
    struct
    {
      int type;
    } lock;
  } data;
} MPIDI_Win_control_t;


#define MAX_ATOMIC_TYPE_SIZE 32
typedef struct
{
  char    buf[MAX_ATOMIC_TYPE_SIZE];   //Origin value or ack result value
  char    test[MAX_ATOMIC_TYPE_SIZE];  //Test element for CAS
  void  * result_addr;                 //Address on source to store output
  void  * remote_addr;                 //Address of target on destination
  void  * request_addr;                //Address of the request object
  MPI_Datatype  datatype;
  MPI_Op        op;
  int           atomic_type;
} MPIDI_AtomicHeader_t;


typedef struct MPIDI_WinLock_info
{
  unsigned            peer;
  int                 lock_type;
  struct MPID_Win   * win;
  void                *flagAddr;
  volatile unsigned   done;
  pami_work_t         work;
} MPIDI_WinLock_info;



typedef struct
{
  MPID_Datatype * pointer;
  MPI_Datatype    type;
  int             count;
  int             contig;
  MPI_Aint        true_lb;
  MPIDI_msg_sz_t  size;

  int             num_contig;
  DLOOP_VECTOR  * map;
  DLOOP_VECTOR    __map;
} MPIDI_Datatype;


typedef struct
{
  void         * addr;
  void         * result_addr;  /* anchor the result buffer address*/
  void         * req;
  MPID_Win     * win;
  MPI_Datatype   type;
  MPI_Op         op;
  pami_endpoint_t origin_endpoint;    /* needed for returning the result */
                                      /* for MPI_Get_accumulate          */
  size_t          len;                /* length of the send data         */
} MPIDI_Win_MsgInfo;

typedef struct
{
  void         * addr;
  void         * req;
  MPID_Win     * win;
  MPI_Datatype   type;
  MPI_Op         op;
  int            count;
  int            counter;
  int            num_contig;
  int            size;
  void         * request;
  void         * tptr;
  pami_endpoint_t src_endpoint;    
} MPIDI_Win_GetAccMsgInfo;


/** \todo make sure that the extra fields are removed */
typedef struct MPIDI_Win_request
{
  MPID_Win               *win;
  MPIDI_Win_requesttype_t type;
  pami_endpoint_t         dest;
  size_t                  offset;
  pami_work_t             post_request;

  struct
  {
    unsigned index;
    size_t   local_offset;
  } state;

  void      * accum_headers;

  struct
  {
    pami_memregion_t memregion;
#ifdef RDMA_FAILOVER
    uint32_t         memregion_used;    /**< memory region registered or not */
#endif
    void            *addr;
    void            *result_addr;  /** result buffer that holds target buffer data */
    int              count;
    MPI_Datatype     datatype;
    MPIDI_Datatype   dt;
    int              completed;
  } origin;

  struct
  {
    pami_task_t      rank;              /**< Comm-local rank */
    MPIDI_Datatype   dt;
  } target;

  struct
  {
    void            *addr;
    int              count;
    MPI_Datatype     datatype;
    MPIDI_Datatype   dt;
  } result;

  void     *user_buffer;
  void     *compare_buffer;    /* anchor of compare buffer for compare and swap */
  uint32_t  buffer_free;
  void     *buffer;
  struct MPIDI_Win_request *next; 
  MPI_Op     op;
  int        result_num_contig;   


  /* for RMA atomic functions */
  
  pami_atomic_t      pami_op;        
  pami_type_t        pami_datatype;  
 
  int request_based;            /* flag for request based rma */
  MPID_Request *req_handle;     /* anchor of MPID_Request struc for request based rma*/
} MPIDI_Win_request;

MPIDI_Win_request  zero_req;    /* used for init. request structure to 0 */

void
MPIDI_Win_datatype_basic(int              count,
                         MPI_Datatype     datatype,
                         MPIDI_Datatype * dt);
void
MPIDI_Win_datatype_map  (MPIDI_Datatype * dt);
void
MPIDI_Win_datatype_unmap(MPIDI_Datatype * dt);

void
MPIDI_WinCtrlSend(pami_context_t       context,
                  MPIDI_Win_control_t *control,
                  int                  rank,
                  MPID_Win            *win);

void
MPIDI_WinLockReq_proc(pami_context_t              context,
                      const MPIDI_Win_control_t * info,
                      unsigned                    peer);
void
MPIDI_WinLockAck_proc(pami_context_t              context,
                      const MPIDI_Win_control_t * info,
                      unsigned                    peer);
void
MPIDI_WinUnlock_proc(pami_context_t              context,
                     const MPIDI_Win_control_t * info,
                     unsigned                    peer);

void
MPIDI_WinComplete_proc(pami_context_t              context,
                       const MPIDI_Win_control_t * info,
                       unsigned                    peer);
void
MPIDI_WinPost_proc(pami_context_t              context,
                   const MPIDI_Win_control_t * info,
                   unsigned                    peer);

void
MPIDI_Win_DoneCB(pami_context_t  context,
                 void          * cookie,
                 pami_result_t   result);
void
MPIDI_WinUnlockDoneCB(pami_context_t  context,
                 void          * cookie,
                 pami_result_t   result);

void
MPIDI_WinAccumCB(pami_context_t    context,
                 void            * cookie,
                 const void      * _msginfo,
                 size_t            msginfo_size,
                 const void      * sndbuf,
                 size_t            sndlen,
                 pami_endpoint_t   sender,
                 pami_recv_t     * recv);
int
MPIDI_Win_init( MPI_Aint length,
                int disp_unit,
                MPID_Win  **win_ptr,
                MPID_Info  *info,
                MPID_Comm *comm_ptr,
                int create_flavor,
                int model);
int
MPIDI_Win_allgather(MPI_Aint size,
                    MPID_Win **win_ptr);

void
MPIDI_WinLockAdvance(pami_context_t   context,
                     MPID_Win         * win);
void
MPIDI_WinLockAck_post(pami_context_t   context,
                      unsigned         peer,
                      MPID_Win         *win);

void
MPIDI_WinLockReq_proc(pami_context_t              context,
                      const MPIDI_Win_control_t * info,
                      unsigned                    peer);

int
MPIDI_Datatype_is_pami_rmw_supported(MPI_Datatype datatype,
	                                 pami_type_t *pami_type,
					 MPI_Op op,
					 pami_atomic_t *pami_op);

int
MPIDI_valid_group_rank(int lpid,
                       MPID_Group *grp);
void
MPIDI_WinLockAllAck_post(pami_context_t   context,
                         unsigned         peer,
                         MPID_Win       * win);


#endif
