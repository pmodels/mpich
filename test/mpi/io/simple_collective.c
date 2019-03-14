/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


/* this deceptively simple test uncovered a bug in the way certain file systems
 * dealt with tuning parmeters.  See
 * https://github.com/open-mpi/ompi/issues/158 and
 * http://trac.mpich.org/projects/mpich/ticket/2261
 *
 * originally uncovered in Darshan:
 * http://lists.mcs.anl.gov/pipermail/darshan-users/2015-February/000256.html
 *
 * to really exercise the bug in simple_collective,
 * we'd have to run on a Lustre or Panasas file system.
 *
 * I am surprised src/mpi/romio/test/create_excl.c did not uncover the bug
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <errno.h>
#include <getopt.h>
#include "mpitest.h"
#include "test_io.h"

static char *opt_file = NULL;
static int rank = -1;

static int parse_args(int argc, char **argv);
static void usage(const char *prog);

int test_write(char *file, int nprocs, int rank, MPI_Info info)
{
    double stime, etime, wtime, w_elapsed, w_slowest, elapsed, slowest;
    MPI_File fh;
    int ret;
    char buffer[700] = { 0 };
    MPI_Status status;
    int verbose = 0;

    MPI_Barrier(MPI_COMM_WORLD);
    stime = MPI_Wtime();

    ret = MPI_File_open(MPI_COMM_WORLD, file,
                        MPI_MODE_CREATE | MPI_MODE_WRONLY | MPI_MODE_EXCL, info, &fh);

    if (ret != 0) {
        fprintf(stderr, "Error: failed to open %s\n", file);
        return 1;
    }

    etime = MPI_Wtime();

    ret = MPI_File_write_at_all(fh, rank * 700, buffer, 700, MPI_BYTE, &status);
    if (ret != 0) {
        fprintf(stderr, "Error: failed to write %s\n", file);
        return 1;
    }

    wtime = MPI_Wtime();

    MPI_File_close(&fh);

    elapsed = etime - stime;
    w_elapsed = wtime - etime;
    MPI_Reduce(&elapsed, &slowest, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&w_elapsed, &w_slowest, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        unlink(file);

        slowest *= 1000.0;
        w_slowest *= 1000.0;
        if (verbose == 1) {
            printf("file: %s, nprocs: %d, open_time: %f ms, write_time: %f ms\n",
                   file, nprocs, slowest, w_slowest);
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int nprocs;
    MPI_Info info;
    int nr_errors = 0;
    INIT_FILENAME;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    GET_TEST_FILENAME;

    MPI_Info_create(&info);
    nr_errors += test_write(filename, nprocs, rank, info);
    /* acutal value does not matter.  test only writes a small amount of data */
    MPI_Info_set(info, "striping_factor", "50");
    nr_errors += test_write(filename, nprocs, rank, info);
    MPI_Info_free(&info);

    MTest_Finalize(nr_errors);
    return MTestReturnValue(nr_errors);
}

/*
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
