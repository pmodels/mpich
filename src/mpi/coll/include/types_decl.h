/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef MPIC_TYPES_DECL_H_INCLUDED
#define MPIC_TYPES_DECL_H_INCLUDED
#define GLOBAL_NAME    MPIC_

#include "../transports/stub/transport_types.h"
#include "../transports/mpich/transport_types.h"

/* Algorithms with STUB transport */
#include "../transports/stub/api_def.h"
#include "../algorithms/stub/pre.h"
#include "../algorithms/tree/pre.h"
#include "../src/tsp_namespace_undef.h"

/* Algorithms with MPICH transport */
#include "../transports/mpich/api_def.h"
#include "../algorithms/stub/pre.h"
#include "../algorithms/tree/pre.h"
#include "../src/tsp_namespace_undef.h"

/* Default Algorithms */
/* Nothing to be inclued */

#undef GLOBAL_NAME
#endif
