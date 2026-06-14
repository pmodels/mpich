/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "process.h"

/*
   Test the argument parsing and processing routines.  See the Makefile
   for typical test choices
*/
int main(int argc, char *argv[], char *envp[])
{
    int rc;

    MPIE_ProcessInit();
    MPIE_Args(argc, argv, &pUniv, 0, 0);
    MPIE_PrintProcessUniverse(stdout, &pUniv);
    MPIE_ForkProcesses(&pUniv.worlds[0], envp, 0, 0, 0, 0, 0, 0);

    rc = MPIE_ProcessGetExitStatus();

    return rc;
}

void mpiexec_usage(const char *str)
{
    fprintf(stderr, "Usage: %s\n", str);
    return 0;
}
