/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef RECEXCHALGO_H_INCLUDED
#define RECEXCHALGO_H_INCLUDED

int MPII_Recexchalgo_init(void);
int MPII_Recexchalgo_comm_init(MPIR_Comm * comm);
int MPII_Recexchalgo_comm_cleanup(MPIR_Comm * comm);
int MPII_Recexchalgo_get_neighbors(int rank, int nranks, int *k_, int *step1_sendto,
                                   int **step1_recvfrom_, int *step1_nrecvs,
                                   int ***step2_nbrs_, int *step2_nphases, int *p_of_k_, int *T_);
/* Converts original rank to rank in step 2 of recursive exchange */
int MPII_Recexchalgo_origrank_to_step2rank(int rank, int rem, int T, int k);
/* Converts rank in step 2 of recursive exchange to original rank */
int MPII_Recexchalgo_step2rank_to_origrank(int rank, int rem, int T, int k);
int MPII_Recexchalgo_get_count_and_offset(int rank, int phase, int k, int nranks, int *count,
                                          int *offset);
int MPII_Recexchalgo_reverse_digits_step2(int rank, int comm_size, int k);
#endif /* RECEXCHALGO_H_INCLUDED */
