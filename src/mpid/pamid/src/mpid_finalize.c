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
 * \file src/mpid_finalize.c
 * \brief Normal job termination code
 */
#include <mpidimpl.h>



#ifdef MPIDI_STATISTICS
extern pami_extension_t pe_extension;


void MPIDI_close_pe_extension() {
     int rc;
     /* PAMI_Extension_open in pami_init   */
     rc = PAMI_Extension_close (pe_extension);
     if (rc != PAMI_SUCCESS) {
         TRACE_ERR("ERROR close PAMI_Extension failed rc %d", rc);
     }
}
#endif

/**
 * \brief Shut down the system
 *
 * At this time, no attempt is made to free memory being used for MPI structures.
 * \return MPI_SUCCESS
*/
int MPID_Finalize()
{
  pami_result_t rc;
  int mpierrno = MPI_SUCCESS;
  MPIR_Barrier_impl(MPIR_Process.comm_world, &mpierrno);

#ifdef MPIDI_STATISTICS
  if (MPIDI_Process.mp_statistics) {
      MPIDI_print_statistics();
  }
  MPIDI_close_pe_extension();
#endif
  /* ------------------------- */
  /* shutdown request queues   */
  /* ------------------------- */
  MPIDI_Recvq_finalize();

  PAMIX_Finalize(MPIDI_Client);

  rc = PAMI_Context_destroyv(MPIDI_Context, MPIDI_Process.avail_contexts);
  MPID_assert_always(rc == PAMI_SUCCESS);

  rc = PAMI_Client_destroy(&MPIDI_Client);
  MPID_assert_always(rc == PAMI_SUCCESS);

#ifdef MPIDI_TRACE
 {  int i;
  for (i=0; i< numTasks; i++) {
      if (MPIDI_In_cntr[i].R)
          MPIU_Free(MPIDI_In_cntr[i].R);
      if (MPIDI_In_cntr[i].PR)
          MPIU_Free(MPIDI_In_cntr[i].PR);
      if (MPIDI_Out_cntr[i].S)
          MPIU_Free(MPIDI_Out_cntr[i].S);
  }
 }
#endif

#ifdef OUT_OF_ORDER_HANDLING
  MPIU_Free(MPIDI_In_cntr);
  MPIU_Free(MPIDI_Out_cntr);
#endif

  return MPI_SUCCESS;
}
