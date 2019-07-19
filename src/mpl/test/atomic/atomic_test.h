/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef ATOMIC_TEST_H_INCLUDED
#define ATOMIC_TEST_H_INCLUDED

#include "../../include/mpl_atomic.h"
#include <assert.h>
#include <pthread.h>
#include <malloc.h>
#include <limits.h>
#define TEST_ASSERT(cond) assert(cond)

#ifdef MPL_USE_LOCK_BASED_PRIMITIVES
pthread_mutex_t MPL_emulation_lock_entity = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *MPL_emulation_lock = &MPL_emulation_lock_entity;
#endif

typedef struct test_parallel_for_thread_arg_t {
    pthread_t handle;
    int num_threads, tid;
    void (*thread_f)(int, int, void *);
    void *thread_arg;
} test_parallel_for_arg_t;

void *test_parallel_for_thread_func(void *raw_arg)
{
    test_parallel_for_arg_t *arg = (test_parallel_for_arg_t *)raw_arg;
    arg->thread_f(arg->num_threads, arg->tid, arg->thread_arg);
    return NULL;
}

void test_parallel_for(int num_threads, void (*thread_f)(int num_threads,
                       int tid, void *arg), void *thread_arg)
{
    if (num_threads <= 0)
        return;
    int tid, err;
    test_parallel_for_arg_t *thread_args = (test_parallel_for_arg_t *)
        malloc(sizeof(test_parallel_for_arg_t) * (num_threads - 1));
    for (tid = 1; tid < num_threads; tid++) {
        thread_args[tid - 1].num_threads = num_threads;
        thread_args[tid - 1].tid = tid;
        thread_args[tid - 1].thread_f = thread_f;
        thread_args[tid - 1].thread_arg = thread_arg;
        err = pthread_create(&thread_args[tid - 1].handle, NULL,
                             test_parallel_for_thread_func,
                             &thread_args[tid - 1]);
        TEST_ASSERT(err == 0);
    }
    thread_f(num_threads, 0, thread_arg);
    for (tid = 1; tid < num_threads; tid++) {
        err = pthread_join(thread_args[tid - 1].handle, NULL);
        TEST_ASSERT(err == 0);
    }
    free(thread_args);
}

const int num_threads_list[] = {1, 2, 4, 8, 16, 32, 64, 128};

#endif /* ATOMIC_TEST_H_INCLUDED */
