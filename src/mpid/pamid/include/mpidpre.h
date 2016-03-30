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

#include "mpiimpl.h"
#include "mpid_datatype_fallback.h"
#include <pami.h>

/* provides "pre" typedefs and such for NBC scheduling mechanism */
#include "mpidu_pre.h"

/** \brief Creates a compile error if the condition is false. */
#define MPID_assert_static(expr) ({ switch(0){case 0:case expr:;} })
#define MPID_assert_always(x) assert(x) /**< \brief Tests for likely problems--always active */
#define MPID_abort()          assert(0) /**< \brief Always exit--usually implies missing functionality */
#if ASSERT_LEVEL==0
#define MPID_assert(x)
#else
#define MPID_assert(x)        assert(x) /**< \brief Tests for likely problems--may not be active in performance code */
#endif

/* The timer code is allowed to return "NOT_INITIALIZED" before the
 * device is initialized.  Once the device is initialized, it must
 * always return SUCCESS, so the upper layers do not need to check for
 * the return code.  */
#define MPID_TIMER_SUCCESS              MPL_TIMER_SUCCESS
#define MPID_TIMER_ERR_NOT_INITIALIZED  MPL_TIMER_ERR_NOT_INITIALIZED

typedef double MPID_Time_t;

int MPID_Wtime(MPID_Time_t *tval);
int MPID_Wtick(double *wtick);
int MPID_Wtime_diff(MPID_Time_t *t1, MPID_Time_t *t2, double *diff);
int MPID_Wtime_todouble(MPID_Time_t *t, double *val);
int MPID_Wtime_acc(MPID_Time_t *t1, MPID_Time_t *t2, MPID_Time_t *t3);
int MPID_Wtime_init(void);

#include "mpidi_platform.h"

#include "mpidi_constants.h"
#include "mpidi_datatypes.h"
#include "mpidi_externs.h"
#include "mpidi_hooks.h"
#include "mpidi_thread.h"
#include "mpidi_util.h"

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
#define MPIDU_Dev_datatype_commit_hook(ptr) MPIDI_PAMI_datatype_commit_hook(ptr)
#define MPIDU_Dev_datatype_destroy_hook(ptr) MPIDI_PAMI_datatype_destroy_hook(ptr)
#define MPIDU_Dev_datatype_dup_hook(ptr) MPIDI_PAMI_datatype_dup_hook(ptr)

#endif
