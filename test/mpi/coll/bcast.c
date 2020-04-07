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
    int minsize = 2;
    int i, j, seed, testsize;
    MPI_Aint count;
    MPI_Aint maxbufsize;
    MPI_Comm comm;
    MPI_Datatype type;
    DTP_pool_s dtp;
    DTP_obj_s obj;
    void *buf;
    char *basic_type;

    MTest_Init(&argc, &argv);

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count = MTestArgListGetLong(head, "count");
    basic_type = MTestArgListGetString(head, "type");

    maxbufsize = MTestDefaultMaxBufferSize();

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    err = DTP_pool_create(basic_type, count, seed + rank, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating send pool (%s,%ld)\n", basic_type, count);
        fflush(stderr);
    }

    MTestArgListDestroy(head);

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
#if defined BCAST_COMM_WORLD_ONLY
        if (comm != MPI_COMM_WORLD) {
            MTestFreeComm(&comm);
            continue;
        }
#endif /* BCAST_COMM_WORLD_ONLY */

        if (comm == MPI_COMM_NULL)
            continue;

        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Errhandler_set(comm, MPI_ERRORS_RETURN);

        for (root = 0; root < size; root++) {
            for (i = 0; i < testsize; i++) {
                err = DTP_obj_create(dtp, &obj, maxbufsize);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }

                buf = malloc(obj.DTP_bufsize);
                if (buf == NULL) {
                    errs++;
                    break;
                }

                if (rank == root) {
                    err = DTP_obj_buf_init(obj, buf, 0, 1, count);
                    if (err != DTP_SUCCESS) {
                        errs++;
                        break;
                    }
                } else {
                    err = DTP_obj_buf_init(obj, buf, -1, -1, count);
                    if (err != DTP_SUCCESS) {
                        errs++;
                        break;
                    }
                }

                err =
                    MPI_Bcast(buf + obj.DTP_buf_offset, obj.DTP_type_count, obj.DTP_datatype, root,
                              comm);
                if (err) {
                    errs++;
                    MTestPrintError(err);
                }

                err = DTP_obj_buf_check(obj, buf, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    errs++;
                    if (errs < 10) {
                        char *desc;
                        DTP_obj_get_description(obj, &desc);
                        fprintf(stderr,
                                "Data received with type %s does not match data sent\n", desc);
                        fflush(stderr);
                    }
                }

                free(buf);
                DTP_obj_free(obj);
            }
        }
        MTestFreeComm(&comm);
    }

    DTP_pool_free(dtp);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
