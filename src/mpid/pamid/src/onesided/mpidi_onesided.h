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

#ifndef __src_onesided_mpidi_onesided_h__
#define __src_onesided_mpidi_onesided_h__

pami_rget_simple_t zero_rget_parms;
pami_get_simple_t zero_get_parms;
pami_rput_simple_t zero_rput_parms;
pami_put_simple_t zero_put_parms;
pami_send_t   zero_send_parms;
pami_recv_t   zero_recv_parms;

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
  union
  {
    struct
    {
      int type;
    } lock;
  } data;
} MPIDI_Win_control_t;


typedef struct MPIDI_WinLock_info
{
  unsigned            peer;
  int                 lock_type;
  struct MPID_Win   * win;
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


/** \todo make sure that the extra fields are removed */
typedef struct
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

  MPIDI_Win_MsgInfo * accum_headers;

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

  void     *buffer;
  void     *user_buffer;
  uint32_t  buffer_free;
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
MPIDI_Win_allgather(void *base,
                    MPI_Aint size,
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
#endif
