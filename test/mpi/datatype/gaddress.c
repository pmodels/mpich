/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_gaddress
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "";
*/

int run(const char *arg)
{
    int errs = 0;
    int buf[10];
    MPI_Aint a1, a2;

    MPI_Get_address(&buf[0], &a1);
    MPI_Get_address(&buf[1], &a2);

    if ((int) (a2 - a1) != sizeof(int)) {
        errs++;
        printf("Get address of two address did not return values the correct distance apart\n");
    }

    return errs;
}
