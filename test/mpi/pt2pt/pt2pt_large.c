/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

/* Basic tests for large pt2pt APIs. */

#if 0
#define CTYPE   int
#define MPITYPE MPI_INT
#else
#define CTYPE   char
#define MPITYPE MPI_CHAR
#endif

int errs;

static void init_buf(void **buf, MPI_Aint count, int stride, CTYPE val)
{
    MPI_Aint total_size = count * stride * sizeof(CTYPE);
    *buf = malloc(total_size);

    CTYPE *intbuf = *buf;
    for (MPI_Aint i = 0; i < count; i++) {
        intbuf[i * stride] = val;
    }
}

static void check_buf(void *buf, MPI_Aint count, int stride, CTYPE val)
{
    CTYPE *intbuf = buf;
    for (MPI_Aint i = 0; i < count; i++) {
        if (intbuf[i * stride] != val) {
            if (errs < 10) {
                printf("i = %ld, expect %d, got %d\n",
                       (long) i, (int) val, (int) intbuf[i * stride]);
            }
            errs++;
        }
    }
}

int main(int argc, char *argv[])
{
    int option = 0;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-option=", 8) == 0) {
            option = atoi(argv[i] + 8);
            break;
        }
    }

    MTest_Init(&argc, &argv);

    MPI_Comm comm;
    comm = MPI_COMM_WORLD;

    int rank, nproc;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);
    if (nproc != 2) {
        printf("Run test with 2 procs!\n");
        return 1;
    }

    MPI_Count count;
    MPI_Datatype datatype;
    int stride;
    bool datatype_need_free = false;

    switch (option) {
        case 0:
            /* small count, just test the _c API */
            count = 1024;
            datatype = MPITYPE;
            stride = 1;
            break;
        case 1:
            /* large count, contiguous datatype */
            count = (MPI_Count) INT_MAX + 100;
            datatype = MPITYPE;
            stride = 1;
            break;
        case 2:
            /* large count, non-contig datatype */
            count = (MPI_Count) INT_MAX + 100;
            MPI_Type_create_resized(MPITYPE, 0, sizeof(CTYPE) * 2, &datatype);
            MPI_Type_commit(&datatype);
            stride = 2;
            datatype_need_free = true;
            break;
        default:
            if (rank == 0) {
                printf("Unexpected option: %d\n", option);
            }
            goto fn_exit;
    }

    int tag = 0;
    if (rank == 0) {
        void *sendbuf;
        init_buf(&sendbuf, count, stride, 1);
        MPI_Send_c(sendbuf, count, datatype, 1, tag, comm);
        free(sendbuf);
    } else if (rank == 1) {
        void *recvbuf;
        init_buf(&recvbuf, count, stride, 0);
        MPI_Recv_c(recvbuf, count, datatype, 0, tag, comm, MPI_STATUS_IGNORE);
        check_buf(recvbuf, count, stride, 1);
        free(recvbuf);
    }

    if (datatype_need_free) {
        MPI_Type_free(&datatype);
    }

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
