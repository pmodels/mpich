/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pmi2.h"

#define CHECK_PMI2(call, exit_code) do { \
    int check_rc = (call); \
    if (check_rc != PMI2_SUCCESS) { \
        fprintf(stderr, "rank %d: %s failed (rc=%d)\n", rank, #call, check_rc); \
        return (exit_code); \
    } \
} while (0)

/* Multiple pending node-attribute requests, including different keys. */
int main(int argc, char **argv)
{
    int has_parent, rank, size, appnum;
    int rc = PMI2_Init(&has_parent, &size, &rank, &appnum);
    if (rc != PMI2_SUCCESS) {
        fprintf(stderr, "PMI2_Init failed (rc=%d)\n", rc);
        return 1;
    }

    if (size != 4) {
        fprintf(stderr, "rank %d: expected size 4, got %d\n", rank, size);
        return 2;
    }

    if (rank == 1) {
        /* Give other ranks time to issue their blocking requests. */
        sleep(1);
        CHECK_PMI2(PMI2_Info_PutNodeAttr("pmi2_nodeattr_test_b", "ready-b"), 3);
        CHECK_PMI2(PMI2_Info_PutNodeAttr("pmi2_nodeattr_test_a", "ready-a"), 4);
    } else {
        const char *key = rank == 2 ? "pmi2_nodeattr_test_b" : "pmi2_nodeattr_test_a";
        const char *expected = rank == 2 ? "ready-b" : "ready-a";
        char value[64] = { 0 };
        int found = 0;

        rc = PMI2_Info_GetNodeAttr(key, value, sizeof(value), &found, 1);
        if (rc != PMI2_SUCCESS || !found || strcmp(value, expected)) {
            fprintf(stderr, "rank %d: GetNodeAttr(%s) rc=%d found=%d value=%s expected=%s\n",
                    rank, key, rc, found, value, expected);
            return 5;
        }
    }

    CHECK_PMI2(PMI2_Finalize(), 6);

    if (rank == 0)
        printf("No Errors\n");
    return 0;
}
