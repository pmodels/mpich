/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TYPEREP_PRE_H_INCLUDED
#define TYPEREP_PRE_H_INCLUDED

#define MPICH_DATATYPE_ENGINE_YAKSA    (1)
#define MPICH_DATATYPE_ENGINE_DATALOOP (2)

#if (MPICH_DATATYPE_ENGINE == MPICH_DATATYPE_ENGINE_YAKSA)
#include "yaksa.h"
typedef struct {
    yaksa_request_t req;
    yaksa_info_t info;
} MPIR_Typerep_req;
#define MPIR_TYPEREP_REQ_NULL YAKSA_REQUEST__NULL
#define MPIR_TYPEREP_HANDLE_TYPE yaksa_type_t
#define MPIR_TYPEREP_HANDLE_NULL YAKSA_TYPE__NULL
#else
typedef struct {
    int req;
    int info;
} MPIR_Typerep_req;             /* unused in dataloop */
#define MPIR_TYPEREP_REQ_NULL 0
#define MPIR_TYPEREP_HANDLE_TYPE struct MPII_Dataloop *
#define MPIR_TYPEREP_HANDLE_NULL NULL
#endif

int MPIR_Typerep_wait(MPIR_Typerep_req typerep_req);
int MPIR_Typerep_test(MPIR_Typerep_req typerep_req, int *completed);

#endif /* TYPEREP_PRE_H_INCLUDED */
