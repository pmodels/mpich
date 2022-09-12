/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpichinfo.h"
/*
   Global definitions of variables that hold information about the
   version and patchlevel.  This allows easy access to the version
   and configure information without requiring the user to run an MPI
   program
*/
const char MPII_Version_string[] = MPICH_VERSION;
const char MPII_Version_date[] = MPICH_VERSION_DATE;
const char MPII_Version_ABI[] = MPICH_ABIVERSION;
const char MPII_Version_configure[] = MPICH_CONFIGURE_ARGS_CLEAN;
const char MPII_Version_device[] = MPICH_DEVICE;
const char MPII_Version_CC[] = MPICH_COMPILER_CC;
const char MPII_Version_CXX[] = MPICH_COMPILER_CXX;
const char MPII_Version_F77[] = MPICH_COMPILER_F77;
const char MPII_Version_FC[] = MPICH_COMPILER_FC;
const char MPII_Version_custom[] = MPICH_CUSTOM_STRING;

int MPIR_Get_library_version_impl(char *version, int *resultlen)
{
    int printed_len;
    printed_len = MPL_snprintf(version, MPI_MAX_LIBRARY_VERSION_STRING,
                               "MPICH Version:      %s\n"
                               "MPICH Release date: %s\n"
                               "MPICH ABI:          %s\n"
                               "MPICH Device:       %s\n"
                               "MPICH configure:    %s\n"
                               "MPICH CC:           %s\n"
                               "MPICH CXX:          %s\n"
                               "MPICH F77:          %s\n"
                               "MPICH FC:           %s\n",
                               MPII_Version_string, MPII_Version_date, MPII_Version_ABI,
                               MPII_Version_device, MPII_Version_configure, MPII_Version_CC,
                               MPII_Version_CXX, MPII_Version_F77, MPII_Version_FC);
    if (strlen(MPII_Version_custom) > 0)
        MPL_snprintf(version + printed_len, MPI_MAX_LIBRARY_VERSION_STRING - printed_len,
                     "MPICH Custom Information:\t%s\n", MPII_Version_custom);

    *resultlen = (int) strlen(version);

    return MPI_SUCCESS;
}
