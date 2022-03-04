/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMI2COMPAT_H_INCLUDED
#define PMI2COMPAT_H_INCLUDED

#include <stdlib.h>
#include <assert.h>

#define PMI2U_Malloc(size_) MPL_malloc(size_, MPL_MEM_PM)
#define PMI2U_Free MPL_free
#define PMI2U_Strdup MPL_strdup
#define PMI2U_Strnapp MPL_strnapp
#define PMI2U_Assert assert
#define PMI2U_Exit MPL_exit
#define PMI2U_Memcpy memcpy

#endif /* PMI2COMPAT_H_INCLUDED */
