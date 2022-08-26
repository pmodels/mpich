#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>

/* Test case contributed by Pascal Deveze <Pascal.Deveze@bull.net>
 *
 * This test uses MPI_File_read_all to read a file with less data than requested.
 * Run with two processors.
 */

#define IO_SIZE 10
static const char *filename = "test.ord";

int main(int argc, char **argv)
{
    int errs = 0;
    MPI_File fh;
    MPI_Status status;
    unsigned char buffer[IO_SIZE];
    unsigned char cmp[IO_SIZE];

    int rank, size;
    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        printf("This test require 2 processes\n");
        goto fn_exit;
    }

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
    MPI_File_set_errhandler(fh, MPI_ERRORS_ARE_FATAL);
    for (int i = 0; i < IO_SIZE; i++) {
        buffer[i] = i;
        cmp[i] = i;
    }
    if (rank == 0) {
        MPI_File_write(fh, buffer, IO_SIZE - 5, MPI_CHAR, &status);
    }

    for (int i = 0; i < IO_SIZE; i++) {
        buffer[i] = 99;
    }
    for (int i = IO_SIZE - 5; i < IO_SIZE; i++) {
        cmp[i] = 99;
    }

    /* strictly speaking, this sync/barrier/sync closes one write access and
     * ensures subsequent read access sees latest data */
    MPI_File_sync(fh);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_File_sync(fh);

    MPI_File_seek(fh, 0, MPI_SEEK_SET);
    /* everybody reads the first IO_SIZE bytes from the file, but the file is
     * only IO_SIZE-5 bytes big.  */
    MPI_File_read_all(fh, buffer, IO_SIZE, MPI_BYTE, &status);

    int count;
    MPI_Get_count(&status, MPI_CHAR, &count);
    if (count != IO_SIZE - 5) {
        fprintf(stderr, "%d: count was %d; expected %d\n", rank, count, IO_SIZE - 5);
        errs++;
    }

    for (int i = 0; i < IO_SIZE; i++) {
        if (buffer[i] != cmp[i]) {
            fprintf(stderr, "%d: buffer[%d] = %d; expected %d\n", rank, i, buffer[i], cmp[i]);
            errs++;
        }
    }

    MPI_File_close(&fh);

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
