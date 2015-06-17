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
#include "mpidi_trace.h"

#include "opa_primitives.h"



#if (MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_THREAD_LOCAL) && defined(__BGQ__)
struct MPID_Request;
typedef struct
{
  struct MPID_Request  * head;
  size_t                 count;
} MPIDI_RequestHandle_t;
#endif

#define MPIDI_PT2PT_LIMIT_SET(is_internal,is_immediate,is_local,value)		\
  MPIDI_Process.pt2pt.limits_lookup[is_internal][is_immediate][is_local] = value\

typedef struct
{
  unsigned remote;
  unsigned local;
} MPIDI_remote_and_local_limits_t;

typedef struct
{
  MPIDI_remote_and_local_limits_t eager;
  MPIDI_remote_and_local_limits_t immediate;
} MPIDI_immediate_and_eager_limits_t;

typedef struct
{
  MPIDI_immediate_and_eager_limits_t application;
  MPIDI_immediate_and_eager_limits_t internal;
} MPIDI_pt2pt_limits_t;

/**
 * \brief MPI Process descriptor
 *
 * This structure contains global configuration flags.
 */
typedef struct
{
  unsigned avail_contexts;
  union
  {
    unsigned             limits_array[8];
    unsigned             limits_lookup[2][2][2];
    MPIDI_pt2pt_limits_t limits;
  } pt2pt;
  unsigned disable_internal_eager_scale; /**< The number of tasks at which point eager will be disabled */
#if TOKEN_FLOW_CONTROL
  unsigned long long mp_buf_mem;
  unsigned long long mp_buf_mem_max;
  unsigned is_token_flow_control_on;
#endif
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
  unsigned mp_infolevel;
  unsigned mp_statistics;     /* print pamid statistcs data                           */
  unsigned mp_printenv; ;     /* print env data                                       */
  unsigned mp_interrupts; ;   /* interrupts                                           */
#endif
#ifdef RDMA_FAILOVER
  unsigned mp_s_use_pami_get; /* force the PAMI_Get path instead of PAMI_Rget         */
#endif

#if (MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_THREAD_LOCAL) && defined(__BGQ__)
  MPIDI_RequestHandle_t request_handles[MPIDI_MAX_THREADS];
#endif

#if QUEUE_BINARY_SEARCH_SUPPORT
  unsigned queue_binary_search_support_on;
#endif

#if CUDA_AWARE_SUPPORT
  unsigned cuda_aware_support_on;
#endif

  unsigned verbose;        /**< The current level of verbosity for end-of-job stats. */
  unsigned statistics;     /**< The current level of stats collection.               */
  unsigned rma_pending;    /**< The max num outstanding requests during an RMA op    */
  unsigned shmem_pt2pt;    /**< Enable optimized shared memory point-to-point functions. */
  unsigned smp_detect;
  pami_geometry_t world_geometry;

  struct
  {
    unsigned collectives;       /**< Enable optimized collective functions. */
    unsigned subcomms;          /**< Enable hardware optimized subcomm's */
    unsigned select_colls;      /**< Enable collective selection */
    unsigned auto_select_colls; /**< Enable automatic collective selection */
    unsigned memory;            /**< Enable memory optimized subcomm's - See MPID_OPT_LVL_xxxx */
    unsigned num_requests;      /**< Number of requests between flow control barriers */
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

  unsigned mpir_nbc;         /**< Enable MPIR_* non-blocking collectives implementations. */
  int  numTasks;             /* total number of tasks on a job                            */
  unsigned typed_onesided;       /**< Enable typed PAMI calls for derived types within MPID_Put and MPID_Get. */
#ifdef DYNAMIC_TASKING
  struct MPIDI_PG_t * my_pg; /**< Process group I belong to */
  int                 my_pg_rank; /**< Rank in process group */
#endif
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
    MPIDI_Protocols_WinGetAccum,
    MPIDI_Protocols_WinGetAccumAck,
#ifdef DYNAMIC_TASKING
    MPIDI_Protocols_Dyntask,
    MPIDI_Protocols_Dyntask_disconnect,
#endif
    MPIDI_Protocols_WinAtomic,
    MPIDI_Protocols_WinAtomicAck,
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
    MPIDI_CONTROL_RETURN_TOKENS,
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
      unsigned    noRDMA:1;    /**< msg sent via shm or mem reg. fails */
      unsigned    reserved:6;  /**< unused bits                        */
      unsigned    tokens:4;    /** tokens need to be returned          */
    } __attribute__ ((__packed__));
  };

#ifdef OUT_OF_ORDER_HANDLING
  unsigned    MPIseqno;    /**< match seqno            */
#endif
#if TOKEN_FLOW_CONTROL
  unsigned    alltokens;   /* control:MPIDI_CONTROL_RETURN_TOKENS  */
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
  MPI_Aint              userbufcount; /**< Userbuf data count         */
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
  int   partner_id;
  int   idx;
  int   PR_idx;
#endif
  struct MPIDI_Win_request   *win_req; /* anchor of request based rma handle so as to free it properly when wait is called */
};

typedef void* fast_query_t;
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
  char optgather, optscatter, optreduce;
  unsigned num_requests;
  /* These need to be freed at geom destroy, so we need to store them
   * inside the communicator struct until destroy time rather than
   * allocating pointers on the stack
   */
  /* For create_taskrange */
  pami_geometry_range_t range;
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
  /* Our best allreduce protocol always works on 
   * doubles and sum/min/max. Since that is a common
   * occurance let's cache that protocol and call
   * it without checking.  Any other dt/op must be 
   * checked */ 
  pami_algorithm_t cached_allreduce;
  pami_metadata_t cached_allreduce_md;
  int query_cached_allreduce; 

  union tasks_descrip_t {
    /* For create_taskrange */
    pami_geometry_range_t *ranges;
    /* For create_tasklist/endpoints if we ever use it */
    pami_task_t *tasks;
    pami_endpoint_t *endpoints;
  } tasks_descriptor;
#ifdef DYNAMIC_TASKING
  int local_leader;
  long long world_intercomm_cntr;
  int *world_ids;      /* ids of worlds that composed this communicator (inter communicator created for dynamic tasking */
#endif
  fast_query_t collsel_fast_query;
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

typedef enum
  {
    MPIDI_REQUEST_LOCK,
    MPIDI_REQUEST_LOCKALL,
  } MPIDI_LOCK_TYPE_t;

struct MPIDI_Win_lock
{
  struct MPIDI_Win_lock *next;
  unsigned               rank;
  MPIDI_LOCK_TYPE_t      mtype;    /* MPIDI_REQUEST_LOCK or MPIDI_REQUEST_LOCKALL    */
  int                    type;
  void                   *flagAddr;
};
struct MPIDI_Win_queue
{
  struct MPIDI_Win_lock *head;
  struct MPIDI_Win_lock *tail;
};

typedef enum {
    MPIDI_ACCU_ORDER_RAR = 1,
    MPIDI_ACCU_ORDER_RAW = 2,
    MPIDI_ACCU_ORDER_WAR = 4,
    MPIDI_ACCU_ORDER_WAW = 8
} MPIDI_Win_info_accumulate_ordering;

typedef enum {
    MPIDI_ACCU_SAME_OP,
    MPIDI_ACCU_SAME_OP_NO_OP
} MPIDI_Win_info_accumulate_ops;

typedef struct MPIDI_Win_info_args {
    int no_locks;
    MPIDI_Win_info_accumulate_ordering accumulate_ordering;
    MPIDI_Win_info_accumulate_ops      accumulate_ops;       /* default is same_op_no_op  */
    int same_size;
    int alloc_shared_noncontig;
} MPIDI_Win_info_args;

typedef struct workQ_t {
   void *msgQ;
   int  count;
} workQ_t;


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
typedef struct MPIDI_Win_info
{
  void             * base_addr;     /**< Node's exposure window base address                  */
  struct MPID_Win  * win;
  uint32_t           disp_unit;     /**< Node's exposure window displacement units            */
  pami_memregion_t   memregion;     /**< Memory region descriptor for each node               */
  uint32_t           memregion_used;
  MPI_Aint           base_size;     /**< Node's exposure window base size in bytes            */
} MPIDI_Win_info;

typedef pthread_mutex_t MPIDI_SHM_MUTEX;

typedef struct MPIDI_Win_shm_ctrl_t
{
  MPIDI_SHM_MUTEX  mutex_lock;    /* shared memory windows -- lock for    */
                                     /*     accumulate/atomic operations     */
  OPA_int_t       active;
  int        shm_count;
} MPIDI_Win_shm_ctrl_t;

typedef struct MPIDI_Win_shm_t
{
    int allocated;                  /* flag: TRUE iff this window has a shared memory
                                                 region associated with it */
    void *base_addr;                /* base address of shared memory region */
    MPI_Aint segment_len;           /* size of shared memory region         */
    union
    {
      uint32_t shm_id;                /* shared memory id - sysv            */
      char     shm_key[64];         /* shared memory key - posix            */
    };
    MPIDI_Win_shm_ctrl_t  *ctrl;
} MPIDI_Win_shm_t;

/**
 * \brief Structure of PAMI extensions to MPID_Win structure
 */
struct MPIDI_Win
{
  struct MPIDI_Win_info     *info;          /**< allocated array of collective info             */
  MPIDI_Win_info_args info_args;
  void             ** shm_base_addrs; /* base address shared by all process in comm      */
  MPIDI_Win_shm_t  *shm;             /* shared memory info                             */
  workQ_t work;
  int   max_ctrlsends;
  struct MPIDI_Win_sync
  {
#if 0
    /** \todo optimize some of the synchronization assertion */
    uint32_t assert; /**< MPI_MODE_* bits asserted at epoch start              */
#endif

    volatile int origin_epoch_type; /**< curretn epoch type for origin */
    volatile int target_epoch_type; /**< curretn epoch type for target */

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
        volatile unsigned allLocked;
      } remote;
      struct
      {
        struct MPIDI_Win_queue requested;
        int                    type;
        unsigned               count;
      } local;
    } lock;
  } sync;
  int request_based;          /* flag for request based rma */
  struct MPID_Request *rreq;  /* anchor of MPID_Request for request based rma */
};

/**
 * \brief Structures and typedefs for collective selection extensions in PAMI
 */

typedef void* advisor_t;
typedef void* advisor_table_t;
typedef void* advisor_attribute_name_t;

typedef union
{
  size_t         intval;
  double         doubleval;
  const char *   chararray;
  const size_t * intarray;
} advisor_attribute_value_t;

typedef struct
{
  advisor_attribute_name_t  name;
  advisor_attribute_value_t value;
} advisor_configuration_t;

typedef struct {
   pami_xfer_type_t  *collectives;
   size_t             num_collectives;
   size_t            *procs_per_node;
   size_t             num_procs_per_node;
   size_t            *geometry_sizes;
   size_t             num_geometry_sizes;
   size_t            *message_sizes;
   size_t             num_message_sizes;
   int                iter;
   int                verify;
   int                verbose;
   int                checkpoint;
} advisor_params_t;

typedef enum
{
  COLLSEL_ALGO = 0,      /* 'Always works' PAMI algorithm */
  COLLSEL_QUERY_ALGO,    /* 'Must query' PAMI algorithm */
  COLLSEL_EXTERNAL_ALGO, /* External algorithm */
} advisor_algorithm_type_t;

/* External algorithm callback function */
typedef pami_result_t (*external_algorithm_fn)(pami_xfer_t *, void *);

typedef struct
{
  external_algorithm_fn callback;
  void                 *cookie;
} external_algorithm_t;

typedef struct
{
  union
  {
    pami_algorithm_t     internal;/* PAMI Algorithm */
    external_algorithm_t external;/* External Algorithm */
  } algorithm;
  pami_metadata_t *metadata;
  advisor_algorithm_type_t algorithm_type;
} advisor_algorithm_t;


typedef pami_result_t (*pami_extension_collsel_init)            (pami_client_t,
                                                                 advisor_configuration_t [],
                                                                 size_t,
                                                                 pami_context_t [],
                                                                 size_t,
                                                                 advisor_t *);

typedef pami_result_t (*pami_extension_collsel_destroy)         (advisor_t *);

typedef int (*pami_extension_collsel_initialized)               (pami_client_t, advisor_t *);

typedef pami_result_t (*pami_extension_collsel_table_load)      (advisor_t,
                                                                 char *,
                                                                 advisor_table_t *);

typedef pami_result_t (*pami_extension_collsel_get_collectives) (advisor_table_t,
                                                                 pami_xfer_type_t **,
                                                                 unsigned          *);

typedef pami_result_t (*pami_extension_collsel_register_algorithms) (advisor_table_t,
                                                                     pami_geometry_t,
                                                                     pami_xfer_type_t,
                                                                     advisor_algorithm_t *,
                                                                     size_t);

typedef pami_result_t (*external_geometry_create_fn)(pami_geometry_range_t* task_slices,
                                                     size_t           slice_count,
                                                     pami_geometry_t *geometry,
                                                     void           **cookie);

typedef pami_result_t (*external_geometry_destroy_fn)(void *cookie);

typedef pami_result_t (*external_register_algorithms_fn)(void *cookie,
                                                         pami_xfer_type_t collective,
                                                         advisor_algorithm_t **algorithms,
                                                         size_t              *num_algorithms);

typedef struct
{
  external_geometry_create_fn   geometry_create;
  external_geometry_destroy_fn  geometry_destroy;
  external_register_algorithms_fn register_algorithms;
} external_geometry_ops_t;

typedef pami_result_t (*pami_extension_collsel_table_generate)  (advisor_t,
                                                                 char *,
                                                                 advisor_params_t *,
                                                                 external_geometry_ops_t *,
                                                                 int);

typedef pami_result_t (*pami_extension_collsel_query_create) (advisor_table_t  advisor_table,
                                                              pami_geometry_t  geometry,
                                                              fast_query_t    *query);

typedef pami_result_t (*pami_extension_collsel_query_destroy) (fast_query_t *query);

typedef int (*pami_extension_collsel_advise) (fast_query_t        fast_query,
                                              pami_xfer_type_t    xfer_type,
                                              size_t              message_size,
                                              advisor_algorithm_t algorithms_optimized[],
                                              size_t              max_algorithms);



#endif
