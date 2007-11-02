/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpichconf.h"
#include <stdlib.h>
/* stdio.h is needed for mpimem, which prototypes a few routines that 
   take FILE * arguments */
#include <stdio.h>
#include "mpibase.h"
#include "mpimem.h"
#include "mpe_thread.h"

/*
 * struct MPEI_Thread_info
 *
 * Structure used to pass the user function and data to the intermediate 
 * function, MPEI_Thread_start.  See comment in
 * MPEI_Thread_start() header for more information.
 */
struct MPEI_Thread_info
{
    MPE_Thread_func_t func;
    void * data;
};


void * MPEI_Thread_start(void * arg);


/*
 * MPE_Thread_create()
 */
void MPE_Thread_create(MPE_Thread_func_t func, void * data, MPE_Thread_id_t * idp, int * errp)
{
    struct MPEI_Thread_info * thread_info;
    int err = MPE_THREAD_SUCCESS;

    /* FIXME: faster allocation, or avoid it all together? */
    thread_info = (struct MPEI_Thread_info *) MPIU_Malloc(sizeof(struct MPEI_Thread_info));
    if (thread_info != NULL)
    {
	pthread_attr_t attr;

	thread_info->func = func;
	thread_info->data = data;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	err = pthread_create(idp, &attr, MPEI_Thread_start, thread_info);
	/* FIXME: convert error to an MPE_THREAD_ERR value */
	
	pthread_attr_destroy(&attr);
    }
    else
    {
	err = 1000000000;
    }
    
    if (errp != NULL)
    {
	*errp = err;
    }
}


/*
 * MPEI_THread_start()
 *
 * Start functions in pthreads are expected to return a void pointer.  Since 
 * our start functions do not return a value we must
 * use an intermediate function to perform call to the user's start function 
 * and then return a value of NULL.
 */
void * MPEI_Thread_start(void * arg)
{
    struct MPEI_Thread_info * thread_info = (struct MPEI_Thread_info *) arg;
    MPE_Thread_func_t func = thread_info->func;
    void * data = thread_info->data;

    MPIU_Free(arg);

    func(data);
    
    return NULL;
}


