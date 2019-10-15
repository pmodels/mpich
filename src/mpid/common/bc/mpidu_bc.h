/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_BC_H_INCLUDED
#define MPIDU_BC_H_INCLUDED

#include "mpir_nodemap.h"

int MPIDU_bc_table_create(int rank, int size, int *nodemap, void *bc, int bc_len, int same_len,
                          int roots_only, void **bc_table, int *ret_bc_len);
int MPIDU_bc_table_destroy(void);
int MPIDU_bc_allgather(MPIR_Comm * allgather_comm, void *bc, int bc_len, int same_len,
                       void **bc_table, int **rank_map, int *ret_bc_len);

#endif /* MPIDU_BC_H_INCLUDED */
