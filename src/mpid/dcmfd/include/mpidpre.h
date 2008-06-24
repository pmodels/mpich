/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/mpidpre.h
 * \brief The leading device header
 *
 * This file is included at the start of the other headers
 * (mpidimpl.h, mpidpost.h, and mpiimpl.h).  It generally contains
 * additions to MPI objects.
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef MPICH_MPIDPRE_H_INCLUDED
#define MPICH_MPIDPRE_H_INCLUDED

/* include message layer stuff */
#include <dcmf.h>
#include <dcmf_globalcollectives.h>
#include <dcmf_collectives.h>

/* verify that the version of the installed dcmf library is compatible */
#if (DCMF_VERSION_RELEASE == 0)
  #if (DCMF_VERSION_MAJOR == 1)
    #if (DCMF_VERSION_MINOR < 0)
      #error Incompatible dcmf minor version
    #endif
  #else
    #error Incompatible dcmf major version
  #endif
#else
  #error Incompatible dcmf release version
#endif


#include "mpid_dataloop.h"

/**
 * \brief Declare hook(s) for Datatype create/destroy
 *
 * multiple hooks could be defined, for example:
 * #define ...hook(a)   { func1(a); func2(a); ... }
 */
#ifdef MPID_Dev_datatype_create_hook
#error  MPID_Dev_datatype_create_hook already defined somewhere else!
#else /* !MPID_Dev_datatype_create_hook */
#define MPID_Dev_datatype_create_hook(a)
#endif /* !MPID_Dev_datatype_create_hook */

#ifdef MPID_Dev_datatype_destroy_hook
#error  MPID_Dev_datatype_destroy_hook already defined somewhere else!
#else /* !MPID_Dev_datatype_destroy_hook */
#define MPID_Dev_datatype_destroy_hook(a)       {\
        extern void MPIDU_dtc_free(MPID_Datatype *);\
        MPIDU_dtc_free(a);			\
}
#endif /* !MPID_Dev_datatype_destroy_hook */

/**
 * ******************************************************************
 * \brief Mutexes for interrupt driven mode
 * ******************************************************************
 */
#ifdef MPID_CS_ENTER
#error "MPID_CS_ENTER is already defined"
#endif
#define MPID_DEFINES_MPID_CS 1
#define MPID_CS_INITIALIZE()                                          \
{                                                                     \
  /* Create thread local storage for nest count that MPICH uses */    \
  MPID_Thread_tls_create(NULL, &MPIR_ThreadInfo.thread_storage, NULL);   \
}
#define MPID_CS_FINALIZE()                                            \
{                                                                     \
  /* Destroy thread local storage created during MPID_CS_INITIALIZE */\
  MPID_Thread_tls_destroy(&MPIR_ThreadInfo.thread_storage, NULL);	      \
}
#if (MPICH_THREAD_LEVEL != MPI_THREAD_MULTIPLE)
#define MPID_CS_ENTER()      {}
#define MPID_CS_EXIT()       {}
#define MPID_CS_CYCLE()      {}
#else
#define MPID_CS_ENTER()      DCMF_CriticalSection_enter(0);
#define MPID_CS_EXIT()       DCMF_CriticalSection_exit(0);
#define MPID_CS_CYCLE()      DCMF_CriticalSection_cycle(0);
#endif


typedef struct MPIDI_VC
{
  int          handle;
  volatile int ref_count;
  int          lpid;
}
MPIDI_VC;

typedef struct MPIDI_VCRT * MPID_VCRT;
typedef struct MPIDI_VC   * MPID_VCR;
#define MPID_GPID_Get(comm_ptr, rank, gpid)     \
{                                               \
  gpid[0] = 0;                                  \
  gpid[1] = comm_ptr->vcr[rank]->lpid;          \
}

/** \brief Our progress engine does not require state */
#define MPID_PROGRESS_STATE_DECL

/**
 * ******************************************************************
 * \brief MPI Onesided operation device declarations (!!! not used)
 * Is here only because mpiimpl.h needs it.
 * ******************************************************************
 */
typedef struct MPIDI_RMA_ops {
    struct MPIDI_RMA_ops *next;  /* pointer to next element in list */
    int type;  /* MPIDI_RMA_PUT, MPID_REQUEST_GET,
                  MPIDI_RMA_ACCUMULATE, MPIDI_RMA_LOCK */
    void *origin_addr;
    int origin_count;
    MPI_Datatype origin_datatype;
    int target_rank;
    MPI_Aint target_disp;
    int target_count;
    MPI_Datatype target_datatype;
    MPI_Op op;  /* for accumulate */
    int lock_type;  /* for win_lock */
} MPIDI_RMA_ops;

/* to send derived datatype across in RMA ops */
typedef struct MPIDI_RMA_dtype_info
{
  int           is_contig;
  int           n_contig_blocks;
  int           size;
  MPI_Aint      extent;
  int           dataloop_size;
  void          *dataloop;
  int           dataloop_depth;
  int           eltype;
  MPI_Aint ub;
  MPI_Aint lb;
  MPI_Aint true_ub;
  MPI_Aint true_lb;
  int has_sticky_ub;
  int has_sticky_lb;
  int unused0;
  int unused1;
}
MPIDI_RMA_dtype_info;


/**
 * \brief This defines the type of message being sent/received
 * mpid_startall() invokes the correct start based on the type of the request
 */
typedef enum
  {
    MPIDI_DCMF_REQUEST_TYPE_RECV=0,
    MPIDI_DCMF_REQUEST_TYPE_SEND,
    MPIDI_DCMF_REQUEST_TYPE_RSEND,
    MPIDI_DCMF_REQUEST_TYPE_BSEND,
    MPIDI_DCMF_REQUEST_TYPE_SSEND,
    MPIDI_DCMF_REQUEST_TYPE_SSEND_ACKNOWLEDGE,
    MPIDI_DCMF_REQUEST_TYPE_CANCEL_REQUEST,
    MPIDI_DCMF_REQUEST_TYPE_CANCEL_ACKNOWLEDGE,
    MPIDI_DCMF_REQUEST_TYPE_CANCEL_NOT_ACKNOWLEDGE,
    MPIDI_DCMF_REQUEST_TYPE_RENDEZVOUS_ACKNOWLEDGE,
    MPIDI_DCMF_REQUEST_TYPE_PLACEHOLDER
  }
MPIDI_DCMF_REQUEST_TYPE;


typedef enum
  {
    MPIDI_DCMF_INITIALIZED=0,
    MPIDI_DCMF_SEND_COMPLETE,
    MPIDI_DCMF_ACKNOWLEGED,
    MPIDI_DCMF_REQUEST_DONE_CANCELLED
  }
MPIDI_DCMF_REQUEST_STATE;


/** \brief Request completion actions */
typedef enum
  {
    MPIDI_DCMF_CA_ERROR = 0,                         /* Should never see this        */
    MPIDI_DCMF_CA_COMPLETE = 1,                      /* The request is now complete  */
    MPIDI_DCMF_CA_UNPACK_UEBUF_AND_COMPLETE,         /* Unpack uebuf, then complete  */
    MPIDI_DCMF_CA_UNPACK_UEBUF_AND_COMPLETE_NOFREE,  /* Unpack uebuf, then complete. do not free uebuf  */
  }
MPIDI_DCMF_CA;


/**
 * \brief MPIDI_Message_match contains enough information to match an
 * MPI message.
 */
typedef struct MPIDI_Message_match
{
  int   tag;        /**< match tag     */
  int   rank;       /**< match rank    */
  int   context_id; /**< match context */
}
MPIDI_Message_match;


/**
 * \brief Message Info (has to be exactly 128 bits long) and associated data types
 * \note sizeof(MPIDI_DCMF_MsgInfo) == 16
 */
struct MPIDI_DCMF_MsgInfo_t
  {
    void     * req;         /**< peer's request pointer */
    unsigned   MPItag;      /**< match tag              */
    unsigned   MPIrank;     /**< match rank             */
    uint16_t   MPIctxt;     /**< match context          */

    uint16_t   type:8;      /**< message type           */
    uint16_t   isSelf:1;    /**< message sent to self   */
    uint16_t   isSync:1;    /**< set for sync sends     */

    uint16_t   isRzv:1;     /**< use pt2pt rendezvous   */

    /* These are not currently in use : */
    uint16_t   isResend:1;    /**< Unused: this message is a re-send */
    uint16_t   isSending:1;   /**< Unused: message is currently being sent */
    uint16_t   extra_flags:3; /**< Unused */
};

typedef union MPIDI_DCMF_MsgInfo
{
  struct MPIDI_DCMF_MsgInfo_t msginfo;
  DCQuad quad[DCQuad_sizeof(struct MPIDI_DCMF_MsgInfo_t)];
}
MPIDI_DCMF_MsgInfo;

/** \brief Full Rendezvous msg info to be set as two quads of unexpected data. */
typedef union
{
  struct MPIDI_DCMF_MsgEnvelope_t
  {
    MPIDI_DCMF_MsgInfo msginfo;
    DCMF_Memregion_t   memregion;
    size_t             offset;
    unsigned           length;
  } envelope;
  DCQuad quad[DCQuad_sizeof(struct MPIDI_DCMF_MsgEnvelope_t)];
} MPIDI_DCMF_MsgEnvelope;

/** \brief This defines the portion of MPID_Request that is specific to the DCMF Device */
struct MPIDI_DCMF_Request
{
  MPIDI_DCMF_MsgEnvelope    envelope;
  unsigned                  peerrank;     /**< The other guy's rank       */

  MPIDI_DCMF_CA ca;                       /**< Completion action          */

  char                    * userbuf;      /**< User buffer                */
  unsigned                  userbufcount; /**< Userbuf data count         */
  char                    * uebuf;        /**< Unexpected buffer          */
  unsigned                  uebuflen;     /**< Length (bytes) of uebuf    */

  MPI_Datatype              datatype;     /**< Data type of message       */
  struct MPID_Datatype    * datatype_ptr; /**< Info about the datatype    */

  int                     cancel_pending; /**< Cancel State               */
  MPIDI_DCMF_REQUEST_STATE  state;        /**< The tranfser state         */

  DCMF_Request_t            msg;          /**< The message layer request  */

  DCMF_Memregion_t          memregion;    /**< Rendezvous rcv memregion   */

  struct MPID_Request     * next;         /**< Link to next req. in queue */
};
/** \brief This defines the portion of MPID_Request that is specific to the DCMF Device */
#define MPID_DEV_REQUEST_DECL        struct MPIDI_DCMF_Request dcmf;


/** \brief needed by the (stolen) CH3 implementation of dcmf_buffer.c */
typedef unsigned MPIDI_msg_sz_t;

/** \brief This defines the portion of MPID_Comm that is specific to the DCMF Device */
struct MPIDI_DCMF_Comm
{
  DCMF_Geometry_t geometry; /**< Geometry component for collectives           */
  DCMF_CollectiveRequest_t barrier; /**< Barrier request for collectives      */
  unsigned *worldranks;     /**< rank list to be used by collectives          */
   unsigned *sndlen; /**< lazy alloc alltoall vars */
   unsigned *rcvlen;
   unsigned *sdispls;
   unsigned *rdispls;
   unsigned *sndcounters;
   unsigned *rcvcounters;
   unsigned char allreducetree; /**< Comm specific tree flags */
   unsigned char allreducepipelinedtree; /**< Comm specific tree flags */
   unsigned char allreducepipelinedtree_dput; /**< Comm specific tree flags */
   unsigned char reducetree;
   unsigned char allreduceccmitree;
   unsigned char reduceccmitree;
   unsigned char bcasttree;
   unsigned char alltoalls;
   unsigned      bcastiter;   /* async broadcast is only used every 32
			       * steps to prevent too many unexpected
			       * messages */
};
/** \brief This defines the portion of MPID_Comm that is specific to the DCMF Device */
#define MPID_DEV_COMM_DECL      struct MPIDI_DCMF_Comm dcmf;


#ifdef HAVE_DEV_COMM_HOOK
#error "Build error - HAVE_DEV_COMM_HOOK defined at least twice!"
#else
#define HAVE_DEV_COMM_HOOK
#define MPID_Dev_comm_create_hook(a)  MPIDI_Comm_create(a)
#define MPID_Dev_comm_destroy_hook(a) MPIDI_Comm_destroy(a)
#endif


struct MPID_Comm;

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
struct MPID_Win_coll_info {
  void *base_addr;      /**< Node's exposure window base address                  */
  int disp_unit;        /**< Node's exposure window displacement units            */
  MPI_Win win_handle;   /**< Node's exposure window handle (local to target node) */
  int rma_sends;        /**< Count of RMA operations that target node             */
  DCMF_Memregion_t mem_region; /**< Memory region descriptor for each node */
};

/* assert sizeof(struct MPID_Win_coll_info) == 16 */

/**
 * \brief Structure of BG extensions to MPID_Win structure
 */
struct MPID_Dev_win_decl {
  struct MPID_Win_coll_info *coll_info; /**< allocated array of collective info       */
  struct MPID_Comm *comm_ptr;     /**< saved pointer to window communicator           */
  volatile int lock_granted;      /**< window lock                                    */
  unsigned long _lock_queue[4];   /**< opaque structure used for lock wait queue      */
  unsigned long _unlk_queue[4];   /**< opaque structure used for unlock wait queue    */
  volatile int my_sync_begin;     /**< counter of POST messages received              */
  volatile int my_sync_done;      /**< counter of COMPLETE messages received          */
  volatile int my_rma_recvs;      /**< counter of RMA operations received             */
  volatile int my_rma_pends;      /**< counter of RMA operations queued to send       */
  volatile int my_get_pends;      /**< counter of GET operations queued               */
  DCMF_Consistency my_cstcy;      /**< default consistency for window                 */
  volatile int epoch_type;        /**< current epoch type                             */
  volatile int epoch_size;        /**< current epoch size (or target for LOCK)        */
  int epoch_assert;               /**< MPI_MODE_* bits asserted at epoch start        */
  int epoch_rma_ok;               /**< flag indicating an exposure epoch is in affect */
};

/**
 * \brief Code-snippet macro to add BG extensions to MPID_Win object structure
 */
#define MPID_DEV_WIN_DECL struct MPID_Dev_win_decl _dev;


/**
 * @defgroup MPID_EPOTYPE MPID One-sided Epoch Types
 *@{
 */
#define MPID_EPOTYPE_NONE       0       /**< No epoch in affect */
#define MPID_EPOTYPE_LOCK       1       /**< MPI_Win_lock access epoch */
#define MPID_EPOTYPE_START      2       /**< MPI_Win_start access epoch */
#define MPID_EPOTYPE_POST       3       /**< MPI_Win_post exposure epoch */
#define MPID_EPOTYPE_POSTSTART  4       /**< MPI_Win_post+MPI_Win_start access/exposure epoch */
#define MPID_EPOTYPE_FENCE      5       /**< MPI_Win_fence access/exposure epoch */
/**@}*/

/**
 * @defgroup MPID_MSGTYPE MPID One-sided Message Types
 *@{
 */
#define MPID_MSGTYPE_NONE       0       /**< Not a valid message */
#define MPID_MSGTYPE_LOCK       1       /**< lock window */
#define MPID_MSGTYPE_UNLOCK     2       /**< (try) unlock window */
#define MPID_MSGTYPE_POST       3       /**< begin POST epoch */
#define MPID_MSGTYPE_START      4       /**< (not used) */
#define MPID_MSGTYPE_COMPLETE   5       /**< end a START epoch */
#define MPID_MSGTYPE_WAIT       6       /**< (not used) */
#define MPID_MSGTYPE_FENCE      7       /**< (not used) */
#define MPID_MSGTYPE_UNFENCE    8       /**< (not used) */
#define MPID_MSGTYPE_PUT        9       /**< PUT RMA operation */
#define MPID_MSGTYPE_GET        10      /**< GET RMA operation */
#define MPID_MSGTYPE_ACC        11      /**< ACCUMULATE RMA operation */
#define MPID_MSGTYPE_DT_MAP     12      /**< Datatype map payload */
#define MPID_MSGTYPE_DT_IOV     13      /**< Datatype iov payload */
#define MPID_MSGTYPE_LOCKACK    14      /**< lock acknowledge */
#define MPID_MSGTYPE_UNLOCKACK  15      /**< unlock acknowledge, with status */
/**@}*/

#endif /* !MPICH_MPIDPRE_H_INCLUDED */
