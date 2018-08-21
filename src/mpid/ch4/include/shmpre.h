/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHMPRE_H_INCLUDED
#define SHMPRE_H_INCLUDED

/* *INDENT-OFF* */
#include "../shm/src/shm_pre.h"
/* *INDENT-ON* */

#define MPIDI_SHM_REQUEST_DECL       MPIDI_POSIX_request_t posix;
#define MPIDI_SHM_COMM_DECL          MPIDI_POSIX_comm_t posix;
#define MPIDI_SHM_WIN_DECL           MPIDI_POSIX_win_t posix;

#endif /* SHMPRE_H_INCLUDED */
