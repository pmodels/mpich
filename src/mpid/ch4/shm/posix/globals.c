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

#include <mpidimpl.h>
#include "posix_impl.h"

MPIDI_POSIX_request_queue_t MPIDI_POSIX_sendq = { NULL, NULL };
MPIDI_POSIX_request_queue_t MPIDI_POSIX_recvq_posted = { NULL, NULL };
MPIDI_POSIX_request_queue_t MPIDI_POSIX_recvq_unexpected = { NULL, NULL };
MPIDI_POSIX_mem_region_t MPIDI_POSIX_mem_region = { {0}
};

char *MPIDI_POSIX_asym_base_addr = 0;
MPID_Thread_mutex_t MPID_shm_mutex;
