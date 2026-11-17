/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run datatype_packed_recv
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Send packed data as MPI_PACKED and receive it with the datatype it was packed from";
*/

/* The sender doesn't know the receive datatype, so a message split into segments
 * may split a basic element of the receive datatype between two segments. The
 * message sizes cover a single segment and several. */

static int test_type(MPI_Datatype elem, const char *name, int nbytes, MPI_Comm comm)
{
    int errs = 0;
    int rank, elem_size;
    MPI_Aint lb, extent;
    MPI_Datatype vec, type;

    MPI_Comm_rank(comm, &rank);
    MPI_Type_size(elem, &elem_size);
    int count = nbytes / elem_size;

    /* one element out of three, like a row of a column-major matrix */
    MPI_Type_get_extent(elem, &lb, &extent);
    MPI_Type_vector(1, 1, 3, elem, &vec);
    MPI_Type_create_resized(vec, 0, 3 * extent, &type);
    MPI_Type_free(&vec);
    MPI_Type_commit(&type);
    MPI_Type_get_extent(type, &lb, &extent);

    size_t buf_sz = (size_t) count * extent;
    unsigned char *buf = calloc(buf_sz, 1);
    int pack_sz;
    MPI_Pack_size(count, type, comm, &pack_sz);
    /* zeroed, since pack may leave padding bytes (e.g. of long double) untouched */
    char *pbuf = calloc(pack_sz, 1);
    char *qbuf = calloc(pack_sz, 1);

    for (size_t i = 0; i < buf_sz; i++) {
        buf[i] = (unsigned char) (i * 7 + 1);
    }

    int pos = 0;
    if (rank == 0) {
        MPI_Pack(buf, count, type, pbuf, pack_sz, &pos, comm);
        MPI_Send(pbuf, pos, MPI_PACKED, 1, 0, comm);
    } else if (rank == 1) {
        unsigned char *rbuf = calloc(buf_sz, 1);
        int err = MPI_Recv(rbuf, count, type, 0, 0, comm, MPI_STATUS_IGNORE);
        if (err != MPI_SUCCESS) {
            char msg[MPI_MAX_ERROR_STRING];
            int len;
            MPI_Error_string(err, msg, &len);
            printf("%s, %d bytes: MPI_Recv failed: %s\n", name, nbytes, msg);
            errs++;
        } else {
            /* compare through the packed form, which leaves out the gaps */
            int pos2 = 0;
            MPI_Pack(buf, count, type, pbuf, pack_sz, &pos, comm);
            MPI_Pack(rbuf, count, type, qbuf, pack_sz, &pos2, comm);
            if (pos != pos2 || memcmp(pbuf, qbuf, pos) != 0) {
                printf("%s, %d bytes: received data differ\n", name, nbytes);
                errs++;
            }
        }
        free(rbuf);
    }

    free(buf);
    free(pbuf);
    free(qbuf);
    MPI_Type_free(&type);
    return errs;
}

int run(const char *arg)
{
    int errs = 0;
    int rank, size;
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    if (size < 2) {
        printf("This test requires at least 2 processes\n");
        MPI_Abort(comm, 1);
    }

    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

    const int sizes[] = { 4000, 16000, 160000 };
    for (int i = 0; i < 3; i++) {
        errs += test_type(MPI_DOUBLE, "MPI_DOUBLE", sizes[i], comm);
        errs += test_type(MPI_C_DOUBLE_COMPLEX, "MPI_C_DOUBLE_COMPLEX", sizes[i], comm);
        if (MPI_C_LONG_DOUBLE_COMPLEX != MPI_DATATYPE_NULL) {
            errs += test_type(MPI_C_LONG_DOUBLE_COMPLEX, "MPI_C_LONG_DOUBLE_COMPLEX", sizes[i],
                              comm);
        }
    }

    MPI_Comm_set_errhandler(comm, MPI_ERRORS_ARE_FATAL);
    return errs;
}
