/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "GetOpt.h"
#include <stdio.h>

bool GetOpt(int *argc, LPTSTR ** argv, CLPTSTR flag)
{
    int i, j;
    if (flag == NULL)
        return false;

    for (i = 0; i < *argc; i++) {
        if (_tcsicmp((*argv)[i], flag) == 0) {
            for (j = i; j < *argc; j++) {
                (*argv)[j] = (*argv)[j + 1];
            }
            *argc -= 1;
            return true;
        }
    }
    return false;
}

bool GetOptInt(int *argc, LPTSTR ** argv, CLPTSTR flag, int *n)
{
    int i, j;
    if (flag == NULL)
        return false;

    for (i = 0; i < *argc; i++) {
        if (_tcsicmp((*argv)[i], flag) == 0) {
            if (i + 1 == *argc)
                return false;
            *n = _ttoi((*argv)[i + 1]);
            for (j = i; j < *argc - 1; j++) {
                (*argv)[j] = (*argv)[j + 2];
            }
            *argc -= 2;
            return true;
        }
    }
    return false;
}

bool GetOptLong(int *argc, LPTSTR ** argv, CLPTSTR flag, long *n)
{
    int i;
    if (GetOptInt(argc, argv, flag, &i)) {
        *n = (long) i;
        return true;
    }
    return false;
}

bool GetOptDouble(int *argc, LPTSTR ** argv, CLPTSTR flag, double *d)
{
    int i, j;

    if (flag == NULL)
        return false;

    for (i = 0; i < *argc; i++) {
        if (_tcsicmp((*argv)[i], flag) == 0) {
            if (i + 1 == *argc)
                return false;
            *d = _tcstod((*argv)[i + 1], NULL);
            for (j = i; j < *argc - 1; j++) {
                (*argv)[j] = (*argv)[j + 2];
            }
            *argc -= 2;
            return true;
        }
    }
    return false;
}

bool GetOptString(int *argc, LPTSTR ** argv, CLPTSTR flag, char *str)
{
    int i, j;

    if (flag == NULL)
        return false;

    for (i = 0; i < *argc; i++) {
        if (_tcsicmp((*argv)[i], flag) == 0) {
            if (i + 1 == *argc)
                return false;
            strcpy(str, (*argv)[i + 1]);
            for (j = i; j < *argc - 1; j++) {
                (*argv)[j] = (*argv)[j + 2];
            }
            *argc -= 2;
            return true;
        }
    }
    return false;
}
