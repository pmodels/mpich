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
#include "../transports/bmpich/transport_types.h"

#include "../transports/stub/api_def.h"
#include "../algorithms/stub/pre.h"
#include "../algorithms/tree/kary_pre.h"
#include "../algorithms/tree/knomial_pre.h"
#include "../algorithms/recexch/pre.h"
#include "../algorithms/dissem/pre.h"
#include "../src/tsp_namespace_undef.h"

#include "../transports/mpich/api_def.h"
#include "../algorithms/stub/pre.h"
#include "../algorithms/tree/kary_pre.h"
#include "../algorithms/tree/knomial_pre.h"
#include "../algorithms/recexch/pre.h"
#include "../algorithms/dissem/pre.h"
#include "../src/tsp_namespace_undef.h"

#include "../transports/bmpich/api_def.h"
#include "../algorithms/tree/kary_pre.h"
#include "../algorithms/tree/knomial_pre.h"
#include "../src/tsp_namespace_undef.h"

#define TRANSPORT_NAME X_
#define TRANSPORT_NAME_LC x
#include "../algorithms/treebasic/pre.h"
#undef TRANSPORT_NAME
#undef TRANSPORT_NAME_LC

#undef GLOBAL_NAME
#endif
