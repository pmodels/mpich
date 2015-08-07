/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_TIMERS_FALLBACK_H_INCLUDED)
#define MPID_TIMERS_FALLBACK_H_INCLUDED

#define MPID_Time_t         MPIU_Time_t
#define MPID_Wtime          MPIU_Wtime
#define MPID_Wtime_diff     MPIU_Wtime_diff
#define MPID_Wtime_acc      MPIU_Wtime_acc
#define MPID_Wtime_todouble MPIU_Wtime_todouble
#define MPID_Wtick          MPIU_Wtick
#define MPID_Wtime_init     MPIU_Wtime_init

#endif /* !defined(MPID_TIMERS_FALLBACK_H_INCLUDED) */
