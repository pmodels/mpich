/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <string.h>
#include "dtypes.h"

/*
   This program is derived from one in the MPICH-1 test suite.  It
   tests a wide variety of basic and derived datatypes.
 */
int main(int argc, char **argv)
{
    MPI_Datatype *types;
    void **inbufs, **outbufs;
    int *counts, *bytesize, ntype;
    MPI_Comm comm;
    int ncomm = 20, rank, np, partner, tag, count;
    int i, j, k, err, toterr, world_rank, errloc;
    MPI_Status status;
    char *obuf;
    char myname[MPI_MAX_OBJECT_NAME];
    int mynamelen;

    MTest_Init(&argc, &argv);

    /*
     * Check for -basiconly to select only the simple datatypes
     */
    for (i = 1; i < argc; i++) {
        if (!argv[i])
            break;
        if (strcmp(argv[i], "-basiconly") == 0) {
            MTestDatatype2BasicOnly();
        }
    }

    MTestDatatype2Allocate(&types, &inbufs, &outbufs, &counts, &bytesize, &ntype);
    MTestDatatype2Generate(types, inbufs, outbufs, counts, bytesize, &ntype);

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    /* Test over a wide range of datatypes and communicators */
    err = 0;
    tag = 0;
    while (MTestGetIntracomm(&comm, 2)) {
        if (comm == MPI_COMM_NULL)
            continue;
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &np);
        if (np < 2)
            continue;
        if (world_rank == 0)
            MTestPrintfMsg(10, "Testing communicator number %s\n", MTestGetIntracommName());

        tag++;
        for (j = 0; j < ntype; j++) {
            MPI_Type_get_name(types[j], myname, &mynamelen);
            if (world_rank == 0)
                MTestPrintfMsg(10, "Testing type %s\n", myname);
            if (rank == 0) {
                partner = np - 1;
                MPI_Send(inbufs[j], counts[j], types[j], partner, tag, comm);
            }
            else if (rank == np - 1) {
                partner = 0;
                obuf = outbufs[j];
                for (k = 0; k < bytesize[j]; k++)
                    obuf[k] = 0;
                MPI_Recv(outbufs[j], counts[j], types[j], partner, tag, comm, &status);
                /* Test correct */
                MPI_Get_count(&status, types[j], &count);
                if (count != counts[j]) {
                    fprintf(stderr,
                            "Error in counts (got %d expected %d) with type %s\n",
                            count, counts[j], myname);
                    err++;
                }
                if (status.MPI_SOURCE != partner) {
                    fprintf(stderr,
                            "Error in source (got %d expected %d) with type %s\n",
                            status.MPI_SOURCE, partner, myname);
                    err++;
                }
                if ((errloc = MTestDatatype2Check(inbufs[j], outbufs[j], bytesize[j]))) {
                    char *p1, *p2;
                    fprintf(stderr,
                            "Error in data with type %s (type %d on %d) at byte %d\n",
                            myname, j, world_rank, errloc - 1);
                    p1 = (char *) inbufs[j];
                    p2 = (char *) outbufs[j];
                    fprintf(stderr, "Got %x expected %x\n", p1[errloc - 1], p2[errloc - 1]);
                    err++;
                }
            }
        }
        MTestFreeComm(&comm);
    }

    MTestDatatype2Free(types, inbufs, outbufs, counts, bytesize, ntype);
    MTest_Finalize(err);
    MPI_Finalize();
    return MTestReturnValue(err);
}
