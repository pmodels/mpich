/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef PORTALS4_PRE_H_INCLUDED
#define PORTALS4_PRE_H_INCLUDED

#include "portals4.h"

typedef struct {
    int handler_id;
    char *pack_buffer;
    ptl_handle_md_t md;
} MPIDI_PORTALS4_am_request_t;

typedef struct {
    int dummy;
} MPIDI_PORTALS4_request_t;

typedef struct {
    int dummy;
} MPIDI_PORTALS4_comm_t;

typedef struct {
    int dummy;
} MPIDI_PORTALS4_dt_t;

typedef struct {
    int dummy;
} MPIDI_PORTALS4_win_t;

typedef struct {
    int dummy;
} MPIDI_PORTALS4_addr_t;

typedef struct {
    int dummy;
} MPIDI_PORTALS4_op_t;

#endif /* PORTALS4_PRE_H_INCLUDED */
