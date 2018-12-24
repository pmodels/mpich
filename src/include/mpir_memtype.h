/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_MEMTYPE_H_INCLUDED
#define MPIR_MEMTYPE_H_INCLUDED

typedef enum MPIR_Memtype {
    MPIR_MEMTYPE__NONE = -1,
    MPIR_MEMTYPE__DDR = 0,
    MPIR_MEMTYPE__MCDRAM,
    MPIR_MEMTYPE__NUM,
    MPIR_MEMTYPE__DEFAULT = MPIR_MEMTYPE__DDR
} MPIR_Memtype;

#endif /* MPIR_MEMTYPE_H_INCLUDED */
