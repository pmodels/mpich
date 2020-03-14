/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "iqueue_impl.h"
#include "iqueue_types.h"

MPIDI_POSIX_eager_iqueue_transport_t MPIDI_POSIX_eager_iqueue_transport_global = { 0 };
