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
 * \file include/mpidi_macros.h
 * \brief ???
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef __include_mpidi_macros_h__
#define __include_mpidi_macros_h__

#include "mpidi_datatypes.h"
#include "mpidi_externs.h"

#define TOKEN_FLOW_CONTROL_ON (TOKEN_FLOW_CONTROL && MPIU_Token_on())

#ifdef TRACE_ON
#ifdef __GNUC__
#define TRACE_ALL(fd, format, ...) fprintf(fd, "%s:%u (%d) " format, __FILE__, __LINE__, MPIR_Process.comm_world->rank, ##__VA_ARGS__)
#define TRACE_OUT(format, ...) TRACE_ALL(stdout, format, ##__VA_ARGS__)
#define TRACE_ERR(format, ...) TRACE_ALL(stderr, format, ##__VA_ARGS__)
#else
#define TRACE_OUT(format...) fprintf(stdout, format)
#define TRACE_ERR(format...) fprintf(stderr, format)
#endif
#else
#define TRACE_OUT(format...)
#define TRACE_ERR(format...)
#endif

#if TOKEN_FLOW_CONTROL
#define MPIU_Token_on() (MPIDI_Process.is_token_flow_control_on)
#else
#define MPIU_Token_on() (0)
#endif

/**
 * \brief Gets significant info regarding the datatype
 * Used in mpid_send, mpidi_send.
 */
#define MPIDI_Datatype_get_info(_count, _datatype,              \
_dt_contig_out, _data_sz_out, _dt_ptr, _dt_true_lb)             \
({                                                              \
  if (HANDLE_GET_KIND(_datatype) == HANDLE_KIND_BUILTIN)        \
    {                                                           \
        (_dt_ptr)        = NULL;                                \
        (_dt_contig_out) = TRUE;                                \
        (_dt_true_lb)    = 0;                                   \
        (_data_sz_out)   = (_count) *                           \
        MPID_Datatype_get_basic_size(_datatype);                \
    }                                                           \
  else                                                          \
    {                                                           \
        MPID_Datatype_get_ptr((_datatype), (_dt_ptr));          \
        (_dt_contig_out) = (_dt_ptr)->is_contig;                \
        (_dt_true_lb)    = (_dt_ptr)->true_lb;                  \
        (_data_sz_out)   = (_count) * (_dt_ptr)->size;          \
    }                                                           \
})

/**
 * \brief Gets data size of the datatype
 */
#define MPIDI_Datatype_get_data_size(_count, _datatype,         \
_data_sz_out)                                                   \
({                                                              \
  if (HANDLE_GET_KIND(_datatype) == HANDLE_KIND_BUILTIN)        \
    {                                                           \
        (_data_sz_out)   = (_count) *                           \
        MPID_Datatype_get_basic_size(_datatype);                \
    }                                                           \
  else                                                          \
    {                                                           \
        MPID_Datatype *_dt_ptr;                                 \
        MPID_Datatype_get_ptr((_datatype), (_dt_ptr));          \
        (_data_sz_out)   = (_count) * (_dt_ptr)->size;          \
    }                                                           \
})

/* Add some error checking for size eventually */
#define MPIDI_Update_last_algorithm(_comm, _name) \
({ strncpy( (_comm)->mpid.last_algorithm, (_name), strlen((_name))+1); })


/**
 * \brief Macro for allocating memory
 *
 * \param[in] count Number of elements to allocate
 * \param[in] type  The type of the memory, excluding "*"
 * \return Address or NULL
 */
#define MPIU_Calloc0(count, type)               \
({                                              \
  size_t __size = (count) * sizeof(type);       \
  type* __p = MPIU_Malloc(__size);              \
  MPID_assert(__p != NULL);                     \
  if (__p != NULL)                              \
    memset(__p, 0, __size);                     \
  __p;                                          \
})

#define MPIU_TestFree(p)                        \
({                                              \
  if (*(p) != NULL)                             \
    {                                           \
      MPIU_Free(*(p));                          \
      *(p) = NULL;                              \
    }                                           \
})


#define MPID_VCR_GET_LPID(vcr, index)           \
({                                              \
  vcr[index]->taskid;                           \
})

#define MPID_VCR_GET_LPIDS(comm, taskids)                      \
({                                                             \
  int i;                                                       \
  taskids=MPIU_Malloc((comm->local_size)*sizeof(pami_task_t)); \
  MPID_assert(taskids != NULL);                                \
  for(i=0; i<comm->local_size; i++)                            \
    taskids[i] = comm->vcr[i]->taskid;                         \
})
#define MPID_VCR_FREE_LPIDS(taskids) MPIU_Free(taskids)

#define MPID_GPID_Get(comm_ptr, rank, gpid)             \
({                                                      \
  gpid[1] = MPID_VCR_GET_LPID(comm_ptr->vcr, rank);     \
  gpid[0] = 0;                                          \
  MPI_SUCCESS; /* return success from macro */          \
})


static inline void
MPIDI_Context_post(pami_context_t       context,
                   pami_work_t        * work,
                   pami_work_function   fn,
                   void               * cookie)
{
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
  /* It is possible that a work function posted to a context may attempt to
   * initiate a communication operation and, if context post were disabled, that
   * operation would be performed directly on the context BY TAKING A LOCK that
   * the is already held by the thread that is advancing the context. This will
   * result in a hang.
   *
   * A solution would be to identify all code flows where this situation might
   * occur and then change the code to avoid taking a lock that is already held.
   *
   * Another solution is to always disable the "non-context-post" configuration
   * when compiled with per-object locking. This would only occur if the
   * application requested !MPI_THREAD_MULTIPLE and the "pretend single threaded
   * by disabling async progress, context post, and multiple contexts" optimization
   * was in effect.
   */
  pami_result_t rc;
  rc = PAMI_Context_post(context, work, fn, cookie);
  MPID_assert(rc == PAMI_SUCCESS);
#else /* (MPICH_THREAD_GRANULARITY != MPICH_THREAD_GRANULARITY_PER_OBJECT) */
  /*
   * It is not necessary to lock the context before access in the "global"
   * mpich lock mode because all threads, application and async progress,
   * must first acquire the global mpich lock upon entry into the library.
   */
  fn(context, cookie);
#endif
}

#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
#define MPIDI_Send_post(__func, __req)                          \
({                                                              \
  pami_context_t context = MPIDI_Context_local(__req);          \
                                                                \
  if (likely(MPIDI_Process.perobj.context_post.active > 0))     \
    {                                                           \
      pami_result_t rc;                                         \
      rc = PAMI_Context_post(context,                           \
                             &(__req)->mpid.post_request,       \
                             __func,                            \
                             __req);                            \
      MPID_assert(rc == PAMI_SUCCESS);                          \
    }                                                           \
  else                                                          \
    {                                                           \
      PAMI_Context_lock(context);                               \
      __func(context, __req);                                   \
      PAMI_Context_unlock(context);                             \
    }                                                           \
})
#else /* (MPICH_THREAD_GRANULARITY != MPICH_THREAD_GRANULARITY_PER_OBJECT) */
#define MPIDI_Send_post(__func, __req)                          \
({                                                              \
  __func(MPIDI_Context[0], __req);                              \
})
#endif /* #if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT) */

#endif
