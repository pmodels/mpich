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
 * \file include/mpidi_datatypes.h
 * \brief ???
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef __include_mpidi_datatypes_h__
#define __include_mpidi_datatypes_h__

#ifdef MPIDI_STATISTICS
#include <pami_ext_pe.h>
#endif

#include "mpidi_constants.h"
#include "mpidi_platform.h"
#include "pami.h"

#if (MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_THREAD_LOCAL) && defined(__BGQ__)
struct MPID_Request;
typedef struct
{
  struct MPID_Request  * head;
  size_t                 count;
} MPIDI_RequestHandle_t;
#endif


/**
 * \brief MPI Process descriptor
 *
 * This structure contains global configuration flags.
 */
typedef struct
{
  unsigned avail_contexts;
  unsigned short_limit;
  unsigned eager_limit;
  unsigned eager_limit_local;
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
  unsigned mp_infolevel;
  unsigned mp_statistics;     /* print pamid statistcs data                           */
  unsigned mp_printenv; ;     /* print env data                                       */
#endif
#ifdef RDMA_FAILOVER
  unsigned mp_s_use_pami_get; /* force the PAMI_Get path instead of PAMI_Rget         */
#endif

#if (MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_THREAD_LOCAL) && defined(__BGQ__)
  MPIDI_RequestHandle_t request_handles[MPIDI_MAX_THREADS];
#endif

  unsigned verbose;        /**< The current level of verbosity for end-of-job stats. */
  unsigned statistics;     /**< The current level of stats collection.               */
  unsigned rma_pending;    /**< The max num outstanding requests during an RMA op    */
  unsigned shmem_pt2pt;    /**< Enable optimized shared memory point-to-point functions. */

  pami_geometry_t world_geometry;

  struct
  {
    unsigned collectives;  /**< Enable optimized collective functions. */
    unsigned subcomms;
    unsigned select_colls; /**< Enable collective selection */
  }
  optimized;

  struct
  {
    volatile unsigned active;  /**< Number of contexts with active async progress */
    unsigned          mode;    /**< 0 == 'disabled', 1 == 'locked', 2 == 'trigger' */
  }
  async_progress;

  struct
  {
    struct
    {
      unsigned requested;    /**< 1 == application requests context post */
      unsigned active;       /**< 1 == context post is currently required */
    } context_post;
  } perobj;                  /**< This structure is only used in the 'perobj' mpich lock mode. */

} MPIDI_Process_t;


enum
  {
    MPIDI_Protocols_Short,
    MPIDI_Protocols_ShortSync,
    MPIDI_Protocols_Eager,
    MPIDI_Protocols_RVZ,
    MPIDI_Protocols_Cancel,
    MPIDI_Protocols_Control,
    MPIDI_Protocols_WinCtrl,
    MPIDI_Protocols_WinAccum,
    MPIDI_Protocols_RVZ_zerobyte,
    MPIDI_Protocols_COUNT,
  };


/**
 * \brief This defines the type of message being sent/received
 * mpid_startall() invokes the correct start based on the type of the request
 */
typedef enum
  {
    MPIDI_REQUEST_PTYPE_RECV,
    MPIDI_REQUEST_PTYPE_SEND,
    MPIDI_REQUEST_PTYPE_BSEND,
    MPIDI_REQUEST_PTYPE_SSEND,
  } MPIDI_REQUEST_PTYPE;


typedef enum
  {
    MPIDI_CONTROL_SSEND_ACKNOWLEDGE,
    MPIDI_CONTROL_CANCEL_REQUEST,
    MPIDI_CONTROL_CANCEL_ACKNOWLEDGE,
    MPIDI_CONTROL_CANCEL_NOT_ACKNOWLEDGE,
    MPIDI_CONTROL_RENDEZVOUS_ACKNOWLEDGE,
  } MPIDI_CONTROL;


/** \brief Request completion actions */
typedef enum
  {
    MPIDI_CA_COMPLETE,
    MPIDI_CA_UNPACK_UEBUF_AND_COMPLETE,         /**< Unpack uebuf, then complete. */
  } MPIDI_CA;


/**
 * \brief MPIDI_Message_match contains enough information to match an
 * MPI message.
 */
typedef struct
{
  int tag;        /**< match tag     */
  int rank;       /**< match rank    */
  int context_id; /**< match context */
#ifdef OUT_OF_ORDER_HANDLING
  int seqno;      /**< match seqno */
#endif
} MPIDI_Message_match;


/**
 * \brief MPID pt2pt message header
 */
typedef struct
{
  MPI_Request req;         /**< peer's request handle  */
  unsigned    MPItag;      /**< match tag              */
  unsigned    MPIrank;     /**< match rank             */
  uint16_t    MPIctxt;     /**< match context          */

  union {
    uint16_t  flags;
    struct {
      unsigned control:3;  /**< message type for control protocols */
      unsigned isSync:1;   /**< set for sync sends     */
      unsigned isRzv :1;   /**< use pt2pt rendezvous   */
    } __attribute__ ((__packed__));
  };

#ifdef OUT_OF_ORDER_HANDLING
  unsigned    MPIseqno;    /**< match seqno            */
#endif
} MPIDI_MsgInfo;

/** \brief Full Rendezvous msg info to be set as two quads of unexpected data. */
typedef struct
{
  MPIDI_MsgInfo    msginfo;
  pami_memregion_t memregion;
#ifdef RDMA_FAILOVER
  uint32_t         memregion_used;
#endif
  void           * data;
  size_t           length;
} MPIDI_MsgEnvelope;

/** \brief This defines the portion of MPID_Request that is specific to the Device */
struct MPIDI_Request
{
  struct MPID_Request  *next;         /**< Link to next req. in queue */
  struct MPID_Datatype *datatype_ptr; /**< Info about the datatype    */
  pami_work_t           post_request; /**<                            */

  MPIDI_MsgEnvelope     envelope;

  void                 *userbuf;      /**< User buffer                */
  unsigned              userbufcount; /**< Userbuf data count         */
  MPI_Datatype          datatype;     /**< Data type of message       */
  pami_task_t           peer_pami;    /**< The other guy's rank (in PAMI) */
  unsigned              peer_comm;    /**< The other guy's rank (in the orig communicator) */
  unsigned            cancel_pending:16; /**< Cancel status              */
  unsigned            uebuf_malloc:16;   /**< does uebuf require free()  */

  unsigned              uebuflen;     /**< Length (bytes) of uebuf    */
  void                 *uebuf;        /**< Unexpected buffer          */

  MPIDI_REQUEST_PTYPE   ptype;        /**< The persistent msg type    */
  MPIDI_CA              ca;           /**< Completion action          */
  pami_memregion_t      memregion;    /**< Rendezvous recv memregion  */
#ifdef OUT_OF_ORDER_HANDLING
  struct MPID_Request  *prev;         /**< Link to prev req. in queue */
  void                 *nextR;        /** < pointer to next recv for the out-of-order list, the out-of-order list is a list per source */
  void                 *prevR;        /** < pointer to prev recv for the out-of-order list, the out-of-order list is a list per source */
  struct MPID_Request  *oo_peer;      /** < pointer to the matched post recv request to complete in the out-of-order case */
#endif
#ifdef RDMA_FAILOVER
  uint32_t             memregion_used:16;
  uint32_t             shm:16;
#endif
#ifdef MPIDI_TRACE
  int   cur_nMsgs;
  int   partner_id;
  int   idx;
  int   PR_idx;
#endif
};


/** \brief This defines the portion of MPID_Comm that is specific to the Device */
struct MPIDI_Comm
{
  pami_geometry_t geometry; /**< Geometry component for collectives      */
  pami_geometry_t parent; /**< The parent geometry this communicator came from */
  pami_algorithm_t *coll_algorithm[PAMI_XFER_COUNT][2];
  pami_metadata_t *coll_metadata[PAMI_XFER_COUNT][2];
  char coll_count[PAMI_XFER_COUNT][2];
  pami_algorithm_t user_selected[PAMI_XFER_COUNT];
  /* no way to tell if user_selected[] is NULL */
  /* could probably union these two though? */
  char user_selected_type[PAMI_XFER_COUNT];
  pami_metadata_t user_metadata[PAMI_XFER_COUNT];
  char last_algorithm[100];
  char preallreduces[MPID_NUM_PREALLREDUCES];
  /* \todo Need to figure out how to deal with algorithms above the pami level */
  char allgathers[4];
  char allgathervs[4];
  char scattervs[2];
  char optgather, optscatter;

  /* These need to be freed at geom destroy, so we need to store them
   * inside the communicator struct until destroy time rather than
   * allocating pointers on the stack
   */
  /* For create_taskrange */
  pami_geometry_range_t *ranges;
  /* For create_tasklist/endpoints if we ever use it */
  pami_task_t *tasks;
  pami_endpoint_t *endpoints;
   /* There are some protocols where the optimized protocol always works and
    * is the best performance */
   /* Assume we have small vs large cutoffs vs medium for some protocols */
   pami_algorithm_t opt_protocol[PAMI_XFER_COUNT][2];
   int must_query[PAMI_XFER_COUNT][2];
   pami_metadata_t opt_protocol_md[PAMI_XFER_COUNT][2];
   int cutoff_size[PAMI_XFER_COUNT][2];
   /* Our best allreduce double protocol only works on 
    * doubles and sum/min/max. Since that is a common
    * occurance let's cache that protocol and call
    * it without checking */
   pami_algorithm_t cached_allred_dsmm; /*dsmm = double, sum/min/max */
   pami_metadata_t cached_allred_dsmm_md;
   int query_allred_dsmm; 

   /* We have some integer optimized protocols that only work on
    * sum/min/max but also have datasize/ppn <= 8k limitations */
   /* Using Amith's protocol, these work on int/min/max/sum of SMALL messages */
   pami_algorithm_t cached_allred_ismm;
   pami_metadata_t cached_allred_ismm_md;
   /* Because this only works at select message sizes, this will have to be
    * nonzero */
   int query_allred_ismm;

  union tasks_descrip_t {
    /* For create_taskrange */
    pami_geometry_range_t *ranges;
    /* For create_tasklist/endpoints if we ever use it */
    pami_task_t *tasks;
    pami_endpoint_t *endpoints;
  } tasks_descriptor;
};


typedef struct
{
  pami_work_t state;
  pami_xfer_t *coll_struct;
} MPIDI_Post_coll_t;


/** \brief Forward declaration of the MPID_Comm structure */
struct MPID_Comm;
/** \brief Forward declaration of the MPID_Win structure */
struct MPID_Win;
/** \brief Forward declaration of the MPID_Group structure */
struct MPID_Group;


struct MPIDI_Win_lock
{
  struct MPIDI_Win_lock *next;
  unsigned               rank;
  int                    type;
};
struct MPIDI_Win_queue
{
  struct MPIDI_Win_lock *head;
  struct MPIDI_Win_lock *tail;
};
/**
 * \brief Collective information related to a window
 *
 * This structure is used to share information about a local window with
 * all nodes in the window communicator. Part of that information includes
 * statistics about RMA operations during access/exposure epochs.
 *
 * The structure is allocated as an array sized for the window communicator.
 * Each entry in the array corresponds directly to the node of the same rank.
 */
struct MPIDI_Win_info
{
  void             * base_addr;     /**< Node's exposure window base address                  */
  struct MPID_Win  * win;
  uint32_t           disp_unit;     /**< Node's exposure window displacement units            */
  pami_memregion_t   memregion;     /**< Memory region descriptor for each node               */
#ifdef RDMA_FAILOVER
  uint32_t           memregion_used;
#endif
};
/**
 * \brief Structure of PAMI extensions to MPID_Win structure
 */
struct MPIDI_Win
{
  struct MPIDI_Win_info * info;    /**< allocated array of collective info             */
  struct MPIDI_Win_sync
  {
#if 0
    /** \todo optimize some of the synchronization assertion */
    uint32_t assert; /**< MPI_MODE_* bits asserted at epoch start              */
#endif

    /* These fields are reset by the sync functions */
    uint32_t          total;    /**< The number of PAMI requests that we know about (updated only by calling thread) */
    volatile uint32_t started;  /**< The number of PAMI requests made (updated only in the context_post callback) */
    volatile uint32_t complete; /**< The number of completed PAMI requests (only updated by the done callbacks) */

    struct MPIDI_Win_sync_pscw
    {
      struct MPID_Group * group;
      volatile unsigned   count;
    } sc, pw;
    struct MPIDI_Win_sync_lock
    {
      struct
      {
        volatile unsigned locked;
      } remote;
      struct
      {
        struct MPIDI_Win_queue requested;
        int                    type;
        unsigned               count;
      } local;
    } lock;
  } sync;
};


#endif
