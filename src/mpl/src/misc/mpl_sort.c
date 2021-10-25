/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"

/* This is a simple function to compare two integers. */
#if defined(HAVE_QSORT)
static int compare_int(const void *a, const void *b)
{
    return (*(int *) a - *(int *) b);
}
#endif

void MPL_sort_int_array(int *arr, int n)
{
#if defined(HAVE_QSORT)
    qsort(arr, n, sizeof(int), compare_int);
#else
    int i, j, value;
    for (i = 0; i < n; i++) {
        value = arr[i];
        j = i;
        while (j > 0 && arr[j - 1] > value) {
            arr[j] = arr[j - 1];
            j = j - 1;
        }
        arr[j] = value;
    }
#endif
}

void MPL_reverse_int_array(int *arr, int n)
{
    for (int i = 0; i < n / 2; i++) {
        int j = n - 1 - i;
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}
