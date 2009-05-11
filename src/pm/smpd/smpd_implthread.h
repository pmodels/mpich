/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* This header file contains utils for SMPD to access the SMPD util functions 
 *  *correctly*
 */
/* FIXME: SMPD internally calls the utils defined for the SMPD device 
 * eg: SMPDU_Sock_wait() -- Remove this dependency by copying the required
 * socket code to smpd
 */
#include "mpiimpl.h"
#include "mpiimplthread.h"

/* Macros to be used before/after calling any SMPD util funcs, 
 * eg: SMPDU_Sock_wait().
 * Since SMPD is single threaded we do not need extra functionalities
 * provided by the SMPD_CS_ENTER() macros
 */
#if !defined(MPICH_IS_THREADED)
    #define SMPD_CS_ENTER()
    #define SMPD_CS_EXIT()
#else
    #define SMPD_CS_ENTER() \
	SMPD_Thread_mutex_lock(&MPIR_ThreadInfo.global_mutex)
    #define SMPD_CS_EXIT()  \
	SMPD_Thread_mutex_unlock(&MPIR_ThreadInfo.global_mutex)
#endif
