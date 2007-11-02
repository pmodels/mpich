/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#define THREAD_RETURN_TYPE DWORD
int MTest_Start_thread(THREAD_RETURN_TYPE (*fn)(void *p),void *arg)
{
    HANDLE hThread;
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)fn, (LPVOID)arg, 0, NULL);
    if (hThread == NULL)
    {
	return GetLastError();
    }
    CloseHandle(hThread);
    return 0;
}
#else
#include <pthread.h>
#define THREAD_RETURN_TYPE void *
int MTest_Start_thread(THREAD_RETURN_TYPE (*fn)(void *p),void *arg)
{
    int err;
    pthread_t thread;
    /*pthread_attr_t attr;*/
    err = pthread_create(&thread, NULL/*&attr*/, fn, arg);
    return err;
}
#endif

