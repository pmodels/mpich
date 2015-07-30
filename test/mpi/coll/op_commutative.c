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
static char MTEST_Descrip[] = "A simple test of MPI_Op_create/commute/free";
*/

static int errs = 0;

/*
static void comm_user_op(void *invec, void *inoutvec, int *len, MPI_Datatype *datatype)
{
    user_op(invec, inoutvec, len, datatype);
}
*/

/*
static void noncomm_user_op(void *invec, void *inoutvec, int *len, MPI_Datatype *datatype)
{
    user_op(invec, inoutvec, len, datatype);
}
*/

static void user_op(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype)
{
    int i;
    int *invec_int = (int *) invec;
    int *inoutvec_int = (int *) inoutvec;

    if (*datatype != MPI_INT) {
        ++errs;
        printf("invalid datatype passed to user_op");
        return;
    }

    for (i = 0; i < *len; ++i) {
        inoutvec_int[i] = invec_int[i] * 2 + inoutvec_int[i];
    }
}


int main(int argc, char *argv[])
{
    MPI_Op c_uop = MPI_OP_NULL;
    MPI_Op nc_uop = MPI_OP_NULL;
    int is_commutative = 0;

    MTest_Init(&argc, &argv);

    /* make sure that user-define ops work too */
    MPI_Op_create(&user_op, 1 /*commute */ , &c_uop);
    MPI_Op_create(&user_op, 0 /*!commute */ , &nc_uop);

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    /* this function was added in MPI-2.2 */

#define CHECK_COMMUTATIVE(op_)                      \
    do {                                            \
        MPI_Op_commutative((op_), &is_commutative); \
        if (!is_commutative) { ++errs; }            \
    } while (0)

    /* Check all predefined reduction operations for commutivity.
     * This list is from section 5.9.2 of the MPI-2.1 standard */
    CHECK_COMMUTATIVE(MPI_MAX);
    CHECK_COMMUTATIVE(MPI_MIN);
    CHECK_COMMUTATIVE(MPI_SUM);
    CHECK_COMMUTATIVE(MPI_PROD);
    CHECK_COMMUTATIVE(MPI_LAND);
    CHECK_COMMUTATIVE(MPI_BAND);
    CHECK_COMMUTATIVE(MPI_LOR);
    CHECK_COMMUTATIVE(MPI_BOR);
    CHECK_COMMUTATIVE(MPI_LXOR);
    CHECK_COMMUTATIVE(MPI_BXOR);
    CHECK_COMMUTATIVE(MPI_MAXLOC);
    CHECK_COMMUTATIVE(MPI_MINLOC);

#undef CHECK_COMMUTATIVE

    MPI_Op_commutative(c_uop, &is_commutative);
    if (!is_commutative) {
        ++errs;
    }

    /* also check our non-commutative user defined operation */
    MPI_Op_commutative(nc_uop, &is_commutative);
    if (is_commutative) {
        ++errs;
    }
#endif

    MPI_Op_free(&nc_uop);
    MPI_Op_free(&c_uop);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
