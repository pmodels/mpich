/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "topotree_types.h"
#include "topotree_util.h"

/*
 * Utility function to print the tree structure to the console.
 * */
void MPIDI_SHM_print_topotree(const char *s, MPIDI_SHM_topotree_t * t)
{
    int i, j;
    for (i = 0; i < t->n; ++i) {
        fprintf(stderr, "%s,Tree,%d,Parent,%d,#Child,%d[", s, i, MPIDI_SHM_TOPOTREE_PARENT(t, i),
                MPIDI_SHM_TOPOTREE_NUM_CHILD(t, i));
        for (j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(t, i); ++j) {
            fprintf(stderr, ",%d", MPIDI_SHM_TOPOTREE_CHILD(t, i, j));
        }
        fprintf(stderr, "]\n");
    }
}

/*
 * This code generates a file per rank for the tree, which is then accumulated by the tree-vis
 * utilit (external) and generates a single tree for visualization/debugging.
 * */
void MPIDI_SHM_print_topotree_file(const char *s, int rand_val, int rank,
                                   MPIR_Treealgo_tree_t * my_tree)
{
    FILE *file;
    char fname[256];
    int c;

    sprintf(fname, "FILE_%s_%d__%d_%d_.tree", s, rand_val, getpid(), rank);
    file = fopen(fname, "w");
    if (file) {
        fprintf(file, "Children:: ");
        int *cptr = (int *) utarray_eltptr(my_tree->children, 0);
        for (c = 0; c < my_tree->num_children; ++c) {
            fprintf(file, " %d", cptr[c]);
        }
        fprintf(file, "\nParent:: %d", my_tree->parent);
        fclose(file);
    }
}

int MPIDI_SHM_topotree_allocate(MPIDI_SHM_topotree_t * t, int n, int k)
{
    int mpi_errno = MPI_SUCCESS;
    t->k = k;
    t->n = n;

    t->base = (int *) (MPL_calloc((k + 2) * n, sizeof(int), MPL_MEM_OTHER));
    MPIR_ERR_CHKANDJUMP(!t->base, mpi_errno, MPI_ERR_OTHER, "**nomem");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
