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
 * \file include/mpidpre.h
 * \brief The leading device header
 *
 * This file is included at the start of the other headers
 * (mpidimpl.h, mpidpost.h, and mpiimpl.h).  It generally contains
 * additions to MPI objects.
 */

#ifndef __include_mpidpre_h__
#define __include_mpidpre_h__

#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "mpid_dataloop.h"
#include <pami.h>

/* provides "pre" typedefs and such for NBC scheduling mechanism */
#include "mpid_sched_pre.h"

/** \brief Creates a compile error if the condition is false. */
#define MPID_assert_static(expr) ({ switch(0){case 0:case expr:;} })
#define MPID_assert_always(x) assert(x) /**< \brief Tests for likely problems--always active */
#define MPID_abort()          assert(0) /**< \brief Always exit--usually implies missing functionality */
#if ASSERT_LEVEL==0
#define MPID_assert(x)
#else
#define MPID_assert(x)        assert(x) /**< \brief Tests for likely problems--may not be active in performance code */
#endif


#include "mpidi_platform.h"

#include "mpidi_constants.h"
#include "mpidi_datatypes.h"
#include "mpidi_externs.h"
#include "mpidi_hooks.h"
#include "mpidi_thread.h"
#include "mpidi_util.h"

#ifndef DYNAMIC_TASKING
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

#ifdef __BGQ__
#define MPID_HANDLE_NUM_INDICES 256
#endif /* __BGQ__ */

#define MPID_MAX_SMP_BCAST_MSG_SIZE (16384)
#define MPID_MAX_SMP_REDUCE_MSG_SIZE (16384)
#define MPID_MAX_SMP_ALLREDUCE_MSG_SIZE (16384)
#ifdef MPID_DEV_DATATYPE_DECL
#error 'Conflicting definitions of MPID_DEV_DATATYPE_DECL'
#else
#define MPID_DEV_DATATYPE_DECL void *device_datatype;
#endif
#define MPID_Dev_datatype_commit_hook(ptr) MPIDI_PAMI_datatype_commit_hook(ptr)
#define MPID_Dev_datatype_destroy_hook(ptr) MPIDI_PAMI_datatype_destroy_hook(ptr)
#define MPID_Dev_datatype_dup_hook(ptr) MPIDI_PAMI_datatype_dup_hook(ptr)

#endif
