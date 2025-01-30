/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <yaksa.h>
#include <assert.h>
#include <stdlib.h>
#include "matrix_util.h"

int main()
{
    int rc;
    int input_matrix[SIZE];
    int pack_buf[SIZE];
    int unpack_buf[SIZE];
    yaksa_type_t subarray;
    int ndims = 2;
    intptr_t array_of_sizes[2] = { ROWS, COLS };
    intptr_t array_of_subsizes[2] = { 4, 4 };
    intptr_t array_of_starts[2] = { 4, 4 };
    yaksa_subarray_order_e order = YAKSA_SUBARRAY_ORDER__C;

    yaksa_init(NULL);   /* before any yaksa API is called the library
                         * must be initialized */

    init_matrix(input_matrix, ROWS, COLS);
    set_matrix(pack_buf, ROWS, COLS, 0);
    set_matrix(unpack_buf, ROWS, COLS, 0);

    rc = yaksa_type_create_subarray(ndims, array_of_sizes, array_of_subsizes,
                                    array_of_starts, order, YAKSA_TYPE__INT, NULL, &subarray);
    assert(rc == YAKSA_SUCCESS);

    yaksa_request_t request;
    uintptr_t actual_pack_bytes;
    rc = yaksa_ipack(input_matrix, 1, subarray, 0, pack_buf, 16 * sizeof(int), &actual_pack_bytes,
                     NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    uintptr_t actual_unpack_bytes;
    rc = yaksa_iunpack(pack_buf, 16 * sizeof(int), unpack_buf, 1, subarray, 0, &actual_unpack_bytes,
                       NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    print_matrix(input_matrix, 8, 8, "input_matrix=");
    print_matrix(unpack_buf, 8, 8, "unpack_buf=");

    yaksa_type_free(subarray);
    yaksa_finalize();
    return 0;
}
