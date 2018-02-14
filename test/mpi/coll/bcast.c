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

typedef struct {
    char *typename;
    MPI_Datatype type;
} Type_t;

Type_t typelist[] = {
    {"MPI_CHAR", MPI_CHAR},
    {"MPI_BYTE", MPI_BYTE},
    {"MPI_WCHAR", MPI_WCHAR},
    {"MPI_SHORT", MPI_SHORT},
    {"MPI_INT", MPI_INT},
    {"MPI_LONG", MPI_LONG},
    {"MPI_LONG_LONG_INT", MPI_LONG_LONG_INT},
    {"MPI_UNSIGNED_CHAR", MPI_UNSIGNED_CHAR},
    {"MPI_UNSIGNED_SHORT", MPI_UNSIGNED_SHORT},
    {"MPI_UNSIGNED", MPI_UNSIGNED},
    {"MPI_UNSIGNED_LONG", MPI_UNSIGNED_LONG},
    {"MPI_UNSIGNED_LONG_LONG", MPI_UNSIGNED_LONG_LONG},
    {"MPI_FLOAT", MPI_FLOAT},
    {"MPI_DOUBLE", MPI_DOUBLE},
    {"MPI_LONG_DOUBLE", MPI_LONG_DOUBLE},
    {"MPI_INT8_T", MPI_INT8_T},
    {"MPI_INT16_T", MPI_INT16_T},
    {"MPI_INT32_T", MPI_INT32_T},
    {"MPI_INT64_T", MPI_INT64_T},
    {"MPI_UINT8_T", MPI_UINT8_T},
    {"MPI_UINT16_T", MPI_UINT16_T},
    {"MPI_UINT32_T", MPI_UINT32_T},
    {"MPI_UINT64_T", MPI_UINT64_T},
    {"MPI_C_COMPLEX", MPI_C_COMPLEX},
    {"MPI_C_FLOAT_COMPLEX", MPI_C_FLOAT_COMPLEX},
    {"MPI_C_DOUBLE_COMPLEX", MPI_C_DOUBLE_COMPLEX},
    {"MPI_C_LONG_DOUBLE_COMPLEX", MPI_C_LONG_DOUBLE_COMPLEX},
    {"MPI_FLOAT_INT", MPI_FLOAT_INT},
    {"MPI_DOUBLE_INT", MPI_DOUBLE_INT},
    {"MPI_LONG_INT", MPI_LONG_INT},
    {"MPI_2INT", MPI_2INT},
    {"MPI_SHORT_INT", MPI_SHORT_INT},
    {"MPI_LONG_DOUBLE_INT", MPI_LONG_DOUBLE_INT},
    {"MPI_DATATYPE_NULL", MPI_DATATYPE_NULL}
};

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, root;
    int minsize = 2, count;
    int i, j, len;
    MPI_Aint sendcount, recvcount;
    MPI_Comm comm;
    MPI_Datatype sendtype, recvtype;
    MPI_Datatype basic_type;
    DTP_t send_dtp, recv_dtp;
    char type_name[MPI_MAX_OBJECT_NAME] = { 0 };
    char send_name[MPI_MAX_OBJECT_NAME] = { 0 };
    char recv_name[MPI_MAX_OBJECT_NAME] = { 0 };
    void *sendbuf, *recvbuf;

    MTest_Init(&argc, &argv);

    /* TODO: parse input parameters using optarg */
    if (argc < 3) {
        fprintf(stdout, "Usage: %s -type=[TYPE] -count=[COUNT]\n", argv[0]);
        return MTestReturnValue(1);
    } else {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "-type=", strlen("-type="))) {
                j = 0;
                while (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL") &&
                       strcmp(argv[i] + strlen("-type="), typelist[j].typename)) {
                    j++;
                }

                if (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL")) {
                    basic_type = typelist[j].type;
                } else {
                    fprintf(stdout, "Error: datatype not recognized\n");
                    return MTestReturnValue(1);
                }
            } else if (!strncmp(argv[i], "-count=", strlen("-count="))) {
                count = atoi(argv[i] + strlen("-count="));
                /* TODO: make sure count is valid */
            }
        }
    }

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

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
