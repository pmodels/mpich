/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
/* USE_STRICT_MPI may be defined in mpitestconf.h */
#include "mpitestconf.h"
#include <stdio.h>
#include <stdlib.h>

/* FIXME: This test only checks that the MPI_Comm_split_type routine
   doesn't fail.  It does not check for correct behavior */


static const char *split_topo[] = {
    "machine", "socket", "package", "numanode", "core", "hwthread", "pu", "l1cache",
    "l1ucache", "l1dcache", "l1icache", "l2cache", "l2ucache",
    "l2dcache", "l2icache", "l3cache", "l3ucache", "l3dcache", "l3icache", "pci:x",
    "ib", "ibx", "gpu", "gpux", "en", "enx", "hfi", "hfix", NULL
};

int main(int argc, char *argv[])
{
    int rank, size, verbose = 0, errs = 0, tot_errs = 0;
    int wrank, i;
    MPI_Comm comm;
    MPI_Info info;

    MPI_Init(&argc, &argv);
    MTest_Init(&argc, &argv);

    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    /* Check to see if MPI_COMM_TYPE_SHARED works correctly */
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &comm);
    if (comm == MPI_COMM_NULL) {
        printf("Expected a non-null communicator, but got MPI_COMM_NULL\n");
        errs++;
    } else {
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        if (rank == 0 && verbose)
            printf("Created shared subcommunicator of size %d\n", size);
        MPI_Comm_free(&comm);
    }


    /* test for topology hints: pass a valid info value, but do not
     * expect that the MPI implementation will respect it.  */
    for (i = 0; split_topo[i]; i++) {
        MPI_Info_create(&info);
        MPI_Info_set(info, "shmem_topo", split_topo[i]);
        MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, info, &comm);
        if (comm != MPI_COMM_NULL) {
            int newsize;
            MPI_Comm_size(comm, &newsize);
            if (newsize > size) {
                printf("Expected comm size to be at most the node size\n");
                errs++;
            }
            MPI_Comm_free(&comm);
        }
        MPI_Info_free(&info);
    }

    /* test for topology hints: pass different info values from
     * different processes, the hint must be ignored then. */
    MPI_Info_create(&info);
    if (rank % 2 == 0) {
        MPI_Info_set(info, "shmem_topo", split_topo[0]);
    } else {
        MPI_Info_set(info, "shmem_topo", split_topo[1]);
    }
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, info, &comm);
    if (comm != MPI_COMM_NULL) {
        int newsize;
        MPI_Comm_size(comm, &newsize);
        if (newsize > size) {
            printf("Expected comm size to be at most the node size\n");
            errs++;
        }
        MPI_Comm_free(&comm);
    }
    MPI_Info_free(&info);


    /* test for topology hints: pass info value to some processes, but
     * not others. */
    if (rank % 2 == 0) {
        MPI_Info_create(&info);
        MPI_Info_set(info, "shmem_topo", split_topo[0]);
    } else
        info = MPI_INFO_NULL;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, info, &comm);
    if (comm != MPI_COMM_NULL) {
        int newsize;
        MPI_Comm_size(comm, &newsize);
        if (newsize > size) {
            printf("Expected comm size to be at most the node size\n");
            errs++;
        }
        MPI_Comm_free(&comm);
    }
    if (rank % 2 == 0)
        MPI_Info_free(&info);


    /* test for topology hints: pass an invalid info value and make
     * sure the behavior is as if no info key was passed.  */
    MPI_Info_create(&info);
    MPI_Info_set(info, "shmem_topo", "__garbage_value__");
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, info, &comm);
    if (comm == MPI_COMM_NULL) {
        printf("Expected a non-null communicator, but got MPI_COMM_NULL\n");
        errs++;
    } else {
        int newsize;

        MPI_Comm_size(comm, &newsize);
        if (newsize != size) {
            printf("Expected comm size to be the same as node size\n");
            errs++;
        }
        MPI_Comm_free(&comm);
    }
    MPI_Info_free(&info);


#if defined(MPIX_COMM_TYPE_NEIGHBORHOOD) && defined(HAVE_MPI_IO)
    /* the MPICH-specific MPIX_COMM_TYPE_NEIGHBORHOOD */
    /* test #1: expected behavior -- user provided a directory, and we
     * determine which processes share access to it */
    MPI_Info_create(&info);
    if (argc == 2)
        MPI_Info_set(info, "nbhd_common_dirname", argv[1]);
    else
        MPI_Info_set(info, "nbhd_common_dirname", ".");
    MPI_Comm_split_type(MPI_COMM_WORLD, MPIX_COMM_TYPE_NEIGHBORHOOD, 0, info, &comm);
    if (comm == MPI_COMM_NULL) {
        printf("Expected a non-null communicator, but got MPI_COMM_NULL\n");
        errs++;
    } else {
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        if (rank == 0 && verbose)
            printf("Correctly created common-file subcommunicator of size %d\n", size);
        MPI_Comm_free(&comm);
    }

    /* test #2: a hint we don't know about */
    MPI_Info_delete(info, "nbhd_common_dirname");
    MPI_Info_set(info, "mpix_tooth_fairy", "enable");
    MPI_Comm_split_type(MPI_COMM_WORLD, MPIX_COMM_TYPE_NEIGHBORHOOD, 0, info, &comm);
    if (comm != MPI_COMM_NULL) {
        printf("Expected a NULL communicator, but got something else\n");
        errs++;
        MPI_Comm_free(&comm);
    } else {
        if (rank == 0 && verbose)
            printf("Unknown hint correctly resulted in NULL communicator\n");
    }


    MPI_Info_free(&info);
#endif

    /* Check to see if MPI_UNDEFINED is respected */
    MPI_Comm_split_type(MPI_COMM_WORLD, (wrank % 2 == 0) ? MPI_COMM_TYPE_SHARED : MPI_UNDEFINED, 0,
                        MPI_INFO_NULL, &comm);
    if ((wrank % 2) && (comm != MPI_COMM_NULL)) {
        printf("Expected MPI_COMM_NULL, but did not get one\n");
        errs++;
    }
    if (wrank % 2 == 0) {
        if (comm == MPI_COMM_NULL) {
            printf("Expected a non-null communicator, but got MPI_COMM_NULL\n");
            errs++;
        } else {
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &size);
            if (rank == 0 && verbose)
                printf("Created shared subcommunicator of size %d\n", size);
            MPI_Comm_free(&comm);
        }
    }
    MPI_Reduce(&errs, &tot_errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
