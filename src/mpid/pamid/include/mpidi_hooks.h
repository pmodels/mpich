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
 * \file include/mpidi_hooks.h
 * \brief ???
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef __include_mpidi_hooks_h__
#define __include_mpidi_hooks_h__


struct MPID_VCR_t {
	pami_task_t      taskid;
#ifdef DYNAMIC_TASKING
	int              pg_rank;    /** rank in process group **/
	struct MPIDI_PG *pg;         /** process group **/
#endif
};
typedef struct MPID_VCR_t * MPID_VCR ;
typedef struct MPIDI_VCRT * MPID_VCRT;


typedef size_t              MPIDI_msg_sz_t;

#define MPID_Irsend     MPID_Isend
#define MPID_Rsend      MPID_Send
#define MPID_Rsend_init MPID_Send_init


/** \brief Our progress engine does not require state */
#define MPID_PROGRESS_STATE_DECL

/** \brief This defines the portion of MPID_Request that is specific to the Device */
#define MPID_DEV_REQUEST_DECL    struct MPIDI_Request mpid;

/** \brief This defines the portion of MPID_Comm that is specific to the Device */
#define MPID_DEV_COMM_DECL       struct MPIDI_Comm    mpid;

/** \brief This defines the portion of MPID_Win that is specific to the Device */
#define MPID_DEV_WIN_DECL        struct MPIDI_Win     mpid;

#define HAVE_DEV_COMM_HOOK
#define MPID_Dev_comm_create_hook(a)  ({ int MPIDI_Comm_create (MPID_Comm *comm); MPIDI_Comm_create (a); })
#define MPID_Dev_comm_destroy_hook(a) ({ int MPIDI_Comm_destroy(MPID_Comm *comm); MPIDI_Comm_destroy(a); })


#endif
