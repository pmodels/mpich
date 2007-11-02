/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_EVENTS_H
#define MM_EVENTS_H

#include "mpidimpl.h"

typedef struct MM_Event
{
#ifdef WITH_WINDOWS_EVENTS
    HANDLE event;
#elif defined(WITH_PTHREAD_COND_EVENTS)
    pthread_cond_t event;
#elif defined(WITH_POLLING_EVENTS)
    int event;
#else
    int event;
#endif
} MM_Event;

MM_Event mm_create_event();
int mm_destroy_event(MM_Event event);
int mm_test_event(MM_Event event);
int mm_wait_event(MM_Event event);
int mm_wait_events(MM_Event *event_ptr, int n);

#endif
