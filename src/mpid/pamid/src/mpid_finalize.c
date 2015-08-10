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


#if TOKEN_FLOW_CONTROL
extern void MPIDI_close_mm();
#endif

#ifdef MPIDI_STATISTICS
extern pami_extension_t pe_extension;

extern int mpidi_dynamic_tasking;
int mpidi_finalized = 0;
#ifdef DYNAMIC_TASKING
extern conn_info  *_conn_info_list;
#endif


void MPIDI_close_pe_extension() {
     extern MPIDI_printenv_t  *mpich_env;
     extern MPIX_stats_t *mpid_statp;
     int rc;
     /* PAMI_Extension_open in pami_init   */
     rc = PAMI_Extension_close (pe_extension);
     if (rc != PAMI_SUCCESS) {
         TRACE_ERR("ERROR close PAMI_Extension failed rc %d", rc);
     }
     if (mpich_env)
         MPIU_Free(mpich_env);
     if (mpid_statp)
         MPIU_Free(mpid_statp);

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
  MPIR_Errflag_t errflag=MPIR_ERR_NONE;
  MPIR_Barrier_impl(MPIR_Process.comm_world, &errflag);

#ifdef MPIDI_STATISTICS
  if (MPIDI_Process.mp_statistics) {
      MPIDI_print_statistics();
  }
  MPIDI_close_pe_extension();
#endif

#ifdef DYNAMIC_TASKING
  mpidi_finalized = 1;
  if(mpidi_dynamic_tasking) {
    /* Tell the process group code that we're done with the process groups.
       This will notify PMI (with PMI_Finalize) if necessary.  It
       also frees all PG structures, including the PG for COMM_WORLD, whose
       pointer is also saved in MPIDI_Process.my_pg */
    mpierrno = MPIDI_PG_Finalize();
    if (mpierrno) {
	TRACE_ERR("MPIDI_PG_Finalize returned with mpierrno=%d\n", mpierrno);
    }

    MPIDI_FreeParentPort();
  }
  if(_conn_info_list) 
    MPIU_Free(_conn_info_list);
  MPIDI_free_all_tranid_node();
#endif


  /* ------------------------- */
  /* shutdown request queues   */
  /* ------------------------- */
  MPIDI_Recvq_finalize();

  PAMIX_Finalize(MPIDI_Client);

#ifdef MPID_NEEDS_ICOMM_WORLD
    MPIR_Comm_release_always(MPIR_Process.icomm_world, 0);
#endif

  MPIR_Comm_release_always(MPIR_Process.comm_self,0);
  MPIR_Comm_release_always(MPIR_Process.comm_world,0);

  rc = PAMI_Context_destroyv(MPIDI_Context, MPIDI_Process.avail_contexts);
  MPID_assert_always(rc == PAMI_SUCCESS);

  rc = PAMI_Client_destroy(&MPIDI_Client);
  MPID_assert_always(rc == PAMI_SUCCESS);

#ifdef MPIDI_TRACE
 {  int i;
  for (i=0; i< MPIDI_Process.numTasks; i++) {
      if (MPIDI_Trace_buf[i].R)
          MPIU_Free(MPIDI_Trace_buf[i].R);
      if (MPIDI_Trace_buf[i].PR)
          MPIU_Free(MPIDI_Trace_buf[i].PR);
      if (MPIDI_Trace_buf[i].S)
          MPIU_Free(MPIDI_Trace_buf[i].S);
  }
 }
 MPIU_Free(MPIDI_Trace_buf);
#endif

#ifdef OUT_OF_ORDER_HANDLING
  MPIU_Free(MPIDI_In_cntr);
  MPIU_Free(MPIDI_Out_cntr);
#endif

 if (TOKEN_FLOW_CONTROL_ON)
   {
     #if TOKEN_FLOW_CONTROL
     extern char *EagerLimit;

     if (EagerLimit) MPIU_Free(EagerLimit);
     MPIU_Free(MPIDI_Token_cntr);
     MPIDI_close_mm();
     #else
     MPID_assert_always(0);
     #endif
   }

  return MPI_SUCCESS;
}
