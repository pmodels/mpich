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
 * \file src/misc/mpid_get_universe_size.c
 * \brief ???
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpidimpl.h>

#ifdef DYNAMIC_TASKING
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif  /* USE_PMI2_API */
#endif  /* DYNAMIC_TASKING */

extern int mpidi_dynamic_tasking;

/*
 * MPID_Get_universe_size - Get the universe size from the process manager
 *
 * Notes: This requires that the PMI routines are used to
 * communicate with the process manager.
 */
int MPID_Get_universe_size(int  * universe_size)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef DYNAMIC_TASKING
#ifdef USE_PMI2_API
    if(mpidi_dynamic_tasking) {
      char val[PMI2_MAX_VALLEN];
      int found = 0;
      char *endptr;

      mpi_errno = PMI2_Info_GetJobAttr("universeSize", val, sizeof(val), &found);
      TRACE_ERR("mpi_errno from PMI2_Info_GetJobAttr=%d\n", mpi_errno);

      if (!found) {
        TRACE_ERR("PMI2_Info_GetJobAttr not found\n");
	*universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
      }
      else {
        *universe_size = strtol(val, &endptr, 0);
        TRACE_ERR("PMI2_Info_GetJobAttr found universe_size=%d\n", *universe_size);
      }
#else
      int pmi_errno = PMI_SUCCESS;

      pmi_errno = PMI_Get_universe_size(universe_size);
      if (pmi_errno != PMI_SUCCESS) {
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
			     "**pmi_get_universe_size",
			     "**pmi_get_universe_size %d", pmi_errno);
      }
      if (*universe_size < 0)
      {
	*universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
      }
#endif /*USE_PMI2_API*/
   } else {
     *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
   }
#else
  *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
#endif /*DYNAMIC_TASKING*/


fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
