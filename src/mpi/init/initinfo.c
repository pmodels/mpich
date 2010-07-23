/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpich2info.h"
/* 
   Global definitions of variables that hold information about the
   version and patchlevel.  This allows easy access to the version 
   and configure information without requiring the user to run an MPI
   program 
*/
const char MPIR_Version_string[]       = MPICH2_VERSION;
const char MPIR_Version_date[]         = MPICH2_VERSION_DATE;
const char MPIR_Version_configure[]    = MPICH2_CONFIGURE_ARGS_CLEAN;
const char MPIR_Version_device[]       = MPICH2_DEVICE;
const char MPIR_Version_CC[]           = MPICH2_COMPILER_CC;
const char MPIR_Version_CXX[]          = MPICH2_COMPILER_CXX;
const char MPIR_Version_F77[]          = MPICH2_COMPILER_F77;
const char MPIR_Version_FC[]           = MPICH2_COMPILER_FC;
