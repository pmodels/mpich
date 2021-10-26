/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IALLREDUCE_TSP_RECURSIVE_EXCHANGE_COMMON_H_INCLUDED
#define IALLREDUCE_TSP_RECURSIVE_EXCHANGE_COMMON_H_INCLUDED

int MPIR_TSP_Iallreduce_sched_intra_recexch_step1(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm,
                                                  int tag,
                                                  MPI_Aint dt_size, int recvbuf_ready,
                                                  MPII_Recexchalgo_t * recexch,
                                                  MPIR_TSP_sched_t sched);

int MPIR_TSP_Iallreduce_sched_intra_recexch_step3(void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype,
                                                  MPIR_Comm * comm, int tag,
                                                  MPII_Recexchalgo_t * recexch,
                                                  MPIR_TSP_sched_t sched);

int MPIR_TSP_Iallreduce_sched_intra_recexch_step1_zero(MPIR_Comm * comm, int tag,
                                                       MPII_Recexchalgo_t * recexch,
                                                       MPIR_TSP_sched_t sched);

int MPIR_TSP_Iallreduce_sched_intra_recexch_step3_zero(MPIR_Comm * comm, int tag,
                                                       MPII_Recexchalgo_t * recexch,
                                                       MPIR_TSP_sched_t sched);

int MPIR_TSP_Iallreduce_recexch_alloc_nbr_bufs(MPI_Aint dt_size,
                                               MPII_Recexchalgo_t * recexch,
                                               MPIR_TSP_sched_t sched);

void *MPIR_TSP_Iallreduce_recexch_get_nbr_buf(MPII_Recexchalgo_t * recexch, int phase, int i);

int MPIR_TSP_Iallreduce_recexch_check_commutative(MPII_Recexchalgo_t * recexch, int myrank);
#endif /* IALLREDUCE_TSP_RECURSIVE_EXCHANGE_COMMON_H_INCLUDED */
