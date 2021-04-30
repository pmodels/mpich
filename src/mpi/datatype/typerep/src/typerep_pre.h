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
typedef yaksa_request_t MPIR_Typerep_req;
#define MPIR_TYPEREP_REQ_NULL YAKSA_REQUEST__NULL
#else
typedef int MPIR_Typerep_req;   /* unused in dataloop */
#define MPIR_TYPEREP_REQ_NULL 0
#endif

#endif /* TYPEREP_PRE_H_INCLUDED */
