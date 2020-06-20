/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_BC_H_INCLUDED
#define MPIDU_BC_H_INCLUDED

/* In case of systems with variable business card lengths,
 *set MPID_MAX_BC_SIZE to maximum possible bc size */
#define MPID_MAX_BC_SIZE 4096

int MPIDU_bc_table_create(int rank, int size, int *nodemap, void *bc, int bc_len, int same_len,
                          int roots_only, void **bc_table, int *ret_bc_len);
int MPIDU_bc_table_destroy(void);
int MPIDU_bc_allgather(MPIR_Comm * allgather_comm, void *bc, int bc_len, int same_len,
                       void **bc_table, int **rank_map, int *ret_bc_len);

#endif /* MPIDU_BC_H_INCLUDED */
