/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pmi.h"

/* example using PMI_Barrier_group from PMI 1.2 */

int main(int argc, char **argv)
{
    char kvsname[1024];
    int has_parent, rank, size;

    PMI_Init(&has_parent);
    PMI_Get_rank(&rank);
    PMI_Get_size(&size);
    if (size < 2) {
        if (rank == 0) {
            printf("Run this example with more than 2 processes.\n");
        }
    }

    if (PMI_Barrier_group(PMI_GROUP_SELF, 0) != PMI_SUCCESS) {
        if (rank == 0) {
            printf("PMI_Barrier_group not supported by PMI server!\n");
        }
        goto fn_exit;
    }

    PMI_KVS_Get_my_name(kvsname, 1024);

#define KEY_NAME "key0"
#define KEY_VAL  "got-key0"
    if (rank == 0) {
        /* NOTE: PMI-v1 does not allow space in both key and value */
        PMI_KVS_Put(kvsname, KEY_NAME, KEY_VAL);
    }

    if (rank == 0 || rank == 1) {
        int procs[2] = { 0, 1 };
        PMI_Barrier_group(procs, 2);
    }

    if (rank == 1) {
        char buf[100];
        PMI_KVS_Get(kvsname, KEY_NAME, buf, 100);
        if (strcmp(buf, KEY_VAL) == 0) {
            printf("No Errors\n");
        } else {
            printf("Test failed!\n");
        }
    }

  fn_exit:
    PMI_Finalize();
    return 0;
}
