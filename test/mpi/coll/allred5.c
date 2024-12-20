/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run coll_allred5
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test MPI_Allreduce with count greater than the number of processes";
*/

/* We make the error count global so that we can easily control the output
   of error information (in particular, limiting it after the first 10
   errors */
static int errs = 0;

int run(const char *arg)
{
    MPI_Comm comm;
    MPI_Datatype dtype;
    int count, *bufin, *bufout, size, i, minsize = 1;

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL) {
            continue;
        }
        MPI_Comm_size(comm, &size);
        count = size * 2;
        bufin = (int *) malloc(count * sizeof(int));
        bufout = (int *) malloc(count * sizeof(int));
        if (!bufin || !bufout) {
            fprintf(stderr, "Unable to allocated space for buffers (%d)\n", count);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        for (i = 0; i < count; i++) {
            bufin[i] = i;
            bufout[i] = -1;
        }

        dtype = MPI_INT;
        MPI_Allreduce(bufin, bufout, count, dtype, MPI_SUM, comm);
        /* Check output */
        for (i = 0; i < count; i++) {
            if (bufout[i] != i * size) {
                fprintf(stderr, "Expected bufout[%d] = %d but found %d\n", i, i * size, bufout[i]);
                errs++;
            }
        }
        free(bufin);
        free(bufout);
        MTestFreeComm(&comm);
    }

    return errs;
}
