/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pmi.h"

#define KEY_NAME "key0"
#define KEY_VALUE "got-key0"

#define EXPECT_INT(var, expect) \
    do { \
        if (var != expect) { \
            printf("Expect " #var " = %d, got %d\n", expect, var); \
            return 1; \
        } \
    } while (0)

#define EXPECT_STR(var, expect) \
    do { \
        if (strcmp(var, expect) != 0) { \
            printf("Expect " #var " = %s, got %s\n", expect, var); \
            return 1; \
        } \
    } while (0)

#define CHECK_PMI_ERRNO(name) \
    do { \
        if (pmi_errno != PMI_SUCCESS) { \
            printf("%s returned error: %d\n", name, pmi_errno); \
            return 1; \
        } \
    } while (0)

int main(int argc, char **argv)
{
    int pmi_errno;
    char kvsname[1024];
    int has_parent, rank, size;

    pmi_errno = PMI_Init(&has_parent);
    CHECK_PMI_ERRNO("PMI_Init");

    pmi_errno = PMI_Get_rank(&rank);
    CHECK_PMI_ERRNO("PMI_Get_rank");

    pmi_errno = PMI_Get_size(&size);
    CHECK_PMI_ERRNO("PMI_Get_size");

    if (size < 2) {
        printf("This test requires 2 processes\n");
        return 1;
    }

    EXPECT_INT(has_parent, 0);

    pmi_errno = PMI_KVS_Get_my_name(kvsname, 1024);
    CHECK_PMI_ERRNO("PMI_KVS_Get_my_name");

    if (rank == 0) {
        /* NOTE: PMI-v1 does not allow space in both key and value */
        pmi_errno = PMI_KVS_Put(kvsname, KEY_NAME, KEY_VALUE);
    }

    pmi_errno = PMI_Barrier();
    CHECK_PMI_ERRNO("PMI_Barrier");

    if (rank == 1) {
        char buf[100];

        pmi_errno = PMI_KVS_Get(kvsname, KEY_NAME, buf, 100);
        CHECK_PMI_ERRNO("PMI_KVS_Get");

        EXPECT_STR(buf, KEY_VALUE);
    }

    pmi_errno = PMI_Finalize();
    CHECK_PMI_ERRNO("PMI_Finalize");

    if (rank == 0) {
        printf("No Errors\n");
    }

    return 0;
}
