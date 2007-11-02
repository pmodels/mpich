/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>

main()
{
    /* fork(); */
    while (1)
    {
        printf("going\n"); fflush(stdout);
	sleep(3);
    }
}
