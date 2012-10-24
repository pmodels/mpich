/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpichinfo.h"
/* 
   Global definitions of variables that hold information about the
   version and patchlevel.  This allows easy access to the version 
   and configure information without requiring the user to run an MPI
   program 
*/
const char MPIR_Version_string[]       = MPICH_VERSION;
const char MPIR_Version_date[]         = MPICH_VERSION_DATE;
const char MPIR_Version_configure[]    = MPICH_CONFIGURE_ARGS_CLEAN;
const char MPIR_Version_device[]       = MPICH_DEVICE;
const char MPIR_Version_CC[]           = MPICH_COMPILER_CC;
const char MPIR_Version_CXX[]          = MPICH_COMPILER_CXX;
const char MPIR_Version_F77[]          = MPICH_COMPILER_F77;
const char MPIR_Version_FC[]           = MPICH_COMPILER_FC;
