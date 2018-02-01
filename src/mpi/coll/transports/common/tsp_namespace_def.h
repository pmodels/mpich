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

#ifndef TSP_NAMESPACE_DEF_H_INCLUDED
#define TSP_NAMESPACE_DEF_H_INCLUDED

#ifndef MPIR_TSP_TRANSPORT_NAME
#error "MPIR_TSP_TRANSPORT_NAME must be defined before this file is included"
#endif

#define NSCAT0(a,b) a##b
#define NSCAT1(a,b) NSCAT0(a,b)

#define MPIR_TSP_NAMESPACE(fn) NSCAT1(NSCAT1(MPII_, MPIR_TSP_TRANSPORT_NAME), fn)

#endif /* TSP_NAMESPACE_DEF_H_INCLUDED */
