/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Simple test that alloc_mem and free_mem work together";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int j, count;
    char *ap;

    MTest_Init(&argc, &argv);

    MPI_Info mem_hints;
    MPI_Info hints;
    MPI_Info_create(&mem_hints);
    hints = mem_hints;

    /* try allocating ddr memory first (default) */
    MPI_Info_set(mem_hints, "bind_memory", "ddr");

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    for (count = 1; count < 128000; count *= 2) {

        err = MPI_Alloc_mem(count, hints, &ap);
        if (err) {
            int errclass;
            /* An error of  MPI_ERR_NO_MEM is allowed */
            MPI_Error_class(err, &errclass);
            if (errclass != MPI_ERR_NO_MEM) {
                errs++;
                MTestPrintError(err);
            } else {
                /* if MPI_ERR_NO_MEM memory cannot be allocated
                 * to the requested type. Fall back to system
                 * default and try again. */
                hints = MPI_INFO_NULL;
            }

        } else {
            /* Access all of this memory */
            for (j = 0; j < count; j++) {
                ap[j] = (char) (j & 0x7f);
            }
            MPI_Free_mem(ap);
        }
    }

    hints = mem_hints;

    /* try allocating hbm memory second (non-default) */
    MPI_Info_set(mem_hints, "bind_memory", "hbm");

    for (count = 1; count < 128000; count *= 2) {

        err = MPI_Alloc_mem(count, hints, &ap);
        if (err) {
            int errclass;
            /* An error of  MPI_ERR_NO_MEM is allowed */
            MPI_Error_class(err, &errclass);
            if (errclass != MPI_ERR_NO_MEM) {
                errs++;
                MTestPrintError(err);
            } else {
                /* if MPI_ERR_NO_MEM memory cannot be allocated
                 * to the requested type. Fall back to system
                 * default and try again. */
                hints = MPI_INFO_NULL;
            }

        } else {
            /* Access all of this memory */
            for (j = 0; j < count; j++) {
                ap[j] = (char) (j & 0x7f);
            }
            MPI_Free_mem(ap);
        }
    }

    MPI_Info_free(&mem_hints);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
