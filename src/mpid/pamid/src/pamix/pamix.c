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
 * \file src/pamix/pamix.c
 * \brief This file contains routines to make the PAMI API usable for MPI internals. It is less likely that
 *        MPI apps will use these routines.
 */

#include <assert.h>
#include <limits.h>
#include "mpidimpl.h"
#include <pamix.h>

#define PAMIX_assert_always(x) assert(x)
#if ASSERT_LEVEL==0
#define PAMIX_assert(x)
#elif ASSERT_LEVEL>=1
#define PAMIX_assert(x)        assert(x)
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifdef __BGQ__
#define __BG__
#include <spi/include/kernel/location.h>
#endif

typedef pami_result_t (*pamix_progress_register_fn) (pami_context_t            context,
                                                     pamix_progress_function   progress_fn,
                                                     pamix_progress_function   suspend_fn,
                                                     pamix_progress_function   resume_fn,
                                                     void                     * cookie);
typedef pami_result_t (*pamix_progress_enable_fn) (pami_context_t   context,
                                                   pamix_progress_t event_type);
typedef pami_result_t (*pamix_progress_disable_fn) (pami_context_t   context,
                                                    pamix_progress_t event_type);

#if defined(__BG__)
typedef const pamix_torus_info_t* (*pamix_torus_info_fn) ();
typedef pami_result_t             (*pamix_task2torus_fn) (pami_task_t, size_t[]);
typedef pami_result_t             (*pamix_torus2task_fn) (size_t[], pami_task_t *);
#endif


static struct
{
  pamix_progress_register_fn progress_register;
  pamix_progress_enable_fn   progress_enable;
  pamix_progress_disable_fn  progress_disable;

#if defined(__BG__)
  pamix_torus_info_fn torus_info;
  pamix_task2torus_fn task2torus;
  pamix_torus2task_fn torus2task;
#endif
} PAMIX_Functions = {0};

pamix_extension_info_t PAMIX_Extensions;


#define PAMI_EXTENSION_OPEN(client, name, ext)  \
({                                              \
  pami_result_t rc;                             \
  rc = PAMI_Extension_open(client, name, ext);  \
  PAMIX_assert_always(rc == PAMI_SUCCESS);      \
})
#define PAMI_EXTENSION_FUNCTION(type, name, ext)        \
({                                                      \
  void* fn;                                             \
  fn = PAMI_Extension_symbol(ext, name);                \
  (type)fn;                                             \
})
void
PAMIX_Initialize(pami_client_t client)
{
  PAMI_EXTENSION_OPEN(client, "EXT_async_progress", &PAMIX_Extensions.progress);
  PAMIX_Functions.progress_register = PAMI_EXTENSION_FUNCTION(pamix_progress_register_fn, "register", PAMIX_Extensions.progress);
  PAMIX_Functions.progress_enable   = PAMI_EXTENSION_FUNCTION(pamix_progress_enable_fn,   "enable",   PAMIX_Extensions.progress);
  PAMIX_Functions.progress_disable  = PAMI_EXTENSION_FUNCTION(pamix_progress_disable_fn,  "disable",  PAMIX_Extensions.progress);

#if defined(__BG__)
  PAMI_EXTENSION_OPEN(client, "EXT_torus_network", &PAMIX_Extensions.torus);
  PAMIX_Functions.torus_info = PAMI_EXTENSION_FUNCTION(pamix_torus_info_fn, "information", PAMIX_Extensions.torus);
  PAMIX_Functions.task2torus = PAMI_EXTENSION_FUNCTION(pamix_task2torus_fn, "task2torus",  PAMIX_Extensions.torus);
  PAMIX_Functions.torus2task = PAMI_EXTENSION_FUNCTION(pamix_torus2task_fn, "torus2task",  PAMIX_Extensions.torus);
#endif

#ifdef PAMIX_IS_LOCAL_TASK
  {
    PAMIX_Extensions.is_local_task.base      = NULL;
    PAMIX_Extensions.is_local_task.stride    = 0;
    PAMIX_Extensions.is_local_task.bitmask   = 0;
    PAMIX_Extensions.is_local_task.node_info = NULL;
    PAMIX_Extensions.is_local_task.status    = PAMI_Extension_open(client, "EXT_is_local_task",
                                                                   &PAMIX_Extensions.is_local_task.extension);

    if (PAMIX_Extensions.is_local_task.status == PAMI_SUCCESS)
      {
        PAMIX_Extensions.is_local_task.base    = PAMI_EXTENSION_FUNCTION(uint8_t *, "base",    PAMIX_Extensions.is_local_task.extension);
        PAMIX_Extensions.is_local_task.stride  = PAMI_EXTENSION_FUNCTION(uintptr_t, "stride",  PAMIX_Extensions.is_local_task.extension);
        PAMIX_Extensions.is_local_task.bitmask = PAMI_EXTENSION_FUNCTION(uintptr_t, "bitmask", PAMIX_Extensions.is_local_task.extension);
#if defined(MPID_USE_NODE_IDS)
        PAMIX_Extensions.is_local_task.node_info = PAMI_EXTENSION_FUNCTION(node_info_fn, "get_node_info", PAMIX_Extensions.is_local_task.extension);
        if (PAMIX_Extensions.is_local_task.node_info == NULL)
          {
            /* The "node information" is not available via the "is_local_task"
             * extension to pami - possibly due to a downlevel version of pami.
             * */
           PAMIX_Extensions.is_local_task.node_info = (node_info_fn) PAMIX_is_local_task_get_node_info;
          }
#endif
      }

#if defined(PAMIX_IS_LOCAL_TASK_STRIDE) && defined(PAMIX_IS_LOCAL_TASK_BITMASK)
    /*
     * If the compile-time stride and bitmask values are defined, then assert
     * that the extension was open successfully and the base pointer is valid to
     * avoid a null dereference in the PAMIX_Task_is_local macro and assert that
     * the compile-time values match the values specified by the extension.
     */
    PAMIX_assert_always(PAMIX_Extensions.is_local_task.status == PAMI_SUCCESS);
    PAMIX_assert_always(PAMIX_Extensions.is_local_task.base != NULL);
    PAMIX_assert_always(PAMIX_IS_LOCAL_TASK_STRIDE  == PAMIX_Extensions.is_local_task.stride);
    PAMIX_assert_always(PAMIX_IS_LOCAL_TASK_BITMASK == PAMIX_Extensions.is_local_task.bitmask);
#endif
  }
#endif /* PAMIX_IS_LOCAL_TASK */
}


void
PAMIX_Finalize(pami_client_t client)
{
  pami_result_t rc;
  rc = PAMI_Extension_close(PAMIX_Extensions.progress);
  PAMIX_assert_always(rc == PAMI_SUCCESS);

  if (PAMIX_Extensions.is_local_task.status == PAMI_SUCCESS)
    {
      rc = PAMI_Extension_close(PAMIX_Extensions.is_local_task.extension);
      PAMIX_assert_always(rc == PAMI_SUCCESS);

      PAMIX_Extensions.is_local_task.base    = NULL;
      PAMIX_Extensions.is_local_task.stride  = 0;
      PAMIX_Extensions.is_local_task.bitmask = 0;
      PAMIX_Extensions.is_local_task.status  = PAMI_ERROR;
    }

#if defined(__BG__)
  rc = PAMI_Extension_close(PAMIX_Extensions.torus);
  PAMIX_assert_always(rc == PAMI_SUCCESS);
#endif
}


pami_configuration_t
PAMIX_Client_query(pami_client_t         client,
                   pami_attribute_name_t name)
{
  pami_result_t rc;
  pami_configuration_t query;
  query.name = name;
  rc = PAMI_Client_query(client, &query, 1);
  PAMIX_assert_always(rc == PAMI_SUCCESS);
  return query;
}


static inline pami_configuration_t
PAMIX_Dispatch_query(pami_context_t        context,
                     size_t                dispatch,
                     pami_attribute_name_t name)
{
  pami_result_t rc;
  pami_configuration_t query;
  query.name = name;
  rc = PAMI_Dispatch_query(context, dispatch, &query, 1);
  PAMIX_assert_always(rc == PAMI_SUCCESS);
  return query;
}


void
PAMIX_Dispatch_set(pami_context_t                  context[],
                   size_t                          num_contexts,
                   size_t                          dispatch,
                   pami_dispatch_callback_function fn,
                   pami_dispatch_hint_t            options,
                   size_t                        * immediate_max)
{
  pami_result_t rc;
  size_t i;
  size_t last_immediate_max = (size_t)-1;
  for (i=0; i<num_contexts; ++i) {
    rc = PAMI_Dispatch_set(context[i],
                           dispatch,
                           fn,
                           (void*)i,
                           options);
    PAMIX_assert_always(rc == PAMI_SUCCESS);

    size_t size;
    size = PAMIX_Dispatch_query(context[i], dispatch, PAMI_DISPATCH_SEND_IMMEDIATE_MAX).value.intval;
    last_immediate_max = MIN(size, last_immediate_max);
    size = PAMIX_Dispatch_query(context[i], dispatch, PAMI_DISPATCH_RECV_IMMEDIATE_MAX).value.intval;
    last_immediate_max = MIN(size, last_immediate_max);
  }

  if (immediate_max != NULL)
    *immediate_max = last_immediate_max;
}


pami_task_t
PAMIX_Endpoint_query(pami_endpoint_t endpoint)
{
  pami_task_t rank;
  size_t      offset;

  pami_result_t rc;
  rc = PAMI_Endpoint_query(endpoint, &rank, &offset);
  PAMIX_assert(rc == PAMI_SUCCESS);

  return rank;
}


void
PAMIX_Progress_register(pami_context_t            context,
                        pamix_progress_function   progress_fn,
                        pamix_progress_function   suspend_fn,
                        pamix_progress_function   resume_fn,
                        void                    * cookie)
{
  PAMIX_assert_always(PAMIX_Functions.progress_register != NULL);
  pami_result_t rc;
  rc = PAMIX_Functions.progress_register(context, progress_fn,suspend_fn, resume_fn, cookie);
  PAMIX_assert_always(rc == PAMI_SUCCESS);
}


void
PAMIX_Progress_enable(pami_context_t   context,
                      pamix_progress_t event_type)
{
  PAMIX_assert(PAMIX_Functions.progress_enable != NULL);
  pami_result_t rc;
  rc = PAMIX_Functions.progress_enable(context, event_type);
  PAMIX_assert(rc == PAMI_SUCCESS);
}


void
PAMIX_Progress_disable(pami_context_t   context,
                       pamix_progress_t event_type)
{
  PAMIX_assert(PAMIX_Functions.progress_disable != NULL);
  pami_result_t rc;
  rc = PAMIX_Functions.progress_disable(context, event_type);
  PAMIX_assert(rc == PAMI_SUCCESS);
}


pami_result_t
PAMIX_is_local_task_get_node_info(pami_task_t  task,
                                  uint32_t    *node_id,
                                  uint32_t    *offset,
                                  uint32_t    *max_nodes)
{
#if defined(__BG__)
  size_t coords[5];
  if (PAMI_SUCCESS == PAMIX_Task2torus(task, coords))
    {
      const pamix_torus_info_t * info = PAMIX_Torus_info();

      *node_id =
        coords[0] +
        coords[1] * info->size[0] +
        coords[2] * info->size[0] * info->size[1] +
        coords[3] * info->size[0] * info->size[1] * info->size[2] +
        coords[4] * info->size[0] * info->size[1] * info->size[2] * info->size[3];

      *max_nodes = info->size[0] * info->size[1] * info->size[2] * info->size[3] * info->size[4];

      *offset = 0; /* ???????????????????????????????? */
    }
#else
  MPID_abort(); /* Other platforms should not need a fallback implementation */
#endif

  return PAMI_SUCCESS;
}

#if defined(__BG__)

const pamix_torus_info_t *
PAMIX_Torus_info()
{
  PAMIX_assert(PAMIX_Functions.torus_info != NULL);
  return PAMIX_Functions.torus_info();
}


int
PAMIX_Task2torus(pami_task_t task_id,
                 size_t      coords[])
{
  PAMIX_assert(PAMIX_Functions.task2torus != NULL);
  pami_result_t rc;
  rc = PAMIX_Functions.task2torus(task_id, coords);
  return rc;
}


#include <stdio.h>
int
PAMIX_Torus2task(size_t        coords[],
                 pami_task_t * task_id)
{
  PAMIX_assert(PAMIX_Functions.torus2task != NULL);
  pami_result_t rc;
  rc = PAMIX_Functions.torus2task(coords, task_id);
  return rc;
#if 0
  if(rc != PAMI_SUCCESS)
  {
   fprintf(stderr,"coords in: %zu %zu %zu %zu %zu, rc: %d\n", coords[0], coords[1], coords[2],coords[3],coords[4], rc);
     PAMIX_assert(rc == PAMI_SUCCESS);
   }
   else return rc;
#endif
}

#endif
