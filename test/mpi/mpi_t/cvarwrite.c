/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"
#include "mpitestconf.h"

#define MAX_STR_CVAR_LEN 512
#define MAX_VAR_NAME_LEN 128

int main(int argc, char *argv[])
{
    int i;
    int required, provided;
    int num_cvar;
    char name[MAX_VAR_NAME_LEN];
    int namelen, verbosity, datatype, desclen, binding, scope, count;
    MPI_T_enum enumtype = MPI_T_ENUM_NULL;
    int iin, iout, iold;
    unsigned uin, uout, uold;
    unsigned long ulin, ulout, ulold;
    unsigned long long ullin, ullout, ullold;
    char cin[MAX_STR_CVAR_LEN], cout[MAX_STR_CVAR_LEN], cold[MAX_STR_CVAR_LEN];
    MPI_T_cvar_handle chandle;
    int errs = 0;

    required = MPI_THREAD_SINGLE;
    namelen = sizeof(name);

    MTest_Init(&argc, &argv);
    MPI_T_init_thread(required, &provided);

    MPI_T_cvar_get_num(&num_cvar);
    MTestPrintfMsg(10, "Total %d MPI control variables\n", num_cvar);

    for (i = 0; i < num_cvar; i++) {
        MPI_T_cvar_get_info(i, name, &namelen, &verbosity, &datatype, &enumtype,
                            NULL /* desc is intentionly ignored to test NULL input */ , &desclen,
                            &binding, &scope);
        if (binding != MPI_T_BIND_NO_OBJECT)
            continue;

        MPI_T_cvar_handle_alloc(i, NULL, &chandle, &count);
        if (count == 1 || (datatype == MPI_CHAR && count < sizeof(cin))) {
            if (MPI_INT == datatype) {
                iin = 123;
                iout = 456;
                MPI_T_cvar_read(chandle, &iold);        /* Read the old value */
                MPI_T_cvar_write(chandle, &iin);        /* Write an arbitrary value */
                MPI_T_cvar_read(chandle, &iout);        /* Read the value again */
                MPI_T_cvar_write(chandle, &iold);       /* Restore the old value */
                if (iin != iout)
                    errs++;
            }
            else if (MPI_UNSIGNED == datatype) {
                uin = 133;
                uout = 986;
                MPI_T_cvar_read(chandle, &uold);
                MPI_T_cvar_write(chandle, &uin);
                MPI_T_cvar_read(chandle, &uout);
                MPI_T_cvar_write(chandle, &uold);
                if (uin != uout)
                    errs++;
            }
            else if (MPI_UNSIGNED_LONG == datatype) {
                ulin = 1830;
                ulout = 2014;
                MPI_T_cvar_read(chandle, &ulold);
                MPI_T_cvar_write(chandle, &ulin);
                MPI_T_cvar_read(chandle, &ulout);
                MPI_T_cvar_write(chandle, &ulold);
                if (ulin != ulout)
                    errs++;
            }
            else if (MPI_UNSIGNED_LONG_LONG == datatype) {
                ullin = 11930;
                ullout = 52014;
                MPI_T_cvar_read(chandle, &ullold);
                MPI_T_cvar_write(chandle, &ullin);
                MPI_T_cvar_read(chandle, &ullout);
                MPI_T_cvar_write(chandle, &ullold);
                if (ullin != ullout)
                    errs++;
            }
            else if (MPI_CHAR == datatype) {
                strcpy(cin, "GARBAGE MPI_CHAR CVAR VALUE");
                strcpy(cout, "TEMPORARY MPI_CHAR CVAR VALUE");
                MPI_T_cvar_read(chandle, cold);
                MPI_T_cvar_write(chandle, cin);
                MPI_T_cvar_read(chandle, cout);
                MPI_T_cvar_write(chandle, cold);
                /* printf("%s = %s\n", name, cold); */
                if (strcmp(cin, cout))
                    errs++;
            }
        }

        MPI_T_cvar_handle_free(&chandle);
    }

    MPI_T_finalize();
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
