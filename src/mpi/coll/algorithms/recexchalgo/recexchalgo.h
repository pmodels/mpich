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
#endif /* RECEXCHALGO_H_INCLUDED */
