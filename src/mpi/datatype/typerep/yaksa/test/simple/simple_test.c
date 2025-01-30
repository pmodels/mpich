/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "yaksa.h"
#include <assert.h>

#define DIMSIZE (80)

int inbuf[DIMSIZE * DIMSIZE], outbuf[DIMSIZE * DIMSIZE];

int main()
{
    int rc = YAKSA_SUCCESS;
    yaksa_type_t vector, vector_vector;
    uintptr_t actual;

    yaksa_init(NULL);

    rc = yaksa_type_create_vector(3, 2, 3, YAKSA_TYPE__INT, NULL, &vector);
    assert(rc == YAKSA_SUCCESS);

    rc = yaksa_type_create_vector(5, 1, 10, vector, NULL, &vector_vector);
    assert(rc == YAKSA_SUCCESS);

    for (int i = 0; i < DIMSIZE * DIMSIZE; i++) {
        inbuf[i] = i;
        outbuf[i] = -1;
    }

    yaksa_request_t request;
    rc = yaksa_ipack(inbuf, 1, vector_vector, 0, outbuf, DIMSIZE * DIMSIZE * sizeof(int), &actual,
                     NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    assert(actual == 5 * 3 * 2 * sizeof(int));

    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    int idx = 0;
    int val = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 2; k++) {
                inbuf[val] = -1;
                if (outbuf[idx] != val)
                    fprintf(stderr, "outbuf[%d] = %d instead of %d\n", idx, outbuf[idx], val);
                idx++;
                val++;
            }
            val++;
        }
        val += 71;
    }

    uintptr_t actual_unpack_bytes;
    rc = yaksa_iunpack(outbuf, actual, inbuf, 1, vector_vector, 0, &actual_unpack_bytes,
                       NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);

    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    idx = 0;
    for (int i = 0; i < DIMSIZE; i++) {
        for (int j = 0; j < DIMSIZE; j++) {
            if (inbuf[idx] != idx)
                fprintf(stderr, "inbuf[%d] = %d instead of %d\n", idx, inbuf[idx], idx);
            idx++;
        }
    }

    yaksa_type_free(vector_vector);
    yaksa_type_free(vector);

    yaksa_finalize();

    return 0;
}
