/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;
    int position, pack_size, i;
    int dis[2], blklens[2];
    MPI_Datatype type;
    int send_buffer[60];
    int recv_buffer[60];
    int pack_buffer[1000];

    MTest_Init(&argc, &argv);

    /* Initialize data in the buffers */
    for (i = 0; i < 60; i++) {
        send_buffer[i] = i;
        recv_buffer[i] = -1;
        pack_buffer[i] = -2;
    }

    /* Create an indexed type with an empty first block */
    dis[0] = 0;
    dis[1] = 20;

    blklens[0] = 0;
    blklens[1] = 40;

    MPI_Type_indexed(2, blklens, dis, MPI_INT, &type);
    MPI_Type_commit(&type);

    position = 0;
    MPI_Pack(send_buffer, 1, type, pack_buffer, sizeof(pack_buffer), &position, MPI_COMM_WORLD);
    pack_size = position;
    position = 0;
    MPI_Unpack(pack_buffer, pack_size, &position, recv_buffer, 1, type, MPI_COMM_WORLD);

    /* Check that the last 40 entries of the recv_buffer have the corresponding
     * elements from the send buffer */
    for (i = 0; i < 20; i++) {
        if (recv_buffer[i] != -1) {
            errs++;
            fprintf(stderr, "recv_buffer[%d] = %d, should = -1\n", i, recv_buffer[i]);
        }
    }
    for (i = 20; i < 60; i++) {
        if (recv_buffer[i] != i) {
            errs++;
            fprintf(stderr, "recv_buffer[%d] = %d, should = %d\n", i, recv_buffer[i], i);
        }
    }
    MPI_Type_free(&type);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;

}
