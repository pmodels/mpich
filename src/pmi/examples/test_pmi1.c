/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Simple example using PMI-v1 interface.
 *
 * To compile:
 *     cc test_pmi1.c -lpmi
 *
 * To run:
 *     mpirun -n 2 ./a.out
 */

#include <stdio.h>
#include <stdlib.h>
#include "pmi.h"

int main(int argc, char **argv)
{

    char kvsname[1024];
    int has_parent, rank, size;

    PMI_Init(&has_parent);
    PMI_Get_rank(&rank);
    PMI_Get_size(&size);
    fprintf(stdout, "    :has_parent=%d, rank=%d, size=%d\n", has_parent, rank, size);

    PMI_KVS_Get_my_name(kvsname, 1024);

    if (rank == 0) {
        /* NOTE: PMI-v1 does not allow space in both key and value */
        PMI_KVS_Put(kvsname, "key0", "got-key0");
    }

    PMI_Barrier();

    if (rank == 1) {
        char buf[100];
        int out_len;

        PMI_KVS_Get(kvsname, "key0", buf, 100);
        fprintf(stdout, "    Got key0 = [%s]\n", buf);
    }

    PMI_Finalize();
    return 0;
}
