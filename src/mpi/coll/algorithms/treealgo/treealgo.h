/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef TREEALGO_H_INCLUDED
#define TREEALGO_H_INCLUDED

#include "treealgo_types.h"

int MPII_Treealgo_init(void);
int MPII_Treealgo_comm_init(MPIR_Comm * comm);
int MPII_Treealgo_comm_cleanup(MPIR_Comm * comm);
int MPII_Treealgo_tree_create(int rank, int nranks, int tree_type, int k, int root,
                              MPII_Treealgo_tree_t * ct);
void MPII_Treealgo_tree_free(MPII_Treealgo_tree_t * tree);

#endif /* TREEALGO_H_INCLUDED */
