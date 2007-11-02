/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef IOLOOP_H_INCLUDED
#define IOLOOP_H_INCLUDED

typedef struct {
    int fd;
    int rdwr;
    int (*handler)( int, int, void * );
    void *extra_data;
} IOHandle;

#define IO_READ  0x1
#define IO_WRITE 0x2

/* Return values for MPIE_IOLoop */
#define IOLOOP_SUCCESS 0
#define IOLOOP_TIMEOUT 0x1
#define IOLOOP_ERROR   0x2

int MPIE_IORegister( int, int, int (*)(int,int,void*), void * );
int MPIE_IODeregister( int );
int MPIE_IOLoop( int );
void TimeoutInit( int );
int  TimeoutGetRemaining( void );

#endif
