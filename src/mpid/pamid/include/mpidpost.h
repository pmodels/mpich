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
#else /* ! DYNAMIC_TASKING */
/* If DYNAMIC_TASKING is not defined, PAMID does not provide its own GPID routines,
   so provide one here. **/

/* FIXME: A temporary version for lpids within my comm world */
static inline int MPID_GPID_GetAllInComm( MPID_Comm *comm_ptr, int local_size,
                                          int local_gpids[], int *singlePG )
{
    int i;
    int *gpid = local_gpids;

    for (i=0; i<comm_ptr->local_size; i++) {
        *gpid++ = 0;
        (void)MPID_VCR_Get_lpid( comm_ptr->vcr[i], gpid );
        gpid++;
    }
    *singlePG = 1;
    return 0;
}

/* FIXME: A temp for lpids within my comm world */
static inline int MPID_GPID_ToLpidArray( int size, int gpid[], int lpid[] )
{
    int i;

    for (i=0; i<size; i++) {
        lpid[i] = *++gpid;  gpid++;
    }
    return 0;
}
/* FIXME: for MPI1, all process ids are relative to MPI_COMM_WORLD.
   For MPI2, we'll need to do something more complex */
static inline int MPID_VCR_CommFromLpids( MPID_Comm *newcomm_ptr,
                                          int size, const int lpids[] )
{
    MPID_Comm *commworld_ptr;
    int i;

    commworld_ptr = MPIR_Process.comm_world;
    /* Setup the communicator's vc table: remote group */
    MPID_VCRT_Create( size, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    for (i=0; i<size; i++) {
        /* For rank i in the new communicator, find the corresponding
           rank in the comm world (FIXME FOR MPI2) */
        /* printf( "[%d] Remote rank %d has lpid %d\n",
           MPIR_Process.comm_world->rank, i, lpids[i] ); */
        if (lpids[i] < commworld_ptr->remote_size) {
            MPID_VCR_Dup( commworld_ptr->vcr[lpids[i]],
                          &newcomm_ptr->vcr[i] );
        }
        else {
            /* We must find the corresponding vcr for a given lpid */
            /* FIXME: Error */
            return 1;
            /* MPID_VCR_Dup( ???, &newcomm_ptr->vcr[i] ); */
        }
    }
    return 0;
}
#endif /* DYNAMIC_TASKING */
#endif
