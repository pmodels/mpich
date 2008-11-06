/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* 
 * Test performance of atomic access operations.
 * This is a *very* simple test.
 */

#include "stdio.h"
#include "unistd.h"

#define MPID_Atomic_fetch_and_incr(count_ptr_, count_old_) do {         \
        (count_old_) = 1;                                               \
        __asm__ __volatile__ ("lock ; xaddl %0,%1"                      \
                              : "=r" (count_old_), "=m" (*count_ptr_)   \
                              :  "0" (count_old_),  "m" (*count_ptr_)); \
    } while (0)

int main( int argc, char **argv )
{
    volatile int count = 0;
    int count_old;
    int failures = 0;
    int i;
    
    for (i = 0; i < 1000; i++)
    {
	MPID_Atomic_fetch_and_inc(&count, count_old);
	if (count_old != i )
	{
	    fprintf(stderr, "count_old=%d, should be %d\n", count_old, i);
	    failures++;
	}
	if (count != i + 1 )
	{
	    fprintf(stderr, "count=%d, should be %d\n", count, i + 1);
	    failures++;
	}
    }
    
    MPID_Atomic_fetch_and_inc(&count, count_old);

    if (failures == 0)
    {
	printf("No Errors\n");
    }
    else
    {
	printf("%d errors encountered\n", failures);
    }
    
    exit(0);
}
