/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef _MPIDI_NEM_STATISTICS_H_
#define _MPIDI_NEM_STATISTICS_H_

#define ENABLE_STATISTICS 1
#include "mpidi_common_statistics.h"

#define ENABLE_NEM_STATISTICS 1

/* from mpid_nem_init.c */
extern uint64_t *MPID_nem_fbox_fall_back_to_queue_count;

#endif  /* _MPIDI_NEM_STATISTICS_H_ */
