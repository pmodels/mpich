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


/**
 * \brief One-sided Message Types
 */
typedef enum
  {
    MPIDI_WIN_MSGTYPE_LOCKACK,  /**< Lock acknowledge  */
    MPIDI_WIN_MSGTYPE_LOCKREQ,  /**< Lock window       */
    MPIDI_WIN_MSGTYPE_UNLOCK,   /**< Unlock window     */

    MPIDI_WIN_MSGTYPE_COMPLETE, /**< End a START epoch */
    MPIDI_WIN_MSGTYPE_POST,     /**< Begin POST epoch  */
  } MPIDI_Win_msgtype_t;

typedef enum
  {
    MPIDI_WIN_REQUEST_ACCUMULATE,
    MPIDI_WIN_REQUEST_GET,
    MPIDI_WIN_REQUEST_PUT,
  } MPIDI_Win_requesttype_t;


typedef struct
{
  MPIDI_Win_msgtype_t type;
  MPID_Win *win;
  union
  {
    struct
    {
      int type;
    } lock;
  } data;
} MPIDI_Win_control_t;


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
  void         * req;
  MPID_Win     * win;
  MPI_Datatype   type;
  MPI_Op         op;
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
  uint32_t  buffer_free;
} MPIDI_Win_request;



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
                  pami_task_t          peer,
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
MPIDI_WinAccumCB(pami_context_t    context,
                 void            * cookie,
                 const void      * _msginfo,
                 size_t            msginfo_size,
                 const void      * sndbuf,
                 size_t            sndlen,
                 pami_endpoint_t   sender,
                 pami_recv_t     * recv);

#endif
