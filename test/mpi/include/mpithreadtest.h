/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPITHREADTEST_H_INCLUDED
#define MPITHREADTEST_H_INCLUDED

#include "mpitestconf.h"

/*
   Define routines to start a thread for different thread packages.
   The routine that is started is expected to return data when it
   exits; the type of this data is

   MTEST_THREAD_RETURN_TYPE
*/

#define THREAD_PACKAGE_INVALID 0
#define THREAD_PACKAGE_NONE    1
#define THREAD_PACKAGE_POSIX   2
#define THREAD_PACKAGE_SOLARIS 3
#define THREAD_PACKAGE_WIN     4
#define THREAD_PACKAGE_UTI     5
#define THREAD_PACKAGE_ARGOBOTS 6

#if defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_WIN)
#include <windows.h>
#define MTEST_THREAD_RETURN_TYPE DWORD
#define MTEST_THREAD_HANDLE HANDLE
#define MTEST_THREAD_LOCK_TYPE HANDLE

/* FIXME: We currently assume Solaris threads and Pthreads are interoperable.
 * We need to use Solaris threads explicitly to avoid potential interoperability issues.*/
#elif defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_POSIX ||   \
                                       THREAD_PACKAGE_NAME == THREAD_PACKAGE_SOLARIS)
#include <pthread.h>
#define MTEST_THREAD_RETURN_TYPE void *
#define MTEST_THREAD_HANDLE pthread_t
#define MTEST_THREAD_LOCK_TYPE pthread_mutex_t

/* A dummy retval that is ignored */
#define MTEST_THREAD_RETVAL_IGN 0

#elif defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_ARGOBOTS)
#include <stdlib.h>
#include <abt.h>

#define MTEST_THREAD_RETURN_TYPE void
#define MTEST_THREAD_HANDLE ABT_thread
#define MTEST_THREAD_LOCK_TYPE ABT_mutex

/* Error handling */

static void MTest_abt_error(int err, const char *msg, const char *file, int line)
{
    char *err_str;
    size_t len;
    int ret;

    if (err == ABT_SUCCESS)
        return;

    ret = ABT_error_get_str(err, NULL, &len);
    if (ret != ABT_SUCCESS) {
        fprintf(stderr, "Error %d at ABT_error_get_str\n", ret);
        exit(EXIT_FAILURE);
    }
    err_str = (char *) malloc(sizeof(char) * len + 1);
    if (ret != ABT_SUCCESS) {
        fprintf(stderr, "Error %d at malloc\n", ret);
        exit(EXIT_FAILURE);
    }
    ret = ABT_error_get_str(err, err_str, NULL);
    if (ret != ABT_SUCCESS) {
        fprintf(stderr, "Error %d at ABT_error_get_str\n", ret);
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "%s (%d): %s (%s:%d)\n", err_str, err, msg, file, line);

    free(err_str);

    exit(EXIT_FAILURE);
}

#define MTEST_ABT_ERROR(e,m) MTest_abt_error(e,m,__FILE__,__LINE__)

#define MTEST_NUM_XSTREAMS 2

#define MTEST_THREAD_RETVAL_IGN

#elif defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_QTHREADS)
#include <stdlib.h>
#include <qthread/qthread.h>

#define MTEST_THREAD_RETURN_TYPE void
#define MTEST_THREAD_HANDLE ABT_thread
#define MTEST_THREAD_LOCK_TYPE ABT_mutex

/* Error handling */

static void MTest_abt_error(int err, const char *msg, const char *file, int line)
{
    char *err_str;
    size_t len;
    int ret;

    if (err == ABT_SUCCESS)
        return;

    ret = ABT_error_get_str(err, NULL, &len);
    if (ret != ABT_SUCCESS) {
        fprintf(stderr, "Error %d at ABT_error_get_str\n", ret);
        exit(EXIT_FAILURE);
    }
    err_str = (char *) malloc(sizeof(char) * len + 1);
    if (ret != ABT_SUCCESS) {
        fprintf(stderr, "Error %d at malloc\n", ret);
        exit(EXIT_FAILURE);
    }
    ret = ABT_error_get_str(err, err_str, NULL);
    if (ret != ABT_SUCCESS) {
        fprintf(stderr, "Error %d at ABT_error_get_str\n", ret);
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "%s (%d): %s (%s:%d)\n", err_str, err, msg, file, line);

    free(err_str);

    exit(EXIT_FAILURE);
}

#define MTEST_ABT_ERROR(e,m) MTest_abt_error(e,m,__FILE__,__LINE__)

#define MTEST_NUM_XSTREAMS 2

#define MTEST_THREAD_RETVAL_IGN


#else
#error "thread package (THREAD_PACKAGE_NAME) not defined or unknown"
#endif

int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE(*fn) (void *p), void *arg);
int MTest_Join_threads(void);
int MTest_thread_lock_create(MTEST_THREAD_LOCK_TYPE *);
int MTest_thread_lock(MTEST_THREAD_LOCK_TYPE *);
int MTest_thread_unlock(MTEST_THREAD_LOCK_TYPE *);
int MTest_thread_lock_free(MTEST_THREAD_LOCK_TYPE *);
int MTest_thread_barrier_init(void);
int MTest_thread_barrier(int);
int MTest_thread_barrier_free(void);
#endif /* MPITHREADTEST_H_INCLUDED */
