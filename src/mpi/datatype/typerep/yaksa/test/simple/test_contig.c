/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksa.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char **argv)
{
    int errs = 0;
    yaksa_type_t type;
    uintptr_t iov_len;

    yaksa_init(NULL);

    {
        yaksa_type_create_vector(5, 2, 2, YAKSA_TYPE__INT, NULL, &type);
        yaksa_iov_len(1, type, &iov_len);
        if (iov_len != 1) {
            printf("Test vector with consecutive blocks, got iov_len = %ld, expect 1\n", iov_len);
            errs++;
        }
        yaksa_type_free(type);
    }

    {
        intptr_t blklens[] = { 2, 2, 1, 3, 3 };
        intptr_t displs[] = { 1, 3, 5, 6, 9 };
        yaksa_type_create_indexed(5, blklens, displs, YAKSA_TYPE__DOUBLE, NULL, &type);
        yaksa_iov_len(1, type, &iov_len);
        if (iov_len != 1) {
            printf("Test indexed with consecutive blocks, got iov_len = %ld, expect 1\n", iov_len);
            errs++;
        }
        yaksa_type_free(type);
    }

    {
        intptr_t blklens[] = { 2, 2, 1, 3, 3 };
        intptr_t displs[] = { 1, 2, 3, 5, 9 };
        yaksa_type_create_indexed(5, blklens, displs, YAKSA_TYPE__DOUBLE, NULL, &type);
        yaksa_iov_len(1, type, &iov_len);
        if (iov_len == 1) {
            printf("Test indexed with non-consecutive blocks, got iov_len = %ld, expect 5\n",
                   iov_len);
            errs++;
        }
        yaksa_type_free(type);
    }

    {
        intptr_t displs[] = { 2, 4, 6, 8, 10 };
        yaksa_type_create_indexed_block(5, 2, displs, YAKSA_TYPE__DOUBLE, NULL, &type);
        yaksa_iov_len(1, type, &iov_len);
        if (iov_len != 1) {
            printf("Test indexed_block with consecutive blocks, got iov_len = %ld, expect 1\n",
                   iov_len);
            errs++;
        }
        yaksa_type_free(type);
    }

    {
        intptr_t displs[] = { 2, 3, 4, 7, 10 };
        yaksa_type_create_indexed_block(5, 2, displs, YAKSA_TYPE__DOUBLE, NULL, &type);
        yaksa_iov_len(1, type, &iov_len);
        if (iov_len == 1) {
            printf("Test indexed_block with non-consecutive blocks, got iov_len = %ld, expect 5\n",
                   iov_len);
            errs++;
        }
        yaksa_type_free(type);
    }

    {
        yaksa_type_t types[] =
            { YAKSA_TYPE__INT32_T, YAKSA_TYPE__INT32_T, YAKSA_TYPE__INT8_T, YAKSA_TYPE__INT8_T,
            YAKSA_TYPE__INT16_T
        };
        intptr_t blklens[] = { 1, 1, 4, 4, 2 };
        intptr_t displs[] = { 4, 8, 12, 16, 20 };
        yaksa_type_create_struct(5, blklens, displs, types, NULL, &type);
        yaksa_iov_len(1, type, &iov_len);
        if (iov_len != 1) {
            printf("Test struct with consecutive blocks, got iov_len = %ld, expect 1\n", iov_len);
            errs++;
        }
        yaksa_type_free(type);
    }

    {
        yaksa_type_t types[] =
            { YAKSA_TYPE__INT32_T, YAKSA_TYPE__INT32_T, YAKSA_TYPE__INT8_T, YAKSA_TYPE__INT8_T,
            YAKSA_TYPE__INT16_T
        };
        intptr_t blklens[] = { 1, 1, 4, 4, 2 };
        intptr_t displs[] = { 4, 12, 13, 14, 20 };
        yaksa_type_create_struct(5, blklens, displs, types, NULL, &type);
        yaksa_iov_len(1, type, &iov_len);
        if (iov_len == 1) {
            printf("Test struct with non-consecutive blocks, got iov_len = %ld, expect 5\n",
                   iov_len);
            errs++;
        }
        yaksa_type_free(type);
    }

    yaksa_finalize();

    return errs;
}
