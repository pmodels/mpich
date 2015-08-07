/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

/* Generates a bitarray based on orig_comm where all procs in group are marked with 1 */
static void group_to_bitarray(MPID_Group *group, MPID_Comm *orig_comm, int **bitarray, int *bitarray_size) {
    int mask;
    int *group_ranks, *comm_ranks, i, index;

    /* Calculate the bitarray size in ints and allocate space */
    *bitarray_size = (orig_comm->local_size / (8 * sizeof(int)) + (orig_comm->local_size % (8 * sizeof(int)) ? 1 : 0));
    *bitarray = (int *) MPIU_Malloc(sizeof(int) * *bitarray_size);

    /* If the group is empty, return an empty bitarray. */
    if (group == MPID_Group_empty) {
        for (i = 0; i < *bitarray_size; i++) *bitarray[i] = 0;
        return;
    }

    /* Get the ranks of group in orig_comm */
    group_ranks = (int *) MPIU_Malloc(sizeof(int) * group->size);
    comm_ranks = (int *) MPIU_Malloc(sizeof(int) * group->size);

    for (i = 0; i < group->size; i++) group_ranks[i] = i;
    for (i = 0; i < *bitarray_size; i++) *bitarray[i] = 0;

    MPIR_Group_translate_ranks_impl(group, group->size, group_ranks,
                                    orig_comm->local_group, comm_ranks);

    /* For each process in the group, shift a bit to the correct location and
     * add it to the bitarray. */
    for (i = 0; i < group->size ; i++) {
        if (comm_ranks[i] == MPI_UNDEFINED) continue;
        index = comm_ranks[i] / (sizeof(int) * 8);
        mask = 0x1 << comm_ranks[i] % (sizeof(int) * 8);
        *bitarray[index] |= mask;
    }

    MPIU_Free(group_ranks);
    MPIU_Free(comm_ranks);
}

/* Generates an MPID_Group from a bitarray */
static MPID_Group *bitarray_to_group(MPID_Comm *comm_ptr, int *bitarray)
{
    MPID_Group *ret_group;
    MPID_Group *comm_group;
    UT_array *ranks_array;
    int i, found = 0;

    /* Create a utarray to make storing the ranks easier */
    utarray_new(ranks_array, &ut_int_icd);

    MPIR_Comm_group_impl(comm_ptr, &comm_group);

    /* Converts the bitarray into a utarray */
    for (i = 0; i < comm_ptr->local_size; i++) {
        if (bitarray[i / (sizeof(int) * 8)] & (0x1 << (i % (sizeof(int) * 8)))) {
            utarray_push_back(ranks_array, &i);
            found++;
        }
    }

    if (found)
        /* Converts the utarray into a group */
        MPIR_Group_incl_impl(comm_group, found, ut_int_array(ranks_array), &ret_group);
    else
        ret_group = MPID_Group_empty;

    utarray_free(ranks_array);
    MPIR_Group_release(comm_group);

    return ret_group;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_get_all_failed_procs
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Comm_get_all_failed_procs(MPID_Comm *comm_ptr, MPID_Group **failed_group, int tag)
{
    int mpi_errno = MPI_SUCCESS, ret_errno;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int i, j, bitarray_size;
    int *bitarray, *remote_bitarray;
    MPID_Group *local_fail;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_GET_ALL_FAILED_PROCS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_GET_ALL_FAILED_PROCS);

    /* Kick the progress engine in case it's been a while so we get all the
     * latest updates about failures */
    MPID_Progress_poke();

    /* Generate the list of failed processes */
    MPIDI_CH3U_Check_for_failed_procs();

    mpi_errno = MPIDI_CH3U_Get_failed_group(-2, &local_fail);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Generate a bitarray based on the list of failed procs */
    group_to_bitarray(local_fail, comm_ptr, &bitarray, &bitarray_size);
    remote_bitarray = MPIU_Malloc(sizeof(int) * bitarray_size);
    if (local_fail != MPID_Group_empty)
        MPIR_Group_release(local_fail);

    /* For now, this will be implemented as a star with rank 0 serving as
     * the source */
    if (comm_ptr->rank == 0) {
        for (i = 1; i < comm_ptr->local_size; i++) {
            /* Get everyone's list of failed processes to aggregate */
            ret_errno = MPIC_Recv(remote_bitarray, bitarray_size, MPI_INT,
                i, tag, comm_ptr, MPI_STATUS_IGNORE, &errflag);
            if (ret_errno) continue;

            /* Combine the received bitarray with my own */
            for (j = 0; j < bitarray_size; j++) {
                if (remote_bitarray[j] != 0) {
                    bitarray[j] |= remote_bitarray[j];
                }
            }
        }

        for (i = 1; i < comm_ptr->local_size; i++) {
            /* Send the list to each rank to be processed locally */
            ret_errno = MPIC_Send(bitarray, bitarray_size, MPI_INT, i,
                tag, comm_ptr, &errflag);
            if (ret_errno) continue;
        }

        /* Convert the bitarray into a group */
        *failed_group = bitarray_to_group(comm_ptr, bitarray);
    } else {
        /* Send my bitarray to rank 0 */
        mpi_errno = MPIC_Send(bitarray, bitarray_size, MPI_INT, 0,
            tag, comm_ptr, &errflag);

        /* Get the resulting bitarray back from rank 0 */
        mpi_errno = MPIC_Recv(remote_bitarray, bitarray_size, MPI_INT, 0,
            tag, comm_ptr, MPI_STATUS_IGNORE, &errflag);

        /* Convert the bitarray into a group */
        *failed_group = bitarray_to_group(comm_ptr, remote_bitarray);
    }

    MPIU_Free(bitarray);
    MPIU_Free(remote_bitarray);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_GET_ALL_FAILED_PROCS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
