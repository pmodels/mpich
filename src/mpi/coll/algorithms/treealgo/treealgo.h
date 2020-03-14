/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TREEALGO_H_INCLUDED
#define TREEALGO_H_INCLUDED

#include "treealgo_types.h"

int MPII_Treealgo_init(void);
int MPII_Treealgo_comm_init(MPIR_Comm * comm);
int MPII_Treealgo_comm_cleanup(MPIR_Comm * comm);
int MPIR_Treealgo_tree_create(int rank, int nranks, int tree_type, int k, int root,
                              MPIR_Treealgo_tree_t * ct);
void MPIR_Treealgo_tree_free(MPIR_Treealgo_tree_t * tree);

#endif /* TREEALGO_H_INCLUDED */
