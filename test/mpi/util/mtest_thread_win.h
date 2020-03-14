/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/************************************************/
/* Inline file to be included in mtest_thread.c */
/************************************************/

int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE(*fn) (void *p), void *arg)
{
    int errs = 0;
    if (nthreads >= MTEST_MAX_THREADS) {
        fprintf(stderr, "Too many threads already created: max is %d\n", MTEST_MAX_THREADS);
        return 1;
    }
    threads[nthreads] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) fn, (LPVOID) arg, 0, NULL);
    if (threads[nthreads] == NULL) {
        return GetLastError();
    } else {
        nthreads++;
    }
    return MTestReturnValue(errs);
}

int MTest_Join_threads(void)
{
    int i, err = 0;
    for (i = 0; i < nthreads; i++) {
        if (threads[i] != INVALID_HANDLE_VALUE) {
            if (WaitForSingleObject(threads[i], INFINITE) == WAIT_FAILED) {
                err = GetLastError();
                fprintf(stderr, "Error WaitForSingleObject(), err = %d\n", err);
            } else {
                CloseHandle(threads[i]);
            }
        }
    }
    nthreads = 0;
    return err;
}

int MTest_thread_lock_create(MTEST_THREAD_LOCK_TYPE * lock)
{
    int errs = 0;
    if (lock == NULL)
        return -1;

    /* Create an unnamed uninheritable mutex */
    *lock = CreateMutex(NULL, FALSE, NULL);
    if (*lock == NULL)
        return -1;

    return MTestReturnValue(errs);
}

int MTest_thread_lock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int errs = 0;
    if (lock == NULL)
        return -1;

    /* Wait infinitely for the mutex */
    if (WaitForSingleObject(*lock, INFINITE) != WAIT_OBJECT_0) {
        return -1;
    }
    return MTestReturnValue(errs);
}

int MTest_thread_unlock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int errs = 0;
    if (lock == NULL)
        return -1;
    if (ReleaseMutex(*lock) == 0) {
        return -1;
    }
    return MTestReturnValue(errs);
}

int MTest_thread_lock_free(MTEST_THREAD_LOCK_TYPE * lock)
{
    int errs = 0;
    if (lock != NULL) {
        if (CloseHandle(*lock) == 0) {
            return -1;
        }
    }
    return MTestReturnValue(errs);
}

int MTest_thread_yield(void)
{
    return 0;
}
