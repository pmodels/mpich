/* This file created from test/mpi/f77/ext/c2fmult.c with f77tof90 */
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
  Check that MPI_xxxx_c2f, applied to the same object several times,
  yields the same handle.  We do this because when MPI handles in
  C are a different length than those in Fortran, care needs to
  be exercised to ensure that the mapping from one to another is unique.
  (Test added to test a potential problem in ROMIO for handling MPI_File
  on 64-bit systems)
*/
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MPI_Fint handleA, handleB;
    int rc;
    int errs = 0;
    int buf[1];
    MPI_Request cRequest;
    MPI_Status st;
    int tFlag;

    MTest_Init(&argc, &argv);

    /* Request */
    rc = MPI_Irecv(buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &cRequest);
    if (rc) {
        errs++;
        printf("Unable to create request\n");
    }
    else {
        handleA = MPI_Request_c2f(cRequest);
        handleB = MPI_Request_c2f(cRequest);
        if (handleA != handleB) {
            errs++;
            printf("MPI_Request_c2f does not give the same handle twice on the same MPI_Request\n");
        }
    }
    MPI_Cancel(&cRequest);
    MPI_Test(&cRequest, &tFlag, &st);
    MPI_Test_cancelled(&st, &tFlag);
    if (!tFlag) {
        errs++;
        printf("Unable to cancel MPI_Irecv request\n");
    }
    /* Using MPI_Request_free should be ok, but some MPI implementations
     * object to it imediately after the cancel and that isn't essential to
     * this test */

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
