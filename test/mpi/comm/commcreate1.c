/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

/* Check that Communicators can be created from various subsets of the
   processes in the communicator.
*/

void abortMsg(const char *, int);
int BuildComm(MPI_Comm, MPI_Group, const char[]);

void abortMsg(const char *str, int code)
{
    char msg[MPI_MAX_ERROR_STRING];
    int class, resultLen;

    MPI_Error_class(code, &class);
    MPI_Error_string(code, msg, &resultLen);
    fprintf(stderr, "%s: errcode = %d, class = %d, msg = %s\n", str, code, class, msg);
    MPI_Abort(MPI_COMM_WORLD, code);
}

int main(int argc, char *argv[])
{
    MPI_Comm dupWorld;
    int wrank, wsize, gsize, err, errs = 0;
    int ranges[1][3];
    MPI_Group wGroup, godd, ghigh, geven;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    /* Create some groups */
    MPI_Comm_group(MPI_COMM_WORLD, &wGroup);

    MTestPrintfMsg(2, "Creating groups\n");
    ranges[0][0] = 2 * (wsize / 2) - 1;
    ranges[0][1] = 1;
    ranges[0][2] = -2;
    err = MPI_Group_range_incl(wGroup, 1, ranges, &godd);
    if (err)
        abortMsg("Failed to create odd group: ", err);
    err = MPI_Group_size(godd, &gsize);
    if (err)
        abortMsg("Failed to get size of odd group: ", err);
    if (gsize != wsize / 2) {
        fprintf(stderr, "Group godd size is %d should be %d\n", gsize, wsize / 2);
        errs++;
    }

    ranges[0][0] = wsize / 2 + 1;
    ranges[0][1] = wsize - 1;
    ranges[0][2] = 1;
    err = MPI_Group_range_incl(wGroup, 1, ranges, &ghigh);
    if (err)
        abortMsg("Failed to create high group\n", err);
    ranges[0][0] = 0;
    ranges[0][1] = wsize - 1;
    ranges[0][2] = 2;
    err = MPI_Group_range_incl(wGroup, 1, ranges, &geven);
    if (err)
        abortMsg("Failed to create even group:", err);

    MPI_Comm_dup(MPI_COMM_WORLD, &dupWorld);
    MPI_Comm_set_name(dupWorld, (char *) "Dup of world");
    /* First, use the groups to create communicators from world and a dup
     * of world */
    errs += BuildComm(MPI_COMM_WORLD, ghigh, "ghigh");
    errs += BuildComm(MPI_COMM_WORLD, godd, "godd");
    errs += BuildComm(MPI_COMM_WORLD, geven, "geven");
    errs += BuildComm(dupWorld, ghigh, "ghigh");
    errs += BuildComm(dupWorld, godd, "godd");
    errs += BuildComm(dupWorld, geven, "geven");

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    /* check that we can create multiple communicators from a single collective
     * call to MPI_Comm_create as long as the groups are all disjoint */
    errs += BuildComm(MPI_COMM_WORLD, (wrank % 2 ? godd : geven), "godd+geven");
    errs += BuildComm(dupWorld, (wrank % 2 ? godd : geven), "godd+geven");
    errs += BuildComm(MPI_COMM_WORLD, MPI_GROUP_EMPTY, "MPI_GROUP_EMPTY");
    errs += BuildComm(dupWorld, MPI_GROUP_EMPTY, "MPI_GROUP_EMPTY");
#endif

    MPI_Comm_free(&dupWorld);
    MPI_Group_free(&ghigh);
    MPI_Group_free(&godd);
    MPI_Group_free(&geven);
    MPI_Group_free(&wGroup);

    MTest_Finalize(errs);

    MPI_Finalize();
    return 0;
}

int BuildComm(MPI_Comm oldcomm, MPI_Group group, const char gname[])
{
    MPI_Comm newcomm;
    int grank, gsize, rank, size, errs = 0;
    char cname[MPI_MAX_OBJECT_NAME + 1];
    int cnamelen;

    MPI_Group_rank(group, &grank);
    MPI_Group_size(group, &gsize);
    MPI_Comm_get_name(oldcomm, cname, &cnamelen);
    MTestPrintfMsg(2, "Testing comm %s from %s\n", cname, gname);
    MPI_Comm_create(oldcomm, group, &newcomm);
    if (newcomm == MPI_COMM_NULL && grank != MPI_UNDEFINED) {
        errs++;
        fprintf(stderr, "newcomm is null but process is in group\n");
    }
    if (newcomm != MPI_COMM_NULL && grank == MPI_UNDEFINED) {
        errs++;
        fprintf(stderr, "newcomm is not null but process is not in group\n");
    }
    if (newcomm != MPI_COMM_NULL && grank != MPI_UNDEFINED) {
        MPI_Comm_rank(newcomm, &rank);
        if (rank != grank) {
            errs++;
            fprintf(stderr, "Rank is %d should be %d in comm from %s\n", rank, grank, gname);
        }
        MPI_Comm_size(newcomm, &size);
        if (size != gsize) {
            errs++;
            fprintf(stderr, "Size is %d should be %d in comm from %s\n", size, gsize, gname);
        }
        MPI_Comm_free(&newcomm);
        MTestPrintfMsg(2, "Done testing comm %s from %s\n", cname, gname);
    }
    return errs;
}
