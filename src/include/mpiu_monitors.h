/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIU_MONITORS_H_INCLUDED)
#define MPICH_MPIU_MONITORS_H_INCLUDED

#if (USE_THREAD_PKG == MPICH_THREAD_PKG_NONE)

/* FIXME: do we to define any data structures or empty routines here? */

#elif (USE_THREAD_PKG == MPICH_THREAD_PKG_POSIX)

/*
 * This monitor implementation does not guarantee fairness or lack of starvation with regards to delay/continue.  To be
 * completely fair, a queue of threads occupying the closet would need to be explicitly managed using a separate condition
 * variable for each thread.  In fact, the same can be said for the front door...
 */

#if defined(HAVE_PTHREAD_H)
#include <pthread.h>
#endif

typedef struct MPIU_Monitor
{
    pthread_mutex_t mutex;
    int front_door_unlocked;
    pthread_cond_t front_door_unlocked_cond;
}
MPIU_Monitor;

typedef struct MPIU_Monitor_closet
{
    int door_open;
    pthread_cond_t door_open_cond;
    int occupancy_count;
}
MPIU_Monitor_closet;

#define MPIU_Monitor_create(monitor_)					\
{									\
    pthread_mutex_init(&(monitor_)->mutex, NULL);			\
    (monitor_)->front_door_unlocked = TRUE;				\
    pthread_cond_init(&(monitor_)->front_door_unlocked_cond, NULL);	\
}

#define MPIU_Monitor_destroy(monitor_)					\
{									\
    pthread_mutex_destroy(&((monitor_)->mutex));			\
    (monitor_)->front_door_unlocked = FALSE;				\
    pthread_cond_destroy(&(monitor_)->front_door_unlocked_cond);	\
}

#define MPIU_Monitor_create_closet(monitor_, closet_)		\
{								\
    (closet_)->door_open = FALSE;				\
    pthread_cond_init(&(closet_)->door_open_cond, NULL);	\
    (closet_)->occupancy_count = 0;				\
}

#define MPIU_Monitor_destroy_closet(monitor_, closet_)	\
{							\
    (closet_)->door_open = FALSE;			\
    pthread_cond_destroy(&(closet_)->door_open_cond);	\
    (closet_)->occupancy_count = -1;			\
}

#define MPIU_Monitor_enter(monitor_)							\
{											\
    pthread_mutex_lock(&(monitor_)->mutex);						\
    											\
    while ((monitor_)->front_door_unlocked == FALSE);					\
    {											\
	pthread_cond_wait(&(monitor_)->front_door_unlocked_cond, &(monitor_)->mutex);	\
    }											\
}

#define MPIU_Monitor_exit(monitor_)					\
{									\
    if ((monitor_)->front_door_unlocked == FALSE)			\
    {									\
	(monitor_)->front_door_unlocked = TRUE;				\
	pthread_cond_signal(&(monitor_)->front_door_unlocked_cond);	\
    }									\
									\
    pthread_mutex_unlock(&(monitor_)->mutex);				\
}

#define MPIU_Monitor_delay(monitor_, closet_)					\
{										\
    if ((monitor_)->front_door_unlocked == FALSE)				\
    {										\
	(monitor_)->front_door_unlocked = TRUE;					\
	pthread_cond_signal(&(monitor_)->front_door_unlocked_cond);		\
    }										\
										\
    (closet_)->closet_door_open = FALSE;					\
    (closet_)->closet_occupancy_count += 1;					\
    do										\
    { 										\
	pthread_cond_wait(&(closet_)->door_open_cond, &(monitor_)->mutex);	\
    }										\
    while((closet_)->closet_door_open == FALSE);				\
    (monitor_)->closet_occupancy_count -= 1;					\
    (closet_)->closet_door_open = FALSE;					\
}

#define MPIU_Monitor_continue(monitor_, closet_)			\
{									\
    if ((closet_)->occupancy_count > 0)					\
    {									\
	(monitor_)->front_door_unlocked = FALSE;			\
	(closet_)->door_open = TRUE;					\
	pthread_cond_signal(&(closet_)->door_open_cond);		\
    }									\
    else if ((monitor_)->front_door_unlocked == FALSE)			\
    {									\
	(monitor_)->front_door_unlocked = TRUE;				\
	pthread_cond_signal(&(monitor_)->front_door_unlocked_cond);	\
    }									\
								    	\
    pthread_mutex_unlock(&(monitor_)->mutex);				\
}

#define MPIU_Monitor_get_occupancy_count(closet_) ((closet_)->occupancy_count)

#else /* USE_THREAD_PKG */
#error MPIU_Monitor not implemented for specified thread package
#endif /* USE_THREAD_PKG */



#endif /* !defined(MPICH_MPIU_MONITORS_H_INCLUDED) */
