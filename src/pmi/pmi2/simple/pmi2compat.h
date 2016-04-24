/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#define PMI2U_Malloc MPL_malloc
#define PMI2U_Free MPL_free
#define PMI2U_Strdup MPL_strdup
#define PMI2U_Strnapp MPL_strnapp
#define PMI2U_Assert MPIR_Assert
#define PMI2U_Exit MPL_exit
#define PMI2U_Info MPIR_Info
#define PMI2U_Memcpy MPIR_Memcpy

