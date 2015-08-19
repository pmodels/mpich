/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* common header includes */
#include <stdlib.h>
#include "mpichconf.h"  /* defines MPICH_THREAD_PACKAGE_NAME */
#include "mpl.h"
#include "mpiutil.h"    /* for HAS_NO_SYMBOLS_WARNING */
#include "mpiu_thread.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

/* This file currently implements these as a preprocessor if/elif/else sequence.
 * This has the upside of not doing #includes for .c files or (poorly
 * named) .i files.  It has the downside of making this file large-ish
 * and a little harder to read in some cases.  If this becomes
 * unmanagable at some point these should be separated back out into
 * header files and included as needed. [goodell@ 2009-06-24] */

/* Implementation specific function definitions (usually in the form of macros) */
#if defined(MPICH_THREAD_PACKAGE_NAME) && (MPICH_THREAD_PACKAGE_NAME == MPICH_THREAD_PACKAGE_POSIX)
/* begin posix impl */

/* stdio.h is needed for mpimem, which prototypes a few routines that
   take FILE * arguments */
#include <stdio.h>
#include "mpimem.h"

/*
 * struct MPIUI_Thread_info
 *
 * Structure used to pass the user function and data to the intermediate
 * function, MPIUI_Thread_start.  See comment in
 * MPIUI_Thread_start() header for more information.
 */
struct MPIUI_Thread_info {
    MPIU_Thread_func_t func;
    void *data;
};


void *MPIUI_Thread_start(void *arg);


/*
 * MPIU_Thread_create()
 */
void MPIU_Thread_create(MPIU_Thread_func_t func, void *data, MPIU_Thread_id_t * idp, int *errp)
{
    struct MPIUI_Thread_info *thread_info;
    int err = MPIU_THREAD_SUCCESS;

    /* FIXME: faster allocation, or avoid it all together? */
    thread_info = (struct MPIUI_Thread_info *) MPIU_Malloc(sizeof(struct MPIUI_Thread_info));
    if (thread_info != NULL) {
        pthread_attr_t attr;

        thread_info->func = func;
        thread_info->data = data;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        err = pthread_create(idp, &attr, MPIUI_Thread_start, thread_info);
        /* FIXME: convert error to an MPIU_THREAD_ERR value */

        pthread_attr_destroy(&attr);
    }
    else {
        err = 1000000000;
    }

    if (errp != NULL) {
        *errp = err;
    }
}


/*
 * MPIUI_Thread_start()
 *
 * Start functions in pthreads are expected to return a void pointer.  Since
 * our start functions do not return a value we must
 * use an intermediate function to perform call to the user's start function
 * and then return a value of NULL.
 */
void *MPIUI_Thread_start(void *arg)
{
    struct MPIUI_Thread_info *thread_info = (struct MPIUI_Thread_info *) arg;
    MPIU_Thread_func_t func = thread_info->func;
    void *data = thread_info->data;

    MPIU_Free(arg);

    func(data);

    return NULL;
}

/* end posix impl */
#endif
