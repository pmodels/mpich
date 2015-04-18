/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <errno.h>
#include <pthread.h>
#include "opa_primitives.h"

/* Define lock types here */
typedef enum{
  MPIU_MUTEX,
  MPIU_TICKET,
  MPIU_PRIORITY
}MPIU_Thread_lock_impl_t;

MPIU_Thread_lock_impl_t MPIU_lock_type;

/*----------------------------*/
/* Ticket lock data structure */
/*----------------------------*/

/* Define the lock as a structure of two counters */
typedef struct ticket_lock_t{
   OPA_int_t next_ticket;
   OPA_int_t now_serving;
} ticket_lock_t;

/*------------------------------*/
/* Priority lock data structure */
/*------------------------------*/

#define HIGH_PRIORITY 1
#define LOW_PRIORITY 2

   /* We only define for now 2 levels of priority:  */
   /* The high priority requests (HPRs)             */
   /* The low priority requests (LPRs)              */
typedef struct priority_lock_t{
   OPA_int_t next_ticket_H __attribute__((aligned(64)));
   OPA_int_t now_serving_H;
   OPA_int_t next_ticket_L __attribute__((aligned(64)));
   OPA_int_t now_serving_L;
   /* In addition we include two other counters      */
   /* so high priority requests block the lower ones */
   OPA_int_t next_ticket_B __attribute__((aligned(64)));
   OPA_int_t now_serving_B;
   /* This is to allow high priority requests know   */
   /* that low priority requests are already blocked */
   /* by another high*/
   unsigned already_blocked;
   unsigned last_acquisition_priority;
} priority_lock_t;

typedef struct MPIU_Thread_mutex_t{
   pthread_mutex_t pthread_lock;
   ticket_lock_t   ticket_lock;
   priority_lock_t priority_lock;
} MPIU_Thread_mutex_t;

typedef pthread_cond_t  MPIU_Thread_cond_t;
typedef pthread_t       MPIU_Thread_id_t;
typedef pthread_key_t   MPIU_Thread_tls_t;

#define MPIU_THREAD_TLS_T_NULL 0
