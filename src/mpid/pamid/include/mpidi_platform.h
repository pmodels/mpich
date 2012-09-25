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

/* Default values */

#define MPIDI_MAX_CONTEXTS 64
/** This is not the real value, but should default to something larger than PAMI_DISPATCH_SEND_IMMEDIATE_MAX */
#define MPIDI_SHORT_LIMIT  555
/** This is set to 4 BGQ torus packets (+1, because of the way it is compared) */
#define MPIDI_EAGER_LIMIT  2049
/** This is set to 0 which effectively disables the eager protocol for local transfers */
#define MPIDI_EAGER_LIMIT_LOCAL  0
/* Default features */
#define USE_PAMI_RDMA 1
#define USE_PAMI_CONSISTENCY PAMI_HINT_ENABLE
#undef  OUT_OF_ORDER_HANDLING
#undef  RDMA_FAILOVER

#define ASYNC_PROGRESS_MODE_DEFAULT 0

#ifdef __BGQ__
#undef  MPIDI_EAGER_LIMIT_LOCAL
#define MPIDI_EAGER_LIMIT_LOCAL  64
#define MPIDI_MAX_THREADS     64
#define MPIDI_MUTEX_L2_ATOMIC 1
#define MPIDI_OPTIMIZED_COLLECTIVE_DEFAULT 1
#define PAMIX_IS_LOCAL_TASK
#define PAMIX_IS_LOCAL_TASK_STRIDE  (4)
#define PAMIX_IS_LOCAL_TASK_BITMASK (0x40)

#undef ASYNC_PROGRESS_MODE_DEFAULT
#define ASYNC_PROGRESS_MODE_DEFAULT 1

static const char _ibm_release_version_[] = "V1R2M0";
#endif

#ifdef __PE__
#undef USE_PAMI_CONSISTENCY
#define USE_PAMI_CONSISTENCY PAMI_HINT_DISABLE
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
#define PAMIX_IS_LOCAL_TASK
#define PAMIX_IS_LOCAL_TASK_STRIDE  (1)
#define PAMIX_IS_LOCAL_TASK_BITMASK (0x01)

#undef ASYNC_PROGRESS_MODE_DEFAULT
#define ASYNC_PROGRESS_MODE_DEFAULT 2

/* When the Pok build team extracts this file from CMVC, %W% will expand to */
/* a string with the current release, for example ppe_rbarlx.               */
/* If this file is cloned from GIT then %W% will not be expanded.  The      */
/* banner code has accounted for this situation.                            */
static const char _ibm_release_version_[] = "%W%";

#endif


#endif
