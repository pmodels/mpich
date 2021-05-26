/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

/* Large data transfer */
#if defined(HUGE_COUNT)
#define BUFF_ELEMENT_COUNT(iter, count) (count)
/* Medium data transfer */
#else
#define BUFF_ELEMENT_COUNT(iter, count) (iter)
#endif /* #if defined(HUGE_COUNT) */
#define DATA_WARN_THRESHOLD 10

#define RECORD_ERROR(expr) if (expr) errs++;    /* track an error */

#define SETVAL(i, j) (i + j)

struct thread_param {
    int id;                     /* Thread id */
    int iter;
    int count;
    MPI_Comm comm;
    int tag;
    int *buff;
    int verify;                 /* Perform data verification? */
    int result;
};

MTEST_THREAD_RETURN_TYPE run_test(void *arg);   /* common interface for mt tests */

#endif /* MT_PT2PT_COMMON_INCLUDED */
