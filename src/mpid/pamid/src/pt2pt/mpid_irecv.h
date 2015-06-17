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
 * \file src/pt2pt/mpid_irecv.h
 * \brief ADI level implemenation of MPI_Irecv()
 */
#ifndef __src_pt2pt_mpid_irecv_h__
#define __src_pt2pt_mpid_irecv_h__

#include <mpidimpl.h>
#include "mpidi_recv.h"

#define MPID_Irecv          MPID_Irecv_inline

/**
 * \brief ADI level implemenation of MPI_Irecv()
 *
 * \param[in]  buf            The buffer to receive into
 * \param[in]  count          Number of expected elements in the buffer
 * \param[in]  datatype       The datatype of each element
 * \param[in]  rank           The sending rank
 * \param[in]  tag            The message tag
 * \param[in]  comm           Pointer to the communicator
 * \param[in]  context_offset Offset from the communicator context ID
 *
 * \param[out] request        Return a pointer to the new request object
 *
 * \returns An MPI Error code
 */
static inline int
MPID_Irecv_inline(void          * buf,
                  MPI_Aint        count,
                  MPI_Datatype    datatype,
                  int             rank,
                  int             tag,
                  MPID_Comm     * comm,
                  int             context_offset,
                  MPID_Request ** request)
{
  return MPIDI_Recv(buf,
                    count,
                    datatype,
                    rank,
                    tag,
                    comm,
                    context_offset,
                    0,
                    MPI_STATUS_IGNORE,
                    request);
}

#endif
