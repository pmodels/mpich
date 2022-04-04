#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pmi2.h"

#define KEY_NAME "key0"
#define KEY_VALUE "got key0"

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
        if (pmi_errno != PMI2_SUCCESS) { \
            printf("%s returned error: %d\n", name, pmi_errno); \
            return 1; \
        } \
    } while (0)

int main(int argc, char **argv)
{
    int pmi_errno;
    char jobid[1024];
    int has_parent, rank, size, appnum;

    pmi_errno = PMI2_Init(&has_parent, &size, &rank, &appnum);
    CHECK_PMI_ERRNO("PMI2_Init");

    if (size < 2) {
        printf("This test requires 2 processes\n");
        return 1;
    }

    EXPECT_INT(has_parent, 0);
    EXPECT_INT(appnum, 0);

    pmi_errno = PMI2_Job_GetId(jobid, 1024);
    CHECK_PMI_ERRNO("PMI2_Job_GetId");

    if (rank == 0) {
        pmi_errno = PMI2_KVS_Put(KEY_NAME, KEY_VALUE);
        CHECK_PMI_ERRNO("PMI2_KVS_Put");
    }

    pmi_errno = PMI2_KVS_Fence();
    CHECK_PMI_ERRNO("PMI2_KVS_Fence");

    if (rank == 1) {
        char buf[100];
        int out_len;

        pmi_errno = PMI2_KVS_Get(jobid, 0, KEY_NAME, buf, 100, &out_len);
        CHECK_PMI_ERRNO("PMI2_KVS_Get");

        EXPECT_STR(buf, KEY_VALUE);
    }

    pmi_errno = PMI2_Finalize();
    CHECK_PMI_ERRNO("PMI2_Finalize");

    if (rank == 0) {
        printf("No Errors\n");
    }

    return 0;
}
