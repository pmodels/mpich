/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <yaksa.h>
#include <assert.h>
#include <stdlib.h>
#include "matrix_util.h"

#define BLKLEN (4)

int main()
{
    int rc;
    int input_matrix[SIZE];
    int pack_buf[SIZE];
    int unpack_buf[SIZE];
    yaksa_type_t indexed_block;
    intptr_t array_of_displacements[ROWS] = {
        4, 12, 20, 28,
        32, 40, 48, 56
    };

    yaksa_init(NULL);   /* before any yaksa API is called the library
                         * must be initialized */

    init_matrix(input_matrix, ROWS, COLS);
    set_matrix(pack_buf, ROWS, COLS, 0);
    set_matrix(unpack_buf, ROWS, COLS, 0);

    rc = yaksa_type_create_indexed_block(ROWS, BLKLEN, array_of_displacements, YAKSA_TYPE__INT,
                                         NULL, &indexed_block);
    assert(rc == YAKSA_SUCCESS);

    /* create an iov of the datatype */
    uintptr_t num_iov;
    rc = yaksa_iov_len(1, indexed_block, &num_iov);
    assert(rc == YAKSA_SUCCESS);

    fprintf(stdout, "num_iov = %lu\n", num_iov);
    struct iovec *iov_elem = malloc(sizeof(struct iovec) * num_iov);

    uintptr_t actual_iov_len;
    rc = yaksa_iov((const char *) input_matrix, 1, indexed_block, 0, iov_elem, num_iov,
                   &actual_iov_len);
    assert(rc == YAKSA_SUCCESS);

    print_matrix(input_matrix, ROWS, COLS, "input_matrix=");

    for (uintptr_t j = 0; j < num_iov; j++) {
        fprintf(stdout, "iov_elem[%lu] => iov_len = %zu; iov_base = [ ", j,
                iov_elem[j].iov_len / sizeof(int));
        for (uintptr_t k = 0; k < iov_elem[j].iov_len / sizeof(int); k++)
            fprintf(stdout, "%.*d ", 2, ((int *) (iov_elem[j].iov_base))[k]);
        fprintf(stdout, "]\n");
    }

    free(iov_elem);

    yaksa_type_free(indexed_block);
    yaksa_finalize();
    return 0;
}
