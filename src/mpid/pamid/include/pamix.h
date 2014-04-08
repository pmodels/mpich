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
 * \file include/pamix.h
 * \brief Extensions to PAMI
 */


#ifndef __include_pamix_h__
#define __include_pamix_h__

#include <pami.h>
#include <mpidi_platform.h>
#if defined(__cplusplus)
extern "C" {
#endif

typedef pami_result_t (*node_info_fn)(pami_task_t  task,
                                      uint32_t    *node_id,
                                      uint32_t    *offset,
                                      uint32_t    *max_nodes);
typedef struct
{
  pami_extension_t progress;

  struct
  {
    pami_extension_t   extension;
    pami_result_t      status;
    uint8_t          * base;
    uintptr_t          stride;
    uintptr_t          bitmask;
    node_info_fn       node_info;
  } is_local_task;

#if defined(__BGQ__)
  pami_extension_t torus;
#endif
} pamix_extension_info_t;

extern pamix_extension_info_t PAMIX_Extensions;

void
PAMIX_Initialize(pami_client_t client);

void
PAMIX_Finalize(pami_client_t client);

pami_configuration_t
PAMIX_Client_query(pami_client_t         client,
                   pami_attribute_name_t name);

void
PAMIX_Dispatch_set(pami_context_t                  context[],
                   size_t                          num_contexts,
                   size_t                          dispatch,
                   pami_dispatch_callback_function fn,
                   pami_dispatch_hint_t            options,
                   size_t                        * immediate_max);

pami_task_t
PAMIX_Endpoint_query(pami_endpoint_t endpoint);


typedef void (*pamix_progress_function) (pami_context_t context, void *cookie);
#define PAMIX_CLIENT_ASYNC_GUARANTEE 1016
#define ASYNC_PROGRESS_ALL  0x1111
typedef enum
{
  PAMIX_PROGRESS_ALL            =    0,
  PAMIX_PROGRESS_RECV_INTERRUPT =    1,
  PAMIX_PROGRESS_TIMER          =    2,
  PAMIX_PROGRESS_EXT            = 1000
} pamix_progress_t;

void
PAMIX_Progress_register(pami_context_t            context,
                        pamix_progress_function   progress_fn,
                        pamix_progress_function   suspend_fn,
                        pamix_progress_function   resume_fn,
                        void                    * cookie);
void
PAMIX_Progress_enable(pami_context_t   context,
                      pamix_progress_t event_type);

void
PAMIX_Progress_disable(pami_context_t   context,
                       pamix_progress_t event_type);

pami_result_t
PAMIX_is_local_task_get_node_info(pami_task_t  task,
                                  uint32_t    *node_id,
                                  uint32_t    *offset,
                                  uint32_t    *max_nodes);

#ifdef __BGQ__

typedef struct
{
  size_t  dims;
  size_t *coord;
  size_t *size;
  size_t *torus;
} pamix_torus_info_t;

const pamix_torus_info_t * PAMIX_Torus_info();
int PAMIX_Task2torus(pami_task_t task_id, size_t coords[]);
int PAMIX_Torus2task(size_t coords[], pami_task_t* task_id);

#endif

#ifdef PAMIX_IS_LOCAL_TASK
#if defined(PAMIX_IS_LOCAL_TASK_STRIDE) && defined(PAMIX_IS_LOCAL_TASK_SHIFT)
#define PAMIX_Task_is_local(task_id)                                           \
  (((1UL << PAMIX_IS_LOCAL_TASK_SHIFT) &                                       \
    *(PAMIX_Extensions.is_local_task.base +                                    \
    task_id * PAMIX_IS_LOCAL_TASK_STRIDE)) >> PAMIX_IS_LOCAL_TASK_SHIFT)
#else
#define PAMIX_Task_is_local(task_id)                                           \
  ((PAMIX_Extensions.is_local_task.base &&                                     \
    (PAMIX_Extensions.is_local_task.bitmask &                                  \
      *(PAMIX_Extensions.is_local_task.base +                                  \
        task_id * PAMIX_Extensions.is_local_task.stride))) > 0)
#endif /* PAMIX_IS_LOCAL_TASK_STRIDE && PAMIX_IS_LOCAL_TASK_SHIFT */
#else
#define PAMIX_Task_is_local(task_id) (0)
#endif /* PAMIX_IS_LOCAL_TASK */

#if defined(__cplusplus)
}
#endif
#endif
