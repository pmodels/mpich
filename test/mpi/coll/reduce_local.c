/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "A simple test of MPI_Reduce_local";
*/

#define MAX_BUF_ELEMENTS (65000)

static int uop_errs = 0;

/* prototype to keep the compiler happy */
static void user_op(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype);

static void user_op(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype)
{
    int i;
    int *invec_int = (int *) invec;
    int *inoutvec_int = (int *) inoutvec;

    if (*datatype != MPI_INT) {
        ++uop_errs;
        printf("invalid datatype passed to user_op");
        return;
    }

    for (i = 0; i < *len; ++i) {
        inoutvec_int[i] = invec_int[i] * 2 + inoutvec_int[i];
    }
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int i;
    int *inbuf = NULL;
    int *inoutbuf = NULL;
    int count = -1;
    MPI_Op uop = MPI_OP_NULL;

    MTest_Init(&argc, &argv);
#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    /* this function was added in MPI-2.2 */

    inbuf = malloc(sizeof(int) * MAX_BUF_ELEMENTS);
    inoutbuf = malloc(sizeof(int) * MAX_BUF_ELEMENTS);

    for (count = 0; count < MAX_BUF_ELEMENTS; count > 0 ? count *= 2 : count++) {
        for (i = 0; i < count; ++i) {
            inbuf[i] = i;
            inoutbuf[i] = i;
        }
        MPI_Reduce_local(inbuf, inoutbuf, count, MPI_INT, MPI_SUM);
        for (i = 0; i < count; ++i)
            if (inbuf[i] != i) {
                ++errs;
                if (inoutbuf[i] != (2 * i))
                    ++errs;
            }
    }

    /* make sure that user-define ops work too */
    MPI_Op_create(&user_op, 0 /*!commute */ , &uop);
    for (count = 0; count < MAX_BUF_ELEMENTS; count > 0 ? count *= 2 : count++) {
        for (i = 0; i < count; ++i) {
            inbuf[i] = i;
            inoutbuf[i] = i;
        }
        MPI_Reduce_local(inbuf, inoutbuf, count, MPI_INT, uop);
        errs += uop_errs;
        for (i = 0; i < count; ++i)
            if (inbuf[i] != i) {
                ++errs;
                if (inoutbuf[i] != (3 * i))
                    ++errs;
            }
    }
    MPI_Op_free(&uop);

    free(inbuf);
    free(inoutbuf);
#endif

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
