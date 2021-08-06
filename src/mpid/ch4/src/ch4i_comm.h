/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4I_COMM_H_INCLUDED
#define CH4I_COMM_H_INCLUDED

#include "ch4_types.h"

int MPIDI_comm_create_rank_map(MPIR_Comm * comm);
int MPIDI_check_disjoint_gpids(uint64_t gpids1[], int n1, uint64_t gpids2[], int n2);

#endif /* CH4I_COMM_H_INCLUDED */
