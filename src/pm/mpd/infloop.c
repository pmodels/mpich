/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>

int main( int argc, char *argv[] )
{
    int i, print_flag = 1;

    if (argc > 1  &&  strcmp(argv[1],"-p") == 0)
        print_flag = 0;
    for (i=0; ; i++)
    {
	if (i % 100000000 == 0)
	{
            if (print_flag)
            {
                printf("i=%d\n",i);
	        fflush(stdout);
            }
	}
    }
    return 0;
}
