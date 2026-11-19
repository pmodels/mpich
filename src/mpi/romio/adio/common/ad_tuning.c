/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

/* Common performance tuning variables for collective I/O. Inside MPICH they
 * are CVARs (MPIR_CVAR_ROMIO_*, declared below, extern declarations in
 * mpir_ext.h); each also reads its legacy ROMIO_ environment variable.
 * ad_get_env_vars() copies them into the romio_* globals whenever a file's
 * hints are set up, so a change through MPI_T applies from the next open.
 * ROMIO may change its copies during an operation, never the CVARs.
 * Outside MPICH, ad_get_env_vars() reads the ROMIO_ environment variables.
 */

#include "ad_tuning.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
   - name : ROMIO
     description : >-
        A category for ROMIO (MPI-IO) variables. ROMIO reads them when a
        file is opened or its hints are set, so a change through MPI_T
        applies from the next open.

cvars:
    - name        : MPIR_CVAR_ROMIO_WRITE_AGGMETHOD
      alt-env     : ROMIO_WRITE_AGGMETHOD
      category    : ROMIO
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Replaces the two-phase collective write aggregation with a one-sided
        algorithm, significantly reducing communication and memory overhead.
        It supports all datasets and datatypes. The caveat is that holes in
        the data when writing to a pre-existing file are ignored unless
        read-modify-write is enabled (see MPIR_CVAR_ROMIO_ONESIDED_NO_RMW and
        MPIR_CVAR_ROMIO_ONESIDED_ALWAYS_RMW), so middleware such as PnetCDF
        that relies on read-modify-write (e.g. for fill values) may be
        affected.
        0 - Normal two-phase collective I/O.
        1 - A separate MPI_Put for each contiguous chunk of data.
        2 - One MPI_Put with a derived datatype built from all the contiguous chunks.

    - name        : MPIR_CVAR_ROMIO_READ_AGGMETHOD
      alt-env     : ROMIO_READ_AGGMETHOD
      category    : ROMIO
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Replaces the two-phase collective read aggregation with a one-sided
        algorithm.
        0 - Normal two-phase collective I/O.
        1 - A separate MPI_Get for each contiguous chunk of data.
        2 - One MPI_Get with a derived datatype built from all the contiguous chunks.

    - name        : MPIR_CVAR_ROMIO_ONESIDED_NO_RMW
      alt-env     : ROMIO_ONESIDED_NO_RMW
      category    : ROMIO
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For one-sided write aggregation (MPIR_CVAR_ROMIO_WRITE_AGGMETHOD 1 or
        2), skip the detection of holes in the data when writing to a
        pre-existing file, avoiding its communication overhead.

    - name        : MPIR_CVAR_ROMIO_ONESIDED_ALWAYS_RMW
      alt-env     : ROMIO_ONESIDED_ALWAYS_RMW
      category    : ROMIO
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For one-sided write aggregation (MPIR_CVAR_ROMIO_WRITE_AGGMETHOD 1 or
        2), always pre-read the file range being written, filling any holes
        in the data. Implies MPIR_CVAR_ROMIO_ONESIDED_NO_RMW.

    - name        : MPIR_CVAR_ROMIO_ONESIDED_INFORM_RMW
      alt-env     : ROMIO_ONESIDED_INFORM_RMW
      category    : ROMIO
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For one-sided write aggregation (MPIR_CVAR_ROMIO_WRITE_AGGMETHOD 1 or
        2), print a message when holes are found in the data, which requires
        a read-modify-write, so the user can set
        MPIR_CVAR_ROMIO_ONESIDED_NO_RMW on later runs to avoid the hole
        detection overhead.

    - name        : MPIR_CVAR_ROMIO_TUNEGATHER
      alt-env     : ROMIO_TUNEGATHER
      category    : ROMIO
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        How the starting and ending offsets are collected for aggregator
        collective I/O: one MPI_Allreduce(MPI_MAX) if true, two or three
        MPI_Allgather calls if false.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int romio_write_aggmethod;
int romio_read_aggmethod;
int romio_onesided_no_rmw;
int romio_onesided_always_rmw;
int romio_onesided_inform_rmw;
int romio_tunegather;

#ifndef ROMIO_INSIDE_MPICH
static int get_env_int(const char *name, int dflt)
{
    char *x = getenv(name);
    return x ? atoi(x) : dflt;
}
#endif

void ad_get_env_vars(void)
{
#ifdef ROMIO_INSIDE_MPICH
    romio_write_aggmethod = MPIR_CVAR_ROMIO_WRITE_AGGMETHOD;
    romio_read_aggmethod = MPIR_CVAR_ROMIO_READ_AGGMETHOD;
    romio_onesided_no_rmw = MPIR_CVAR_ROMIO_ONESIDED_NO_RMW;
    romio_onesided_always_rmw = MPIR_CVAR_ROMIO_ONESIDED_ALWAYS_RMW;
    romio_onesided_inform_rmw = MPIR_CVAR_ROMIO_ONESIDED_INFORM_RMW;
    romio_tunegather = MPIR_CVAR_ROMIO_TUNEGATHER;
#else
    romio_write_aggmethod = get_env_int("ROMIO_WRITE_AGGMETHOD", 0);
    romio_read_aggmethod = get_env_int("ROMIO_READ_AGGMETHOD", 0);
    romio_onesided_no_rmw = get_env_int("ROMIO_ONESIDED_NO_RMW", 0);
    romio_onesided_always_rmw = get_env_int("ROMIO_ONESIDED_ALWAYS_RMW", 0);
    romio_onesided_inform_rmw = get_env_int("ROMIO_ONESIDED_INFORM_RMW", 0);
    romio_tunegather = get_env_int("ROMIO_TUNEGATHER", 1);
#endif
    if (romio_onesided_always_rmw)
        romio_onesided_no_rmw = 1;
}
