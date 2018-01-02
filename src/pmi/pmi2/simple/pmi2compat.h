/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI2COMPAT_H_INCLUDED
#define PMI2COMPAT_H_INCLUDED

#include "mpiimpl.h"

#define PMI2U_Malloc(size_) MPL_malloc(size_, MPL_MEM_PM)
#define PMI2U_Free MPL_free
#define PMI2U_Strdup MPL_strdup
#define PMI2U_Strnapp MPL_strnapp
#define PMI2U_Assert MPIR_Assert
#define PMI2U_Exit MPL_exit
#define PMI2U_Info MPIR_Info
#define PMI2U_Memcpy MPIR_Memcpy

#endif /* PMI2COMPAT_H_INCLUDED */
