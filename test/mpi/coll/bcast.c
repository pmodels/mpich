/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "dtpools.h"

/*
static char MTEST_Descrip[] = "Test of broadcast with various roots and datatypes";
*/



int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, root;
    int minsize = 2, count;
    int i, j, len;
    MPI_Aint sendcount, recvcount;
    MPI_Comm comm;
    MPI_Datatype sendtype, recvtype;
    DTP_t send_dtp, recv_dtp;
    char send_name[MPI_MAX_OBJECT_NAME] = { 0 };
    char recv_name[MPI_MAX_OBJECT_NAME] = { 0 };
    void *sendbuf, *recvbuf;

    MTest_Init(&argc, &argv);

#ifndef USE_DTP_POOL_TYPE__STRUCT       /* set in 'test/mpi/structtypetest.txt' to split tests */
    MPI_Datatype basic_type;
    char type_name[MPI_MAX_OBJECT_NAME] = { 0 };

    err = MTestInitBasicSignature(argc, argv, &count, &basic_type);
    if (err)
        MTestReturnValue(1);

    err = DTP_pool_create(basic_type, count, &send_dtp);
    if (err != DTP_SUCCESS) {
        MPI_Type_get_name(basic_type, type_name, &len);
        fprintf(stdout, "Error while creating send pool (%s,%d)\n", type_name, count);
        fflush(stdout);
    }

    err = DTP_pool_create(basic_type, count, &recv_dtp);
    if (err != DTP_SUCCESS) {
        MPI_Type_get_name(basic_type, type_name, &len);
        fprintf(stdout, "Error while creating recv pool (%s,%d)\n", type_name, count);
        fflush(stdout);
    }
#else
    MPI_Datatype *basic_types = NULL;
    int *basic_type_counts = NULL;
    int basic_type_num;

    err = MTestInitStructSignature(argc, argv, &basic_type_num, &basic_type_counts, &basic_types);
    if (err)
        MTestReturnValue(1);

    err = DTP_pool_create_struct(basic_type_num, basic_types, basic_type_counts, &send_dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stdout, "Error while creating struct pool\n");
        fflush(stdout);
    }

    err = DTP_pool_create_struct(basic_type_num, basic_types, basic_type_counts, &recv_dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stdout, "Error while creating struct pool\n");
        fflush(stdout);
    }

    /* this is ignored */
    count = 0;
#endif

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

#if defined BCAST_COMM_WORLD_ONLY
        if (comm != MPI_COMM_WORLD) {
            MTestFreeComm(&comm);
            continue;
        }
#endif /* BCAST_COMM_WORLD_ONLY */

        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Errhandler_set(comm, MPI_ERRORS_RETURN);

        for (root = 0; root < size; root++) {
            for (i = 0; i < send_dtp->DTP_num_objs; i++) {
                err = DTP_obj_create(send_dtp, i, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }

                sendcount = send_dtp->DTP_obj_array[i].DTP_obj_count;
                sendtype = send_dtp->DTP_obj_array[i].DTP_obj_type;
                sendbuf = send_dtp->DTP_obj_array[i].DTP_obj_buf;

                for (j = 0; j < recv_dtp->DTP_num_objs; j++) {
                    err = DTP_obj_create(recv_dtp, j, 0, 0, 0);
                    if (err != DTP_SUCCESS) {
                        errs++;
                        break;
                    }

                    recvcount = recv_dtp->DTP_obj_array[j].DTP_obj_count;
                    recvtype = recv_dtp->DTP_obj_array[j].DTP_obj_type;
                    recvbuf = recv_dtp->DTP_obj_array[j].DTP_obj_buf;

                    if (rank == root) {
                        err = MPI_Bcast(sendbuf, sendcount, sendtype, root, comm);
                        if (err) {
                            errs++;
                            MTestPrintError(err);
                        }
                    } else {
                        err = MPI_Bcast(recvbuf, recvcount, recvtype, root, comm);
                        if (err) {
                            errs++;
                            MPI_Type_get_name(recvtype, recv_name, &len);
                            fprintf(stderr, "Error with communicator %s and datatype %s\n",
                                    MTestGetIntracommName(), recv_name);
                            MTestPrintError(err);
                        }

                        err = DTP_obj_buf_check(recv_dtp, j, 0, 1, count);
                        if (err != DTP_SUCCESS) {
                            errs++;
                            if (errs < 10) {
                                MPI_Type_get_name(sendtype, send_name, &len);
                                MPI_Type_get_name(recvtype, recv_name, &len);
                                fprintf(stdout,
                                        "Data received with type %s does not match data sent with type %s\n",
                                        recv_name, send_name);
                                fflush(stdout);
                            }
                        }
                    }
                    DTP_obj_free(recv_dtp, j);
                }
                DTP_obj_free(send_dtp, i);
            }
        }
        MTestFreeComm(&comm);
    }

    DTP_pool_free(send_dtp);
    DTP_pool_free(recv_dtp);

#ifdef USE_DTP_POOL_TYPE__STRUCT
    /* cleanup array if any */
    if (basic_types) {
        free(basic_types);
    }
    if (basic_type_counts) {
        free(basic_type_counts);
    }
#endif

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
