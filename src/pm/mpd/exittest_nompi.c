/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>

int main(int argc, char *argv[])
{
    fprintf( stderr, "Process %d is alive\n", getpid( ) );
    fprintf( stderr, "Process %d exiting with exit code %d\n", getpid( ), -getpid( ) );
    return( -getpid( ) );
}
