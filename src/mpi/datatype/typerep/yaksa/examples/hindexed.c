/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <yaksa.h>
#include <assert.h>
#include "matrix_util.h"

int main()
{
    int rc;
    int input_matrix[SIZE];
    int pack_buf[SIZE];
    int unpack_buf[SIZE];
    yaksa_type_t hindexed;
    intptr_t array_of_blocklengths[ROWS - 1] = {
        1, 2, 2, 4, 4, 4, 4
    };
    intptr_t array_of_displacements[ROWS - 1] = {
        9 * sizeof(int),
        18 * sizeof(int), 26 * sizeof(int),
        36 * sizeof(int), 44 * sizeof(int), 52 * sizeof(int), 60 * sizeof(int)
    };

    yaksa_init(NULL);   /* before any yaksa API is called the library
                         * must be initialized */

    init_matrix(input_matrix, ROWS, COLS);
    set_matrix(pack_buf, ROWS, COLS, 0);
    set_matrix(unpack_buf, ROWS, COLS, 0);

    rc = yaksa_type_create_hindexed(ROWS - 1, array_of_blocklengths, array_of_displacements,
                                    YAKSA_TYPE__INT, NULL, &hindexed);
    assert(rc == YAKSA_SUCCESS);

    yaksa_request_t request;
    uintptr_t actual_pack_bytes;
    rc = yaksa_ipack(input_matrix, 1, hindexed, 0, pack_buf, 21 * sizeof(int), &actual_pack_bytes,
                     NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    uintptr_t actual_unpack_bytes;
    rc = yaksa_iunpack(pack_buf, 21 * sizeof(int), unpack_buf, 1, hindexed, 0, &actual_unpack_bytes,
                       NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    print_matrix(input_matrix, ROWS, COLS, "input=");
    print_matrix(pack_buf, ROWS, COLS, "pack_buf=");
    print_matrix(unpack_buf, ROWS, COLS, "unpack_buf=");

    yaksa_type_free(hindexed);
    yaksa_finalize();
    return 0;
}
