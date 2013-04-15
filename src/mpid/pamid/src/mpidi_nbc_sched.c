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
 * \file src/mpidi_nbc_sched.c
 * \brief Non-blocking collectives hooks
 */

#include <pami.h>
#include <mpidimpl.h>

/**
 * work object for persistent advance of nbc schedules
 */
pami_work_t mpidi_nbc_work_object;

/**
 * \brief Persistent work function for nbc schedule progress
 */
pami_result_t mpidi_nbc_work_function (pami_context_t context, void *cookie)
{
  int made_progress = 0;
  MPIDU_Sched_progress (&made_progress);

  return PAMI_EAGAIN;
}

/**
 * \brief Initialize support for MPIR_* nbc implementation.
 *
 * The MPIR_* non-blocking collectives only work if the schedule is advanced.
 * This is done by posting a work function to context 0 that invokes the
 * schedule progress function.
 *
 * Because this is a persistent work function and will negatively impact the
 * performance of all other MPI operations - even when mpir non-blocking
 * collectives are not used - the work function is only posted if explicitly
 * requested.
 */
void MPIDI_NBC_init ()
{
  if (MPIDI_Process.mpir_nbc != 0)
  {
    PAMI_Context_post(MPIDI_Context[0],
                      &mpidi_nbc_work_object,
                      mpidi_nbc_work_function,
                      NULL);
  }

  return;
}
