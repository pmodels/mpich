/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include <limits.h>

/*
static char MTEST_Descrip[] = "Test reading and writing ordered output";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int size, rank, i, rc;
    MPI_Datatype basic_type;
    void *buf;
    MPI_File fh;
    MPI_Comm comm;
    MPI_Status status;
    int do_large_count = 0;
    MPI_Count count, total_count;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-large-count") == 0) {
            do_large_count = 1;
#if MPI_VERSION < 4
            printf("Large count test only available with MPI_VERSION >= 4\n");
            return 1;
#endif
        }
    }
    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_File_open(comm, (char *) "test.ord",
                  MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &fh);

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    if (!do_large_count) {
        basic_type = MPI_INT;
        count = 1;
        total_count = count * size;
        buf = malloc(total_count * sizeof(int));
        *(int *) buf = rank;
        rc = MPI_File_write_ordered(fh, buf, (int) count, basic_type, &status);
    } else {
        basic_type = MPI_CHAR;
        count = (MPI_Count) INT_MAX + 100;
        total_count = count * size;
        buf = malloc(total_count * sizeof(char));
        *(char *) buf = rank;
#if MPI_VERSION >= 4
        rc = MPI_File_write_ordered_c(fh, buf, count, basic_type, &status);
#endif
    }
    if (rc) {
        MTestPrintErrorMsg("File_write_ordered", rc);
        errs++;
    }
    /* make sure all writes finish before we seek/read */
    MPI_Barrier(comm);

    /* Set the individual pointer to 0, since we want to use a read_all */
    MPI_File_seek(fh, 0, MPI_SEEK_SET);
    if (!do_large_count) {
        MPI_File_read_all(fh, buf, (int) total_count, basic_type, &status);
        int *p = buf;
        for (i = 0; i < size; i++) {
            if (p[count * i] != i) {
                errs++;
                fprintf(stderr, "%d: buf[%zd] = %d, expect %d\n", rank, (size_t) (count * i),
                        p[count * i], i);
            }
        }
    } else {
#if MPI_VERSION >= 4
        MPI_File_read_all_c(fh, buf, total_count, basic_type, &status);
#endif
        char *p = buf;
        for (i = 0; i < size; i++) {
            if (p[count * i] != i) {
                errs++;
                fprintf(stderr, "%d: buf[%zd] = %d, expect %d\n", rank, (size_t) (count * i),
                        p[count * i], i);
            }
        }
    }

    MPI_File_seek_shared(fh, 0, MPI_SEEK_SET);
    if (!do_large_count) {
        int *p = buf;
        p[0] = -1;
        MPI_File_read_ordered(fh, buf, (int) count, basic_type, &status);
        if (p[0] != rank) {
            errs++;
            fprintf(stderr, "%d: buf[0] = %d\n", rank, p[0]);
        }
    } else {
        char *p = buf;
        p[0] = -1;
#if MPI_VERSION >= 4
        MPI_File_read_ordered_c(fh, buf, count, basic_type, &status);
#endif
        if (p[0] != rank) {
            errs++;
            fprintf(stderr, "%d: buf[0] = %d\n", rank, p[0]);
        }
    }

    free(buf);
    MPI_File_close(&fh);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
