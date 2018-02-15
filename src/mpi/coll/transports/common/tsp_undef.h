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

/*
 * Header protection (i.e., TSP_UNDEF_H_INCLUDED) is intentionally
 * omitted since this header might get included multiple times within
 * the same .c file.  Here's one possible usage model:
 *
 * #include "tsp_gentran.h"
 * #include "algo_impl.h"
 * #include "tsp_undef.h"
 *
 * #include "tsp_gentran2.h"
 * #include "algo_impl.h"
 * #include "tsp_undef.h"
 *
 * In this case, the same algo is instantiated multiple times with
 * different transports.
 */

#undef MPIR_TSP_TRANSPORT_NAME

/* Transport Types */
#undef MPIR_TSP_sched_t

/* Transport API */
#undef MPIR_TSP_sched_create
#undef MPIR_TSP_sched_isend
#undef MPIR_TSP_sched_irecv
#undef MPIR_TSP_sched_imcast
#undef MPIR_TSP_sched_start
