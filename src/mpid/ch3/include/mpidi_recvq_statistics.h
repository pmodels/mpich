/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef _MPIDI_RECVQ_STATISTICS_H_
#define _MPIDI_RECVQ_STATISTICS_H_

#define ENABLE_STATISTICS 1
#include "mpidi_common_statistics.h"

#define ENABLE_RECVQ_STATISTICS 1

extern uint64_t MPIDI_CH3I_unexpected_recvq_buffer_size;   /* from ch3u_recvq.c */

#endif  /* _MPIDI_RECVQ_STATISTICS_H_ */
