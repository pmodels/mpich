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
 * \file src/mpidi_mutex.c
 * \brief ???
 */
#include <mpidimpl.h>


#if    MPIDI_MUTEX_L2_ATOMIC

static inline void*
MPIDI_Mutex_initialize_l2atomics(size_t size)
{
  typedef pami_result_t (*pamix_proc_memalign_fn) (void**, size_t, size_t, const char*);

  pami_result_t rc;
  pami_extension_t l2;
  pamix_proc_memalign_fn PAMIX_L2_proc_memalign;
  void* l2atomics = NULL;

  rc = PAMI_Extension_open(NULL, "EXT_bgq_l2atomic", &l2);
  MPID_assert_always(rc == PAMI_SUCCESS);
  PAMIX_L2_proc_memalign = (pamix_proc_memalign_fn)PAMI_Extension_symbol(l2, "proc_memalign");
  MPID_assert_always(PAMIX_L2_proc_memalign != NULL);
  rc = PAMIX_L2_proc_memalign(&l2atomics, 64, size, NULL);
  MPID_assert_always(rc == PAMI_SUCCESS);
  MPID_assert_always(l2atomics != NULL);
  /* printf("MPID L2 space: virt=%p  HW=%p  L2BaseAddress=%"PRIu64"\n", l2atomics, __l2_op_ptr(l2atomics, 0), Kernel_L2AtomicsBaseAddress()); */

  return l2atomics;
}

MPIDI_Mutex_t * MPIDI_Mutex_vector;
uint32_t        MPIDI_Mutex_counter[MPIDI_MAX_THREADS][MPIDI_MAX_MUTEXES];

/**
 *  \brief Initialize a mutex.
 *
 *  In this API, mutexes are acessed via indices from
 *  0..MPIDI_MAX_MUTEXES. The mutexes are recursive
 */
int
MPIDI_Mutex_initialize()
{
  size_t i, j;

  MPIDI_Mutex_vector = (MPIDI_Mutex_t*)MPIDI_Mutex_initialize_l2atomics(sizeof (MPIDI_Mutex_t) * MPIDI_MAX_MUTEXES);

  for (i=0; i<MPIDI_MAX_MUTEXES; ++i) {
    L2_AtomicStore(&(MPIDI_Mutex_vector[i].counter), 0);
    L2_AtomicStore(&(MPIDI_Mutex_vector[i].bound),   1);
  }

  for (i=0; i<MPIDI_MAX_MUTEXES; ++i) {
    for (j=0; j<MPIDI_MAX_THREADS; ++j) {
      MPIDI_Mutex_counter[j][i] = 0;
    }
  }

  return 0;
}

#elif  MPIDI_MUTEX_LLSC

MPIDI_Mutex_t MPIDI_Mutex_vector [MPIDI_MAX_MUTEXES];
uint32_t      MPIDI_Mutex_counter[MPIDI_MAX_THREADS][MPIDI_MAX_MUTEXES];

#else

pthread_mutex_t MPIDI_Mutex_lock;

#endif
