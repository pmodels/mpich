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
 * \file include/mpidi_constants.h
 * \brief ???
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef __include_mpidi_constants_h__
#define __include_mpidi_constants_h__

enum
  {
    /* N is "number of operations" and P is number of ranks */
    MPIDI_VERBOSE_NONE        = 0, /**< Do not print any verbose information */
    MPIDI_VERBOSE_SUMMARY_0   = 1, /**< Print summary information on rank 0.     O(1) lines printed. */
    MPIDI_VERBOSE_SUMMARY_ALL = 2, /**< Print summary information on all ranks.  O(P) lines printed. */
    MPIDI_VERBOSE_DETAILS_0   = 2, /**< Print detailed information on rank 0.    O(N) lines printed. */
    MPIDI_VERBOSE_DETAILS_ALL = 3, /**< Print detailed information on all ranks. O(P*N) lines printed. */
  };


/**
 * \defgroup Allgather(v) optimization datatype info
 * \{
 */
enum
  {
    MPID_SEND_CONTIG     = 0, /**< Contiguous send buffer */
    MPID_RECV_CONTIG     = 1, /**< Contiguous recv buffer */
    MPID_RECV_CONTINUOUS = 2, /**< Continuous recv buffer */
    MPID_LARGECOUNT      = 3, /**< Total send count is "large" */
    MPID_MEDIUMCOUNT     = 4, /**< Total send count is "medium" */
    MPID_ALIGNEDBUFFER   = 5, /**< Buffers are 16b aligned */
  };

enum
  {
    MPID_ALLGATHER_PREALLREDUCE  = 0,
    MPID_ALLGATHERV_PREALLREDUCE = 1,
    MPID_ALLREDUCE_PREALLREDUCE  = 2,
    MPID_BCAST_PREALLREDUCE      = 3,
    MPID_SCATTERV_PREALLREDUCE   = 4,
    MPID_GATHER_PREALLREDUCE     = 5,
    MPID_NUM_PREALLREDUCES       = 6,
  };

enum /* The type of protocol selected */
  {
    MPID_COLL_NOQUERY           = 0,
    MPID_COLL_QUERY             = 1,
    /* Can we cache stuff? If not set to ALWAYS_QUERY */
    MPID_COLL_ALWAYS_QUERY      = 2,
    MPID_COLL_CHECK_FN_REQUIRED = 3,
    MPID_COLL_USE_MPICH         = 4,
    MPID_COLL_NOSELECTION       = 5,
    MPID_COLL_OPTIMIZED         = 6,
  };

enum
 {
   MPID_COLL_OFF = 0,
   MPID_COLL_ON  = 1,
   MPID_COLL_FCA = 2, /* Selecting these is fairly easy so special case */
   MPID_COLL_CUDA = 3, /* This is used to enable PAMI geometry but sets default to MPICH */
 };
/** \} */


enum
{
MPID_EPOTYPE_NONE      = 0,       /**< No epoch in affect */
MPID_EPOTYPE_LOCK      = 1,       /**< MPI_Win_lock access epoch */
MPID_EPOTYPE_START     = 2,       /**< MPI_Win_start access epoch */
MPID_EPOTYPE_POST      = 3,       /**< MPI_Win_post exposure epoch */
MPID_EPOTYPE_FENCE     = 4,       /**< MPI_Win_fence access/exposure epoch */
MPID_EPOTYPE_REFENCE   = 5,       /**< MPI_Win_fence possible access/exposure epoch */
MPID_EPOTYPE_LOCK_ALL  = 6,       /**< MPI_Win_lock_all access epoch */
};

enum
{
  MPID_AUTO_SELECT_COLLS_NONE            = 0,
  MPID_AUTO_SELECT_COLLS_BARRIER         = 1,
  MPID_AUTO_SELECT_COLLS_BCAST           = ((int)((MPID_AUTO_SELECT_COLLS_BARRIER        << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_ALLGATHER       = ((int)((MPID_AUTO_SELECT_COLLS_BCAST          << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_ALLGATHERV      = ((int)((MPID_AUTO_SELECT_COLLS_ALLGATHER      << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_ALLREDUCE       = ((int)((MPID_AUTO_SELECT_COLLS_ALLGATHERV     << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_ALLTOALL        = ((int)((MPID_AUTO_SELECT_COLLS_ALLREDUCE      << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_ALLTOALLV       = ((int)((MPID_AUTO_SELECT_COLLS_ALLTOALL       << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_EXSCAN          = ((int)((MPID_AUTO_SELECT_COLLS_ALLTOALLV      << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_GATHER          = ((int)((MPID_AUTO_SELECT_COLLS_EXSCAN         << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_GATHERV         = ((int)((MPID_AUTO_SELECT_COLLS_GATHER         << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_REDUCE_SCATTER  = ((int)((MPID_AUTO_SELECT_COLLS_GATHERV        << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_REDUCE          = ((int)((MPID_AUTO_SELECT_COLLS_REDUCE_SCATTER << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_SCAN            = ((int)((MPID_AUTO_SELECT_COLLS_REDUCE         << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_SCATTER         = ((int)((MPID_AUTO_SELECT_COLLS_SCAN           << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_SCATTERV        = ((int)((MPID_AUTO_SELECT_COLLS_SCATTER        << 1) & 0xFFFFFFFF)),
  MPID_AUTO_SELECT_COLLS_TUNE            = 0x80000000,
  MPID_AUTO_SELECT_COLLS_ALL             = 0xFFFFFFFF,
};

enum /* PAMID_COLLECTIVES_MEMORY_OPTIMIZED levels */
 
{
  MPID_OPT_LVL_IRREG     = 1,       /**< Do not optimize irregular communicators */
  MPID_OPT_LVL_NONCONTIG = 2,       /**< Disable some non-contig collectives     */
};
#endif
