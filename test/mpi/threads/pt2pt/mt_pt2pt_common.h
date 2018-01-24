/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef MT_PT2PT_COMMON_H_INCLUDED
#define MT_PT2PT_COMMON_H_INCLUDED

#include "mpi.h"
#include "mpitest.h"
#include "mpithreadtest.h"

#include <stdio.h>
#include <stdlib.h>

#define NTHREADS 4

/* Set blocking send function */
#if defined(BSEND)
#define SEND_FUN MPI_Bsend
#elif defined(SSEND)
#define SEND_FUN MPI_Ssend
#elif defined(RSEND)
#define SEND_FUN MPI_Rsend
#else /* default */
#define SEND_FUN MPI_Send
#endif

/* Set nonblocking send function */
#if defined(IBSEND)
#define ISEND_FUN MPI_Ibsend
#elif defined(ISSEND)
#define ISEND_FUN MPI_Issend
#elif defined(IRSEND)
#define ISEND_FUN MPI_Irsend
#else /* default */
#define ISEND_FUN MPI_Isend
#endif

/* Set iterations, message size, and buffer size */
/* Large data tranfer */
#if defined(HUGE_COUNT)
#ifdef BSEND
#define ITER 8  /* use smaller iterations for bsend to fit 'buffer size' in an int param */
#else
#define ITER 64
#endif /* #ifdef BSEND */
#define COUNT (4*1024*1024)
#define BUFF_ELEMENT_COUNT COUNT
/* Medium data transfer */
#elif defined(MEDIUM_COUNT)
#define ITER 1024
#define COUNT (4*1024)
#define BUFF_ELEMENT_COUNT COUNT
/* Small data transfer */
#else
#define ITER 2048
#define COUNT 1
#define BUFF_ELEMENT_COUNT ITER /* sends one element in each iteration */
#endif /* #if defined(HUGE_COUNT) */
#define DATA_WARN_THRESHOLD 10


#define RECORD_ERROR(expr) if (expr) errs++;    /* track an error */

struct thread_param {
    int id;                     /* Thread id */
    MPI_Comm comm;
    int tag;
    int *buff;
    int verify;                 /* Perform data verification? */
    int result;
};

MTEST_THREAD_RETURN_TYPE run_test(void *arg);   /* common interface for mt tests */

#endif /* MT_PT2PT_COMMON_INCLUDED */
