/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* This header file contains utils for SMPD to access the MPID util functions 
 *  *correctly*
 */
/* FIXME: SMPD internally calls the utils defined for the MPID device 
 * eg: MPIDU_Sock_wait() -- Remove this dependency by copying the required
 * socket code to smpd
 */
#include "mpiimpl.h"
#include "mpiimplthread.h"

/* Macros to be used before/after calling any MPID util funcs, 
 * eg: MPIDU_Sock_wait().
 * Since SMPD is single threaded we do not need extra functionalities
 * provided by the MPID_CS_ENTER() macros
 */
#if !defined(MPICH_IS_THREADED)
    #define SMPD_CS_ENTER()
    #define SMPD_CS_EXIT()
#else
    #define SMPD_CS_ENTER() \
	MPID_Thread_mutex_lock(&MPIR_ThreadInfo.global_mutex)
    #define SMPD_CS_EXIT()  \
	MPID_Thread_mutex_unlock(&MPIR_ThreadInfo.global_mutex)
#endif
