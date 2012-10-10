/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>

int main()
{
    setvbuf(stdout,NULL,_IOLBF,0);  
    printf( "first line\n" );
    sleep(1);
    printf( "second line\n" );
    sleep(1); 
    printf( "last line\n" ); 
    fflush(stdout);
    return 0;
}
