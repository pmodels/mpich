/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include <stdlib.h>
#include <assert.h>

#define GROUPSIZE 5

/* Insertion sort on array 'a' containing n elements */
static inline void isort(int a[], const int n)
{
    int i;
    int j;
    int key;

    for (j = 1; j < n; ++j) {
	key = a[j];
	i = j - 1;
	while (i >= 0 && a[i] > key) {
	    a[i+1] = a[i];
	    i -= 1;
	}
	a[i+1] = key;
    }
}


/* Partition array 'a' of size n around the value pivot and returns
 * the index of the pivot point */
static inline int partition(const int pivot, int a[], const int n)
{
    int i = 0;
    int j = n-1;
    int tmp;

    while (1) {
	while ((j > 0) && (a[j] < pivot)) --j;
	while ((i < n) && (a[i] >= pivot)) ++i;

	if (i < j) {
	    tmp = a[i];
	    a[i] = a[j];
	    a[j] = tmp;
	}
	else {
	    return j;
	}
    }
}


/* This does all of the work for k_select */
static void k_select_r(const int k, int a[], const int n, int *value)
{
    int ngroups;
    int i;
    int median_median;
    int surfeit;
    int mk;
    int *medians;

    assert (k < n);
    ngroups = (n + GROUPSIZE-1) / GROUPSIZE;
    medians = MPIU_Malloc(ngroups * sizeof (int));

    /* Divide 'a' into groups, sort the elements of each group and
       pull out the median of each group */
    for (i = 0; i < ngroups-1; ++i) {
	isort(&a[i * GROUPSIZE], GROUPSIZE);
	medians[i] = a[i*GROUPSIZE + GROUPSIZE/2];
    }
    surfeit = (n - i * GROUPSIZE);
    isort(&a[i * GROUPSIZE], surfeit);
    medians[i] = a[i*GROUPSIZE + surfeit/2];

    if (ngroups == 1) {
	*value = a[n - 1 - k];
	MPIU_Free(medians);
	return;
    }

    /* use k_select to find the median of medians */
    k_select_r((ngroups / 2) - 1 + (ngroups % 2), medians, ngroups, &median_median);
    MPIU_Free(medians);

    /* partition array a around the median of medians and return the
       index of the median of medians */
    mk = partition(median_median, a, n);

    /* if this is the last element, we're done */
    if (mk == n - 1)
	*value = a[mk];
    else if (k <= mk)
	/* call k_select to find the kth element in a[0..mk-1] (the
	   array up to the element before the median of medians) */
	k_select_r(k, &a[0], mk + 1, value);
    else
	/* call k_select to find the (k-mk)th element in a[mk+1..n] (the
	   array starting from the element after the median of medians) */
	k_select_r(k - (mk+1), &a[mk+1], n - (mk+1), value);
}


/* Returns the value of the kth smallest element in array a of size
 * n. Allocates an array and calls the recursive function k_select_r
 * which does all the work */
static int k_select(const int k, int a[], const int n, int *value)
{
    int mpi_errno = MPI_SUCCESS;
    int ngroups;

    ngroups = (n + GROUPSIZE-1) / GROUPSIZE;

    k_select_r(k - 1, a, n, value);

    return mpi_errno;
}

/* silence "no previous prototype" warnings */
int MPIU_Outlier_ratio(int * sizes, int n_sizes, double outlier_frac, double threshold);

/* Returns the ratio of the maximum size and the outlier_frac
 * percentile size. */
int MPIU_Outlier_ratio(int * sizes, int n_sizes, double outlier_frac, double threshold)
{
    int max, k_max, i;
    double retval;

    k_select(n_sizes, sizes, n_sizes, &max);
    max = sizes[0];
    for (i = 0; i < n_sizes; i++)
	if (max < sizes[i])
	    max = sizes[i];
    k_select((int) (n_sizes * outlier_frac), sizes, n_sizes, &k_max);
    retval = (double) max / k_max;

    if (retval > threshold) {
	return 1;
    }
    else {
	return 0;
    }
}
