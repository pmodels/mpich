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
    int errclass;

    MTest_Init(&argc, &argv);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Info hints;
    MPI_Info_create(&hints);
    MPI_Info_set(hints, "mpi_minimum_memory_alignment", "4096");

    const char *hintstrs[] = { "ddr", "hbm", "network", "shmem", NULL };

    for (int i = 0; hintstrs[i]; i++) {
        MPI_Info_set(hints, "mpich_buf_type", hintstrs[i]);
        for (count = 1; count < 128000; count *= 2) {
            err = MPI_Alloc_mem(count, hints, &ap);

            if (err) {
                MPI_Error_class(err, &errclass);
                if (errclass == MPI_ERR_NO_MEM) {
                    err = MPI_Alloc_mem(count, MPI_INFO_NULL, &ap);
                }
            }

            if (err) {
                errs++;
                MTestPrintError(err);
            } else {
                /* Access all of this memory */
                for (j = 0; j < count; j++) {
                    ap[j] = (char) (j & 0x7f);
                }
                MPI_Free_mem(ap);
            }
        }
    }

    MPI_Info_free(&hints);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
