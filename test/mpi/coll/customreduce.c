#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*      Warning - this test will fail for custom datatypes and custom reduce operator combination
 *
 */
int add_op(const void *invec, void *inoutvec, int *count, MPI_Datatype datatype)
{
    int i;
    int *cin = (int *) invec, *cout = (int *) inoutvec;
    for (i = 0; i < *count; i++) {
        cout[i] = cin[i] + cout[i];
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int rank, count, size, minsize = 1;
    int isLeftGroup;
    MPI_Status status;
    unsigned int errs = 0;
    MPI_Comm comm;
    MTestDatatype sendtype, recvtype;
    MTest_Init(&argc, &argv);

    /*
     * Reduce with custom operator
     */
    while (MTestGetIntracomm(&comm, minsize)) {
        if (comm == MPI_COMM_NULL)
            continue;
        MPI_Comm_rank(comm, &rank);
        for (count = 1; count <= 64; count = count * 2) {
            while (MTestGetDatatypes(&sendtype, &recvtype, count)) {
                MPI_Op op;
                int rc;
                MPI_Request request;
                int sendNI, sendNA, sendND, sendCombiner;
                int recvNI, recvNA, recvND, recvCombiner;
                MPI_Type_get_envelope(sendtype.datatype, &sendNI, &sendNA, &sendND, &sendCombiner);
                MPI_Type_get_envelope(recvtype.datatype, &recvNI, &recvNA, &recvND, &recvCombiner);
                if (sendNI != recvNI || sendNA != recvNA || sendND != recvND
                    || sendCombiner != recvCombiner) {
                    continue;
                }
                MPI_Op_create((MPI_User_function *) add_op, 1, &op);
                sendtype.InitBuf(&sendtype);
                recvtype.InitBuf(&recvtype);
                if (rank == 0) {
                    rc = MPI_Reduce(sendtype.buf, recvtype.buf, sendtype.count,
                                    sendtype.datatype, op, 0, comm);
                }
                else {
                    rc = MPI_Reduce(sendtype.buf, recvtype.buf, recvtype.count,
                                    recvtype.datatype, op, 0, comm);
                }
                MPI_Op_free(&op);
                if (rc) {
                    errs++;
                }
                else if (rank == 0) {
                    errs += MTestCheckRecv(0, &recvtype);
                }
                MTestFreeDatatype(&sendtype);
                MTestFreeDatatype(&recvtype);
            }
        }
        MTestFreeComm(&comm);
    }

    /*
     * Allreduce with custom operator
     */
    while (MTestGetIntracomm(&comm, minsize)) {
        if (comm == MPI_COMM_NULL)
            continue;
        MPI_Comm_rank(comm, &rank);
        for (count = 1; count <= 64; count = count * 2) {
            while (MTestGetDatatypes(&sendtype, &recvtype, count)) {
                MPI_Op op;
                int rc;
                MPI_Request request;
                int sendNI, sendNA, sendND, sendCombiner;
                int recvNI, recvNA, recvND, recvCombiner;
                MPI_Type_get_envelope(sendtype.datatype, &sendNI, &sendNA, &sendND, &sendCombiner);
                MPI_Type_get_envelope(recvtype.datatype, &recvNI, &recvNA, &recvND, &recvCombiner);
                if (sendNI != recvNI || sendNA != recvNA || sendND != recvND
                    || sendCombiner != recvCombiner) {
                    continue;
                }
                MPI_Op_create((MPI_User_function *) add_op, 1, &op);
                sendtype.InitBuf(&sendtype);
                recvtype.InitBuf(&recvtype);
                if (rank == 0) {
                    rc = MPI_Allreduce(sendtype.buf, recvtype.buf,
                                       sendtype.count, sendtype.datatype, op, comm);
                }
                else {
                    rc = MPI_Allreduce(sendtype.buf, recvtype.buf,
                                       recvtype.count, recvtype.datatype, op, comm);
                }
                MPI_Op_free(&op);
                if (rc) {
                    errs++;
                }
                else {
                    errs += MTestCheckRecv(0, &recvtype);
                }
                MTestFreeDatatype(&sendtype);
                MTestFreeDatatype(&recvtype);

            }

        }
        MTestFreeComm(&comm);
    }

    MTest_Finalize(errs);
    return 0;
}
