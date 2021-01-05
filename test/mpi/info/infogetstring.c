/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

#define NKEYS 3
#define CANARY 'C'
#define INVALID_KEY "invalid_key_abcdef"

int main(int argc, char *argv[])
{
    int errs = 0;
    MPI_Info info;
    char *keys[NKEYS] = { (char *) "file", (char *) "soft", (char *) "host" };
    char *values[NKEYS] = { (char *) "runfile.txt", (char *) "2:1000:4,3:1000:7",
        (char *) "myhost.myorg.org"
    };
    char value[MPI_MAX_INFO_VAL];
    int i, flag, buflen;

    MTest_Init(&argc, &argv);

    MPI_Info_create(&info);
    /* Use only named keys if the info implementation only supports
     * the predefined keys (e.g., IBM) */
    for (i = 0; i < NKEYS; i++) {
        MPI_Info_set(info, keys[i], values[i]);
    }

    /* Case 1: invalid key */
    buflen = 10;
    value[0] = CANARY;
    MPI_Info_get_string(info, INVALID_KEY, &buflen, value, &flag);
    if (flag) {
        errs++;
        printf("get_string succeeded for invalid key %s\n", INVALID_KEY);
    }
    /* It is unclear if buflen is left unchanged for an invalid key, but the
     * specification says value must be unchanged. */
    if (value[0] != CANARY) {
        errs++;
        printf("get_string with invalid key changed value\n");
    }

    /* Case 2: buflen = 0 */
    for (i = 0; i < NKEYS; i++) {
        buflen = 0;
        value[0] = CANARY;
        MPI_Info_get_string(info, keys[i], &buflen, value, &flag);
        if (!flag) {
            errs++;
            printf("get_string failed for valid key %s\n", keys[i]);
        }
        if (buflen != strlen(values[i]) + 1) {
            errs++;
            printf("get_string returned %d but buflen must be %d\n", buflen,
                   (int) (strlen(values[i]) + 1));
        }
        if (value[0] != CANARY) {
            errs++;
            printf("get_string changes value while buflen is 0\n");
        }
    }

    /* Case 3: buflen = 2 (very small number) */
    for (i = 0; i < NKEYS; i++) {
        buflen = 2;
        MPI_Info_get_string(info, keys[i], &buflen, value, &flag);
        if (!flag) {
            errs++;
            printf("get_string failed for valid key %s\n", keys[i]);
        }
        if (buflen != strlen(values[i]) + 1) {
            errs++;
            printf("get_string returned %d but buflen must be %d\n", buflen,
                   (int) (strlen(values[i]) + 1));
        }
        if (value[0] != values[i][0]) {
            errs++;
            printf("get_string returned incorrect value for key %s\n", keys[i]);
        }
        if (value[1] != '\0') {
            errs++;
            printf("get_string returned value that is not null-terminated for key %s\n", keys[i]);
        }
    }

    /* Case 4: buflen = MPI_MAX_INFO_VAL (very big number) */
    for (i = 0; i < NKEYS; i++) {
        buflen = MPI_MAX_INFO_VAL;
        MPI_Info_get_string(info, keys[i], &buflen, value, &flag);
        if (!flag) {
            errs++;
            printf("get_string failed for valid key %s\n", keys[i]);
        }
        if (buflen != strlen(values[i]) + 1) {
            errs++;
            printf("get_string returned %d but buflen must be %d\n", buflen,
                   (int) (strlen(values[i]) + 1));
        }
        if (strcmp(value, values[i])) {
            errs++;
            printf("get_string returned incorrect value for key %s\n", keys[i]);
        }
    }


    MPI_Info_free(&info);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
