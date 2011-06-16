/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpix.h"
#include <stdio.h>
#include <stdlib.h>

/*@

MPIX_Group_comm_create - Creates a new communicator from a group

Input Parameters:
+ comm - communicator (handle)
- group - group, which is a subset of the group of 'comm'  (handle)
- tag - tag to distinguish group creation in threaded environments

Output Parameter:
. new_comm - new communicator (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_GROUP

@*/
int MPIX_Group_comm_create(MPI_Comm old_comm, MPI_Group group, int tag, MPI_Comm * new_comm)
{
    int i, grp_me, me, nproc, merge_size;
    MPI_Comm comm, inter_comm;
    MPI_Group old_group = MPI_GROUP_NULL;
    int *granks, *ranks, rank_count;
    int ret = MPI_SUCCESS;

    /* Create a group for all of old_comm and translate it to "group",
     * which should be a subset of old_comm's group. If implemented
     * inside MPI, the below translation can be done more efficiently,
     * since we have access to the internal lpids directly. */
    ret = MPI_Comm_group(old_comm, &old_group);
    if (ret) goto fn_fail;

    ret = MPI_Group_size(group, &rank_count);
    if (ret) goto fn_fail;

    granks = malloc(rank_count * sizeof(int));
    ranks = malloc(rank_count * sizeof(int));
    for (i = 0; i < rank_count; i++)
        granks[i] = i;

    ret = MPI_Group_translate_ranks(group, rank_count, granks, old_group, ranks);
    if (ret) goto fn_fail;

    free(granks);

    ret = MPI_Comm_rank(old_comm, &me);
    if (ret) goto fn_fail;

    ret = MPI_Comm_size(old_comm, &nproc);
    if (ret) goto fn_fail;

    /* If the group size is 0, return MPI_COMM_NULL */
    if (rank_count == 0) {
        *new_comm = MPI_COMM_NULL;
        goto fn_exit;
    }

    /* If I am the only process in the group, return a dup of
     * MPI_COMM_SELF */
    if (rank_count == 1 && ranks[0] == me) {
        ret = MPI_Comm_dup(MPI_COMM_SELF, new_comm);
        if (ret) goto fn_fail;

        goto fn_exit;
    }

    /* If I am not a part of the group, return MPI_COMM_NULL */
    grp_me = -1;
    for (i = 0; i < rank_count; i++) {
        if (ranks[i] == me) {
            grp_me = i;
            break;
        }
    }
    if (grp_me < 0) {
        *new_comm = MPI_COMM_NULL;
        goto fn_exit;
    }

    ret = MPI_Comm_dup(MPI_COMM_SELF, &comm);
    if (ret) goto fn_fail;

    for (merge_size = 1; merge_size < rank_count; merge_size *= 2) {
        int gid = grp_me / merge_size;
        MPI_Comm save_comm = comm;

        if (gid % 2 == 0) {
            /* Check if right partner doesn't exist */
            if ((gid + 1) * merge_size >= rank_count)
                continue;

            ret = MPI_Intercomm_create(comm, 0, old_comm, ranks[(gid + 1) * merge_size],
                                       tag, &inter_comm);
            if (ret) goto fn_fail;

            ret = MPI_Intercomm_merge(inter_comm, 0 /* LOW */ , &comm);
            if (ret) goto fn_fail;
        }
        else {
            ret = MPI_Intercomm_create(comm, 0, old_comm, ranks[(gid - 1) * merge_size],
                                       tag, &inter_comm);
            if (ret) goto fn_fail;

            ret = MPI_Intercomm_merge(inter_comm, 1 /* HIGH */ , &comm);
            if (ret) goto fn_fail;
        }

        ret = MPI_Comm_free(&inter_comm);
        if (ret) goto fn_fail;

        if (save_comm != MPI_COMM_SELF) {
            ret = MPI_Comm_free(&save_comm);
            if (ret) goto fn_fail;
        }
    }

    *new_comm = comm;

  fn_exit:
    free(ranks);
    if (old_group != MPI_GROUP_NULL) {
        if (ret) /* avoid squashing a more interesting error code */
            MPI_Group_free(&old_group);
        else
            ret = MPI_Group_free(&old_group);
    }
    return ret;

  fn_fail:
    goto fn_exit;
}
