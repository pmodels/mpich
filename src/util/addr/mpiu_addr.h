/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

int MPIU_addr_table_create(int size, int rank, int local_size, int local_rank,
                           int local_leader, void *addr, int addr_len,
                           void **addr_table);
int MPIU_addr_table_destroy(void *addr_table, int local_size);
