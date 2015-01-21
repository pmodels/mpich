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
 * \file include/mpidi_platform.h
 * \brief ???
 */


#ifndef __include_mpidi_platform_h__
#define __include_mpidi_platform_h__

/* Allow MPICH to detect local tasks */
#define MPID_USE_NODE_IDS
typedef int32_t MPID_Node_id_t;

/* Default values */

#define MPIDI_MAX_CONTEXTS 64
/** This is not the real value, but should default to something larger than PAMI_DISPATCH_SEND_IMMEDIATE_MAX */
#define MPIDI_SHORT_LIMIT  555
/** This is set to 4 BGQ torus packets (+1, because of the way it is compared) */
#define MPIDI_EAGER_LIMIT  2049
/** This is set to 0 which effectively disables the eager protocol for local transfers */
#define MPIDI_EAGER_LIMIT_LOCAL  0
/** This is set to 'max unsigned' which effectively never disables internal eager at scale */
#define MPIDI_DISABLE_INTERNAL_EAGER_SCALE ((unsigned)-1)

/* Default features */
#define USE_PAMI_RDMA 1
#define USE_PAMI_CONSISTENCY PAMI_HINT_ENABLE
#undef  OUT_OF_ORDER_HANDLING
#undef  DYNAMIC_TASKING
#undef  RDMA_FAILOVER
#undef  QUEUE_BINARY_SEARCH_SUPPORT

#define ASYNC_PROGRESS_MODE_DEFAULT 0

/*
 * The default behavior is to disable (ignore) the 'internal vs application' and
 * the 'local vs remote' point-to-point eager limits.
 */
#define MPIDI_PT2PT_EAGER_LIMIT(is_internal,is_local)                           \
({                                                                              \
  MPIDI_Process.pt2pt.limits_lookup[0][0][0];                                   \
})

/*
 * The default behavior is to disable (ignore) the 'internal vs application' and
 * the 'local vs remote' point-to-point short limits.
 */
#define MPIDI_PT2PT_SHORT_LIMIT(is_internal,is_local)                           \
({                                                                              \
  MPIDI_Process.pt2pt.limits_lookup[0][1][0];                                   \
})



#ifdef __BGQ__
#undef  MPIDI_EAGER_LIMIT_LOCAL
#define MPIDI_EAGER_LIMIT_LOCAL  4097
#undef  MPIDI_EAGER_LIMIT
#define MPIDI_EAGER_LIMIT  4097
#undef  MPIDI_DISABLE_INTERNAL_EAGER_SCALE
#define MPIDI_DISABLE_INTERNAL_EAGER_SCALE (512*1024)
#define MPIDI_MAX_THREADS     64
#define MPIDI_MUTEX_L2_ATOMIC 1
#define MPIDI_OPTIMIZED_COLLECTIVE_DEFAULT 1

#define PAMIX_IS_LOCAL_TASK
#define PAMIX_IS_LOCAL_TASK_STRIDE  (4)
#define PAMIX_IS_LOCAL_TASK_SHIFT   (6)
#define MPIDI_SMP_DETECT_DEFAULT 1
#define TOKEN_FLOW_CONTROL    0
#define CUDA_AWARE_SUPPORT    0

/*
 * Enable both the 'internal vs application' and the 'local vs remote'
 * point-to-point eager limits.
 */
#undef MPIDI_PT2PT_EAGER_LIMIT
#define MPIDI_PT2PT_EAGER_LIMIT(is_internal,is_local)                           \
({                                                                              \
  MPIDI_Process.pt2pt.limits_lookup[is_internal][0][is_local];                  \
})

/*
 * Enable both the 'internal vs application' and the 'local vs remote'
 * point-to-point short limits.
 */
#undef MPIDI_PT2PT_SHORT_LIMIT
#define MPIDI_PT2PT_SHORT_LIMIT(is_internal,is_local)                           \
({                                                                              \
  MPIDI_Process.pt2pt.limits_lookup[is_internal][1][is_local];                  \
})


#undef ASYNC_PROGRESS_MODE_DEFAULT
#define ASYNC_PROGRESS_MODE_DEFAULT 1

static const char _ibm_release_version_[] = "V1R2M0";
#endif

#ifdef __PE__

/*
 * This 'maximum contexts' define needs to be changed when mpich on PE
 * will support multiple contexts. Currently the PE PAMI allows multiple
 * contexts, but the PE mpich code is not set up to use them.
 */
#undef MPIDI_MAX_CONTEXTS
#define MPIDI_MAX_CONTEXTS 1

#undef USE_PAMI_CONSISTENCY
#define USE_PAMI_CONSISTENCY PAMI_HINT_DISABLE
#undef  MPIDI_SHORT_LIMIT
#define MPIDI_SHORT_LIMIT 256 - sizeof(MPIDI_MsgInfo)
#undef  MPIDI_EAGER_LIMIT
#define MPIDI_EAGER_LIMIT 65536
#undef  MPIDI_EAGER_LIMIT_LOCAL
#define MPIDI_EAGER_LIMIT_LOCAL  1048576
#define OUT_OF_ORDER_HANDLING 1
#define MPIDI_STATISTICS      1
#define MPIDI_PRINTENV        1
#define MPIDI_OPTIMIZED_COLLECTIVE_DEFAULT 0
#undef  USE_PAMI_RDMA
#define RDMA_FAILOVER
#define MPIDI_BANNER          1
#define MPIDI_NO_ASSERT       1
#define TOKEN_FLOW_CONTROL    1
#define DYNAMIC_TASKING       1
#define CUDA_AWARE_SUPPORT    1

/* 'is local task' extension and limits */
#define PAMIX_IS_LOCAL_TASK
#define PAMIX_IS_LOCAL_TASK_STRIDE  (1)
#define PAMIX_IS_LOCAL_TASK_SHIFT   (0)
#define MPIDI_SMP_DETECT_DEFAULT 1
/*
 * Enable only the 'local vs remote' point-to-point eager limits.
 */
#undef MPIDI_PT2PT_EAGER_LIMIT
#define MPIDI_PT2PT_EAGER_LIMIT(is_internal,is_local)                           \
({                                                                              \
  MPIDI_Process.pt2pt.limits_lookup[0][0][is_local];                            \
})

/*
 * Enable only the 'local vs remote' point-to-point short limits.
 */
#undef MPIDI_PT2PT_SHORT_LIMIT
#define MPIDI_PT2PT_SHORT_LIMIT(is_internal,is_local)                           \
({                                                                              \
  MPIDI_Process.pt2pt.limits_lookup[0][1][is_local];                            \
})


#undef ASYNC_PROGRESS_MODE_DEFAULT
#define ASYNC_PROGRESS_MODE_DEFAULT 2

/* When the Pok build team extracts this file from CMVC, %W% will expand to */
/* a string with the current release, for example ppe_rbarlx.               */
/* If this file is cloned from GIT then %W% will not be expanded.  The      */
/* banner code has accounted for this situation.                            */
static const char _ibm_release_version_[] = "%W%";

#endif

#if TOKEN_FLOW_CONTROL
#define BUFFER_MEM_DEFAULT (1<<26)          /* 64MB                         */
#define BUFFER_MEM_MAX     (1<<26)          /* 64MB                         */
#define ONE_SHARED_SEGMENT (1<<28)          /* 256MB                        */
#define EAGER_LIMIT_DEFAULT     65536
#define MAX_BUF_BKT_SIZE        (1<<18)     /* Max eager_limit is 256K     */
#define MIN_BUF_BKT_SIZE        (64)
#define TOKENS_BIT         (4)              /* 4 bits piggy back to sender */
                                            /* should be consistent with tokens
                                               defined in MPIDI_MsgInfo    */
#define TOKENS_BITMASK ((1 << TOKENS_BIT)-1)
#endif

#endif
