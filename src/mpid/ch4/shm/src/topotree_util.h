/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TOPOTREE_UTIL_H_INCLUDED
#define TOPOTREE_UTIL_H_INCLUDED

void MPIDI_SHM_print_topotree(const char *s, MPIDI_SHM_topotree_t * t);
void MPIDI_SHM_print_topotree_file(const char *s, int rand_val, int rank,
                                   MPIR_Treealgo_tree_t * my_tree);
int MPIDI_SHM_topotree_allocate(MPIDI_SHM_topotree_t * t, int n, int k);

#endif /* TOPOTREE_UTIL_H_INCLUDED */
