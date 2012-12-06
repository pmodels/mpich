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
 * \file include/mpidpost.h
 * \brief The trailing device header
 *
 * This file is included after the rest of the headers
 * (mpidimpl.h, mpidpre.h, and mpiimpl.h)
 */

#ifndef __include_mpidpost_h__
#define __include_mpidpost_h__

#include <mpid_datatype.h>
#include "mpidi_prototypes.h"
#include "mpidi_macros.h"

#include "../src/mpid_progress.h"
#include "../src/mpid_request.h"
#include "../src/mpid_recvq.h"
#include "../src/pt2pt/mpid_isend.h"
#include "../src/pt2pt/mpid_send.h"
#include "../src/pt2pt/mpid_irecv.h"

#ifdef DYNAMIC_TASKING
#define MPID_ICCREATE_REMOTECOMM_HOOK(_p,_c,_np,_gp,_r) \
     MPID_PG_ForwardPGInfo(_p,_c,_np,_gp,_r)
#endif

#endif
