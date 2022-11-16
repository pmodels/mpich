/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
/* USE_STRICT_MPI may be defined in mpitestconf.h */
#include "mpitestconf.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* FIXME: This test only checks that the MPI_Comm_split_type routine
   doesn't fail.  It does not check for correct behavior */


static const char *split_topo[] = {
    "machine", "socket", "package", "numanode", "core", "hwthread", "pu", "l1cache",
    "l1ucache", "l1dcache", "l1icache", "l2cache", "l2ucache",
    "l2dcache", "l2icache", "l3cache", "l3ucache", "l3dcache", "l3icache", "pci:x",
    "ib", "ibx", "gpu", "gpux", "en", "enx", "eth", "ethx", "hfi", "hfix", NULL
};

static const char *split_topo_network[] = {
    "switch_level:2", "subcomm_min_size:2", "min_mem_size:512" /* Minimum memory size in bytes */ ,
    "torus_dimension:2", NULL
};

int main(int argc, char *argv[])
{
    int rank, size, verbose = 0, errs = 0, tot_errs = 0;
    int wrank, i;
    MPI_Comm comm;
    MPI_Info info;
    int world_size;

    MTest_Init(&argc, &argv);

    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    /* Check to see if MPI_COMM_TYPE_SHARED works correctly */
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &comm);
    if (comm == MPI_COMM_NULL) {
        printf("MPI_COMM_TYPE_SHARED (no hint): got MPI_COMM_NULL\n");
        errs++;
    } else {
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        if (rank == 0 && verbose)
            printf("MPI_COMM_TYPE_SHARED (no hint): Created shared subcommunicator of size %d\n",
                   size);
        MPI_Comm_free(&comm);
    }

#if MTEST_HAVE_MIN_MPI_VERSION(4,0)
    /* Check to see if MPI_COMM_TYPE_HW_GUIDED works correctly */
    for (i = 0; split_topo[i]; i++) {
        MPI_Info_create(&info);
        MPI_Info_set(info, "mpi_hw_resource_type", split_topo[i]);
        int ret;
        ret = MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_HW_GUIDED, 0, info, &comm);
        /* result will depend on platform and process bindings, just check returns */
        if (ret != MPI_SUCCESS) {
            printf("MPI_COMM_TYPE_HW_GUIDED (%s) failed\n", split_topo[i]);
            errs++;
        } else if (comm != MPI_COMM_NULL) {
            MPI_Comm_free(&comm);
        }
        MPI_Info_free(&info);
    }

    /* Test MPI_COMM_TYPE_HW_GUIDED: pass MPI_INFO_NULL, it must return MPI_COMM_NULL. */
    info = MPI_INFO_NULL;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_HW_GUIDED, 0, info, &comm);
    if (comm != MPI_COMM_NULL) {
        printf("MPI_COMM_TYPE_HW_GUIDED with MPI_INFO_NULL didn't return MPI_COMM_NULL\n");
        errs++;
        MPI_Comm_free(&comm);
    }

    /* Test MPI_COMM_TYPE_HW_GUIDED: info without correct key, it must return MPI_COMM_NULL. */
    MPI_Info_create(&info);
    /* note: shmem_topo is the wrong key for MPI_COMM_TYPE_HW_GUIDED */
    MPI_Info_set(info, "shmem_topo", split_topo[0]);
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_HW_GUIDED, 0, info, &comm);
    if (comm != MPI_COMM_NULL) {
        printf("MPI_COMM_TYPE_HW_GUIDED without correct key didn't return MPI_COMM_NULL\n");
        errs++;
        MPI_Comm_free(&comm);
    }
    MPI_Info_free(&info);

    /* Check to see if MPI_COMM_TYPE_HW_UNGUIDED works correctly */
    MPI_Info_create(&info);
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_HW_UNGUIDED, 0, info, &comm);
    if (comm != MPI_COMM_NULL) {
        int newsize;
        MPI_Comm_size(comm, &newsize);
        if (!(newsize < world_size)) {
            printf("MPI_COMM_TYPE_HW_UNGUIDED: Expected comm to be a proper sub communicator\n");
            errs++;
        }
        char resource_type[100] = "";
        int has_key = 0;
        MPI_Info_get(info, "mpi_hw_resource_type", 100, resource_type, &has_key);
        if (!has_key || strlen(resource_type) == 0) {
            printf("MPI_COMM_TYPE_HW_UNGUIDED: info for mpi_hw_resource_type not returned\n");
            errs++;
        }

        MPI_Comm_free(&comm);
    }
#endif

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
                printf("MPI_COMM_TYPE_SHARED (shmem_topo): comm size (%d) > node size (%d)\n",
                       newsize, size);
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
            printf("MPI_COMM_TYPE_SHARED (shmem_topo): comm size (%d) > node size (%d)\n",
                   newsize, size);
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
            printf("MPI_COMM_TYPE_SHARED (shmem_topo): comm size (%d) > node size (%d)\n",
                   newsize, size);
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
        printf("MPI_COMM_TYPE_SHARED (shmem_topo): invalid info value result in MPI_COMM_NULL\n");
        errs++;
    } else {
        int newsize;

        MPI_Comm_size(comm, &newsize);
        if (newsize != size) {
            printf("MPI_COMM_TYPE_SHARED (shmem_topo): comm size (%d) != node size (%d)\n",
                   size, size);
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
        printf("MPIX_COMM_TYPE_NEIGHBORHOOD: got MPI_COMM_NULL\n");
        errs++;
    } else {
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        if (rank == 0 && verbose)
            printf("MPIX_COMM_TYPE_NEIGHBORHOOD: common-file subcommunicator size = %d\n", size);
        MPI_Comm_free(&comm);
    }

    /* test #2: mismatching hints across processes */
    MPI_Info_delete(info, "nbhd_common_dirname");

    if (wrank == 1)
        MPI_Info_set(info, "nbhd_common_dirname", "__first_garbage_value__");
    else
        MPI_Info_set(info, "nbhd_common_dirname", "__second_garbage_value__");
    MPI_Comm_split_type(MPI_COMM_WORLD, MPIX_COMM_TYPE_NEIGHBORHOOD, 0, info, &comm);
    if (world_size > 1 && comm != MPI_COMM_NULL) {
        printf("MPIX_COMM_TYPE_NEIGHBORHOOD: mismatched hints got non null communicator\n");
        MPI_Comm_free(&comm);
        errs++;
    }

    /* test #3: a hint we don't know about */
    MPI_Info_delete(info, "nbhd_common_dirname");
    MPI_Info_set(info, "mpix_tooth_fairy", "enable");
    MPI_Comm_split_type(MPI_COMM_WORLD, MPIX_COMM_TYPE_NEIGHBORHOOD, 0, info, &comm);
    if (comm != MPI_COMM_NULL) {
        printf("MPIX_COMM_TYPE_NEIGHBORHOOD: bad info value didn't return NULL communicator\n");
        errs++;
        MPI_Comm_free(&comm);
    }

    MPI_Info_free(&info);
#endif

#if defined(MPIX_COMM_TYPE_NEIGHBORHOOD)
    /* Network topology tests */
    for (i = 0; split_topo_network[i]; i++) {
        MPI_Info_create(&info);
        MPI_Info_set(info, "network_topo", split_topo_network[i]);
        MPI_Comm_split_type(MPI_COMM_WORLD, MPIX_COMM_TYPE_NEIGHBORHOOD, 0, info, &comm);
        if (comm != MPI_COMM_NULL) {
            int newsize;
            MPI_Comm_size(comm, &newsize);
            if (newsize > world_size) {
                printf("MPIX_COMM_TYPE_NEIGHBORHOOD (network_topo): impossible\n");
                errs++;
            }
            MPI_Comm_free(&comm);
        }
        MPI_Info_free(&info);
    }
#endif

    /* Check to see if MPI_UNDEFINED is respected */
    MPI_Comm_split_type(MPI_COMM_WORLD, (wrank % 2 == 0) ? MPI_COMM_TYPE_SHARED : MPI_UNDEFINED, 0,
                        MPI_INFO_NULL, &comm);
    if ((wrank % 2) && (comm != MPI_COMM_NULL)) {
        printf("MPI_UNDEFINED: Expected MPI_COMM_NULL, but did not get one\n");
        errs++;
    }
    if (wrank % 2 == 0) {
        if (comm == MPI_COMM_NULL) {
            printf("MPI_UNDEFINED: Expected a non-null communicator, but got MPI_COMM_NULL\n");
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

    return MTestReturnValue(errs);
}
