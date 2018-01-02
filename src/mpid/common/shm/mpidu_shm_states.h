/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_SHM_STATES_H_INCLUDED
#define MPIDU_SHM_STATES_H_INCLUDED

#define MPID_STATE_LIST_MPIDU_SHM \
MPID_STATE_MPIDU_SHM_BARRIER, \
MPID_STATE_MPIDU_SHM_BARRIER_INIT, \
MPID_STATE_MPIDU_SHM_SEG_ALLOC, \
MPID_STATE_MPIDU_SHM_SEG_DESTROY, \
MPID_STATE_MPIDU_SHM_SEG_COMMIT, \
MPID_STATE_MPIDU_SHM_CHECK_ALLOC,

#endif /* MPIDU_SHM_STATES_H_INCLUDED */
/* vim: ft=c */
