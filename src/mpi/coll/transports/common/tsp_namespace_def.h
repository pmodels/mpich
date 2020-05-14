/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
