/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2001 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mm_events.h"

MM_Event mm_create_event(void)
{
    MM_Event event;
    event.event = 0;
    return event;
}

int mm_destroy_event(MM_Event event)
{
    return MPI_SUCCESS;
}

int mm_test_event(MM_Event event)
{
    return MPI_SUCCESS;
}

int mm_wait_event(MM_Event event)
{
    return MPI_SUCCESS;
}

int mm_wait_events(MM_Event *event_ptr, int n)
{
    return MPI_SUCCESS;
}
