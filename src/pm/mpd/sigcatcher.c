/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void handler(int signal) {

    printf("sigcatcher:  caught signal %i\n", signal);
    fflush(stdout);
    /* if(signal == SIGTERM)
	exit(signal);
    */	
}

int main(int argc, char *argv[]){

    int i;

    for(i=1;i<32;i++)
	signal(i, handler);

    printf("sigcatcher:  ready\n");
    fflush(stdout);
    while(1)
	;
    /* sleep(1); */
}
