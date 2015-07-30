/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* */
#include <stdio.h>
#include "mpi.h"
#include "mpitestconf.h"

int main(int argc, char *argv[])
{
    int i, ncvar, cnameLen, verbosity, binding, scope;
    int wrank, wsize, provided;
    char cname[256];
    char buf1[400], buf2[400], buf3[500];
    MPI_Datatype dtype;
    MPI_T_enum enumtype;
    MPI_T_cvar_handle bcastHandle, bcastLongHandle;
    int bcastCount, bcastScope, bcastCvar = -1;
    int bcastLongCount, bcastLongScope, bcastLongCvar = -1;
    int gatherScope, gatherCvar = -1;
    int newval;
    int errs = 0;

    MPI_Init(&argc, &argv);
    MPI_T_init_thread(MPI_THREAD_SINGLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    MPI_T_cvar_get_num(&ncvar);
    /* Find collective tuning cvars */
    for (i = 0; i < ncvar; i++) {
        cnameLen = sizeof(cname);
        MPI_T_cvar_get_info(i, cname, &cnameLen, &verbosity, &dtype,
                            &enumtype, NULL, NULL, &binding, &scope);
        if (strcmp(cname, "MPIR_CVAR_GATHER_VSMALL_MSG_SIZE") == 0) {
            gatherCvar = i;
            gatherScope = scope;
        }
        else if (strcmp(cname, "MPIR_CVAR_BCAST_SHORT_MSG_SIZE") == 0) {
            bcastCvar = i;
            bcastScope = scope;
            if (binding != MPI_T_BIND_NO_OBJECT && binding != MPI_T_BIND_MPI_COMM) {
                fprintf(stderr, "Unexpected binding for MPIR_CVAR_BCAST_SHORT_MSG\n");
                errs++;
            }
        }
        else if (strcmp(cname, "MPIR_CVAR_BCAST_LONG_MSG_SIZE") == 0) {
            bcastLongCvar = i;
            bcastLongScope = scope;
            if (binding != MPI_T_BIND_NO_OBJECT && binding != MPI_T_BIND_MPI_COMM) {
                fprintf(stderr, "Unexpected binding for MPIR_CVAR_BCAST_LONG_MSG\n");
                errs++;
            }
        }
        else if (strcmp(cname, "MPIR_CVAR_BCAST_MIN_PROCS") == 0) {
        }
    }

    /* Change the BCAST cvar.  If the parameter has local scope,
     * change it only on some processes */
    if (bcastCvar < 0 || bcastLongCvar < 0) {
        /* Skip because we did not find a corresponding control variable */
    }
    else {
        MPI_T_cvar_handle_alloc(bcastCvar, NULL, &bcastHandle, &bcastCount);
        if (bcastScope == MPI_T_SCOPE_LOCAL) {
            if ((wrank & 0x1)) {
                newval = 100;
                MPI_T_cvar_write(bcastHandle, &newval);
                MPI_T_cvar_read(bcastHandle, &newval);
                if (newval != 100) {
                    errs++;
                    fprintf(stderr, "cvar write failed for bcast\n");
                }
            }
        }
        else {
            newval = 100;
            MPI_T_cvar_write(bcastHandle, &newval);
            MPI_T_cvar_read(bcastHandle, &newval);
            if (newval != 100) {
                errs++;
                fprintf(stderr, "cvar write failed for bcast\n");
            }
        }

        MPI_T_cvar_handle_alloc(bcastLongCvar, NULL, &bcastLongHandle, &bcastLongCount);
        if (bcastLongScope == MPI_T_SCOPE_LOCAL) {
            if ((wrank & 0x1)) {
                newval = 200;
                MPI_T_cvar_write(bcastLongHandle, &newval);
                MPI_T_cvar_read(bcastLongHandle, &newval);
                if (newval != 200) {
                    errs++;
                    fprintf(stderr, "cvar write failed for bcast long\n");
                }
            }
        }
        else {
            newval = 100;
            MPI_T_cvar_write(bcastLongHandle, &newval);
            MPI_T_cvar_read(bcastLongHandle, &newval);
            if (newval != 100) {
                errs++;
                fprintf(stderr, "cvar write failed for bcast long\n");
            }
        }

        /* Test 1: Everyone under the new size */
        MPI_Bcast(buf1, 40, MPI_BYTE, 0, MPI_COMM_WORLD);

        /* Test 2: Everyone between the old and new size */
        /* As of 8/16/13, it appears that the small and medium algorithm
         * are the same; at least, local changes do not cause deadlocks */
        MPI_Bcast(buf2, 150, MPI_BYTE, 0, MPI_COMM_WORLD);

        /* Test 3: Everyone over the new long size */
        /* As of 8/16/13, this causes the program to hang, almost
         * certainly because the parameters much be changed collectively
         * but incorrectly indicate that they have local scope */
        MPI_Bcast(buf3, 250, MPI_BYTE, 0, MPI_COMM_WORLD);

        MPI_T_cvar_handle_free(&bcastHandle);
        MPI_T_cvar_handle_free(&bcastLongHandle);
    }

    /* Change the GATHER cvar.  If the parameter has local scope,
     * change it only on some processes.  Use a subcommunicator if the
     * size of comm_world is too large */

    /* For full coverage, should address all parameters for collective
     * algorithm selection */

    if (wrank == 0) {
        if (errs) {
            printf("found %d errors\n", errs);
        }
        else {
            printf(" No errors\n");
        }
    }

    MPI_T_finalize();
    MPI_Finalize();

    return 0;
}
