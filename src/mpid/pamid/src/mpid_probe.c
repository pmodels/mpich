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
 * \file src/mpid_probe.c
 * \brief ???
 */
#include <mpidimpl.h>


int
MPID_Probe(int source,
           int tag,
           MPID_Comm * comm,
           int context_offset,
           MPI_Status * status)
{
  const int context = comm->recvcontext_id + context_offset;

  if (source == MPI_PROC_NULL)
    {
        MPIR_Status_set_procnull(status);
        return MPI_SUCCESS;
    }
  MPID_PROGRESS_WAIT_WHILE(!MPIDI_Recvq_FU_r(source, tag, context, status));
  return MPI_SUCCESS;
}
