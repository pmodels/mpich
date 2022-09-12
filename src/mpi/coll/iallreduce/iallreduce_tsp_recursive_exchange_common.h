/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IALLREDUCE_TSP_RECURSIVE_EXCHANGE_COMMON_H_INCLUDED
#define IALLREDUCE_TSP_RECURSIVE_EXCHANGE_COMMON_H_INCLUDED

int MPIR_TSP_Iallreduce_sched_intra_recexch_step1(const void *sendbuf,
                                                  void *recvbuf,
                                                  MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  MPI_Op op, int is_commutative, int tag,
                                                  MPI_Aint extent, int dtcopy_id, int *recv_id,
                                                  int *reduce_id, int *vtcs, int is_inplace,
                                                  int step1_sendto, bool in_step2, int step1_nrecvs,
                                                  int *step1_recvfrom, int per_nbr_buffer,
                                                  void ***step1_recvbuf_, MPIR_Comm * comm,
                                                  MPIR_TSP_sched_t sched);
#endif /* IALLREDUCE_TSP_RECURSIVE_EXCHANGE_COMMON_H_INCLUDED */
