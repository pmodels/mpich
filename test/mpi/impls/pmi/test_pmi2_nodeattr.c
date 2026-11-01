/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pmi2.h"

/* Multiple pending node-attribute requests, including different keys. */
int main(int argc, char **argv)
{
    int has_parent, rank, size, appnum;
    int rc = PMI2_Init(&has_parent, &size, &rank, &appnum);
    if (rc != PMI2_SUCCESS)
        return 1;

    if (size != 4)
        return 2;

    if (rank == 1) {
        /* Give other ranks time to issue their blocking requests. */
        sleep(1);
        rc = PMI2_Info_PutNodeAttr("pmi2_nodeattr_test_b", "ready-b");
        if (rc != PMI2_SUCCESS)
            return 3;
        rc = PMI2_Info_PutNodeAttr("pmi2_nodeattr_test_a", "ready-a");
        if (rc != PMI2_SUCCESS)
            return 4;
    } else {
        const char *key = rank == 2 ? "pmi2_nodeattr_test_b" : "pmi2_nodeattr_test_a";
        const char *expected = rank == 2 ? "ready-b" : "ready-a";
        char value[64] = { 0 };
        int found = 0;

        rc = PMI2_Info_GetNodeAttr(key, value, sizeof(value), &found, 1);
        if (rc != PMI2_SUCCESS || !found || strcmp(value, expected))
            return 5;
    }

    rc = PMI2_Finalize();
    if (rc != PMI2_SUCCESS)
        return 6;

    if (rank == 0)
        printf("No Errors\n");
    return 0;
}
