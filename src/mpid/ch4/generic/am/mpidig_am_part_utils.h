/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_PART_UTILS_H_INCLUDED
#define MPIDIG_AM_PART_UTILS_H_INCLUDED

#include "ch4_impl.h"

/* Request status used for partitioned comm.
 * Updated as atomic cntr because AM callback and local operation may be
 * concurrently performed by threads. */
#define MPIDIG_PART_REQ_UNSET 0 /* Initial status set at psend|precv_init */
#define MPIDIG_PART_REQ_PARTIAL_ACTIVATED 1     /* Intermediate status (unused).
                                                 * On receiver means either (1) request matched at initial round
                                                 * or (2) locally started;
                                                 * On sender means either (1) receiver started
                                                 * or (2) locally started. */
#define MPIDIG_PART_REQ_CTS 2   /* Indicating receiver is ready to receive data
                                 * (requests matched and start called) */

/* Status to inactivate at request completion */
#define MPIDIG_PART_SREQ_INACTIVE 0     /* Sender need 2 increments to be CTS: start and remote start */
#define MPIDIG_PART_RREQ_INACTIVE 1     /* Receiver need 1 increments to be CTS: start */

/* Atomically increase partitioned rreq status and fetch state after increase.
 * Called when receiver matches request, sender receives CTS, or either side calls start. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_PART_REQ_INC_FETCH_STATUS(MPIR_Request * part_req)
{
    int prev_stat = MPL_atomic_fetch_add_int(&MPIDIG_PART_REQUEST(part_req, status), 1);
    return prev_stat + 1;
}

#endif /* MPIDIG_AM_PART_UTILS_H_INCLUDED */
