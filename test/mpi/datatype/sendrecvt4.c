/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include "dtypes.h"


/*
   This program is derived from one in the MPICH-1 test suite

   This version sends and receives EVERYTHING from MPI_BOTTOM, by putting
   the data into a structure.
 */
int main(int argc, char **argv)
{
    MPI_Datatype *types;
    void **inbufs, **outbufs;
    int *counts, *bytesize, ntype;
    MPI_Comm comm;
    int ncomm = 20, rank, np, partner, tag, count;
    int j, k, err, toterr, world_rank, errloc;
    MPI_Status status;
    char *obuf;
    MPI_Datatype offsettype;
    int blen;
    MPI_Aint displ, extent, natural_extent;
    char myname[MPI_MAX_OBJECT_NAME];
    int mynamelen;

    MTest_Init(&argc, &argv);

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
        tag++;
        for (j = 0; j < ntype; j++) {
            MPI_Type_get_name(types[j], myname, &mynamelen);
            if (world_rank == 0)
                MTestPrintfMsg(10, "Testing type %s\n", myname);
            if (rank == 0) {
                MPI_Get_address(inbufs[j], &displ);
                blen = 1;
                MPI_Type_create_struct(1, &blen, &displ, types + j, &offsettype);
                MPI_Type_commit(&offsettype);
                /* Warning: if the type has an explicit MPI_UB, then using a
                 * simple shift of the offset won't work.  For now, we skip
                 * types whose extents are negative; the correct solution is
                 * to add, where required, an explicit MPI_UB */
                MPI_Type_extent(offsettype, &extent);
                if (extent < 0) {
                    if (world_rank == 0)
                        MTestPrintfMsg(10, "... skipping (appears to have explicit MPI_UB\n");
                    MPI_Type_free(&offsettype);
                    continue;
                }
                MPI_Type_extent(types[j], &natural_extent);
                if (natural_extent != extent) {
                    MPI_Type_free(&offsettype);
                    continue;
                }
                partner = np - 1;
                MPI_Send(MPI_BOTTOM, counts[j], offsettype, partner, tag, comm);
                MPI_Type_free(&offsettype);
            }
            else if (rank == np - 1) {
                partner = 0;
                obuf = outbufs[j];
                for (k = 0; k < bytesize[j]; k++)
                    obuf[k] = 0;
                MPI_Get_address(outbufs[j], &displ);
                blen = 1;
                MPI_Type_create_struct(1, &blen, &displ, types + j, &offsettype);
                MPI_Type_commit(&offsettype);
                /* Warning: if the type has an explicit MPI_UB, then using a
                 * simple shift of the offset won't work.  For now, we skip
                 * types whose extents are negative; the correct solution is
                 * to add, where required, an explicit MPI_UB */
                MPI_Type_extent(offsettype, &extent);
                if (extent < 0) {
                    MPI_Type_free(&offsettype);
                    continue;
                }
                MPI_Type_extent(types[j], &natural_extent);
                if (natural_extent != extent) {
                    MPI_Type_free(&offsettype);
                    continue;
                }
                MPI_Recv(MPI_BOTTOM, counts[j], offsettype, partner, tag, comm, &status);
                /* Test for correctness */
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
                    fprintf(stderr,
                            "Error in data with type %s (type %d on %d) at byte %d\n",
                            myname, j, world_rank, errloc - 1);
                    if (err < 10) {
                        /* Give details on only the first 10 errors */
                        unsigned char *in_p = (unsigned char *) inbufs[j],
                            *out_p = (unsigned char *) outbufs[j];
                        int jj;
                        jj = errloc - 1;
                        jj &= 0xfffffffc;       /* lop off a few bits */
                        in_p += jj;
                        out_p += jj;
                        fprintf(stderr, "%02x%02x%02x%02x should be %02x%02x%02x%02x\n",
                                out_p[0], out_p[1], out_p[2], out_p[3],
                                in_p[0], in_p[1], in_p[2], in_p[3]);
                    }
                    err++;
                }
                MPI_Type_free(&offsettype);
            }
        }
        MTestFreeComm(&comm);
    }

    MTestDatatype2Free(types, inbufs, outbufs, counts, bytesize, ntype);
    MTest_Finalize(err);
    MPI_Finalize();
    return MTestReturnValue(err);
}
