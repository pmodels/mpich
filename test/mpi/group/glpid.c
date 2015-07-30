/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpiimpl.h"

int main(int argc, char *argv[])
{
    MPID_Group group, *group_ptr = &group;
    int i;

    MPI_Init(&argc, &argv);

    /* Setup a sample group */
    group.handle = 1;
    group.ref_count = 1;
    group.size = 4;
    group.rank = 0;
    group.idx_of_first_lpid = -1;
    group.lrank_to_lpid = (MPID_Group_pmap_t *)
        MPIU_Malloc(group.size * sizeof(MPID_Group_pmap_t));
    for (i = 0; i < group.size; i++) {
        group.lrank_to_lpid[i].lrank = i;
        group.lrank_to_lpid[i].lpid = group.size - i - 1;
        group.lrank_to_lpid[i].next_lpid = -1;
        group.lrank_to_lpid[i].flag = 0;
    }

    /* Set up the group lpid list */
    MPIR_Group_setup_lpid_list(group_ptr);

    /* Print the group structure */
    printf("Index of first lpid = %d\n", group.idx_of_first_lpid);
    for (i = 0; i < group.size; i++) {
        printf("lrank_to_lpid[%d].next_lpid = %d, .lpid = %d\n",
               i, group.lrank_to_lpid[i].next_lpid, group.lrank_to_lpid[i].lpid);
    }

    MPI_Finalize();
    return 0;
}
