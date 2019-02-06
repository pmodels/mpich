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

/* Header protection (i.e., IALLREDUCE_TSP_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Iallreduce_sched_intra_recexch_step1
#define MPIR_TSP_Iallreduce_sched_intra_recexch_step1          MPIR_TSP_NAMESPACE(Iallreduce_sched_intra_recexch_step1)

int MPIR_TSP_Iallreduce_sched_intra_recexch_step1(const void *sendbuf,
                                                  void *recvbuf,
                                                  int count,
                                                  MPI_Datatype datatype,
                                                  MPI_Op op, int is_commutative, int tag,
                                                  int extent, int dtcopy_id, int *recv_id,
                                                  int *reduce_id, int *vtcs, int is_inplace,
                                                  int step1_sendto, bool in_step2, int step1_nrecvs,
                                                  int *step1_recvfrom, int per_nbr_buffer,
                                                  void ***step1_recvbuf_, MPIR_Comm * comm,
                                                  MPIR_TSP_sched_t * sched);
