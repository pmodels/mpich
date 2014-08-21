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
static int *group_to_bitarray(MPID_Group *group, MPID_Comm *orig_comm) {
    uint32_t *bitarray, mask;
    int bitarray_size = (orig_comm->local_size / 8) + (orig_comm->local_size % 8 ? 1 : 0);
    int *group_ranks, *comm_ranks, i, index;

    bitarray = (int *) MPIU_Malloc(sizeof(int) * bitarray_size);

    if (group == MPID_Group_empty) {
        for (i = 0; i < bitarray_size; i++) bitarray[i] = 0;
        return bitarray;
    }

    group_ranks = (int *) MPIU_Malloc(sizeof(int) * group->size);
    comm_ranks = (int *) MPIU_Malloc(sizeof(int) * group->size);

    for (i = 0; i < group->size; i++) group_ranks[i] = i;
    for (i = 0; i < bitarray_size; i++) bitarray[i] = 0;

    MPIR_Group_translate_ranks_impl(group, group->size, group_ranks,
                                    orig_comm->local_group, comm_ranks);

    for (i = 0; i < group->size && comm_ranks[i] != MPI_UNDEFINED; i++) {
        index = comm_ranks[i] / 32;
        mask = 0x80000000 >> comm_ranks[i] % 32;
        bitarray[index] |= mask;
    }

    MPIU_Free(group_ranks);
    MPIU_Free(comm_ranks);

    return bitarray;
}

/* Generates an MPID_Group from a bitarray */
static MPID_Group *bitarray_to_group(MPID_Comm *comm_ptr, uint32_t *bitarray)
{
    MPID_Group *ret_group;
    MPID_Group *comm_group;
    UT_array *ranks_array;
    int i, found = 0;

    utarray_new(ranks_array, &ut_int_icd);

    MPIR_Comm_group_impl(comm_ptr, &comm_group);

    /* Converts the bitarray into a utarray */
    for (i = 0; i < comm_ptr->local_size; i++) {
        if (bitarray[i/32] & (0x80000000 >> (i % 32))) {
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
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Comm_get_all_failed_procs(MPID_Comm *comm_ptr, MPID_Group **failed_group, int tag)
{
    int mpi_errno = MPI_SUCCESS;
    int errflag = 0;
    int i, j, bitarray_size;
    uint32_t *bitarray, *remote_bitarray;
    MPID_Group *local_fail;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_GET_ALL_FAILED_PROCS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_COMM_GET_ALL_FAILED_PROCS);

    /* Kick the progress engine in case it's been a while so we get all the
     * latest updates about failures */
    MPID_Progress_poke();

    /* Generate the list of failed processes */
    MPIDI_CH3U_Check_for_failed_procs();

    mpi_errno = MPIDI_CH3U_Get_failed_group(-2, &local_fail);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Generate a bitarray based on the list of failed procs */
    bitarray = group_to_bitarray(local_fail, comm_ptr);
    bitarray_size = (comm_ptr->local_size / 8) + (comm_ptr->local_size % 8 ? 1 : 0);
    remote_bitarray = MPIU_Malloc(sizeof(uint32_t) * bitarray_size);

    /* For now, this will be implemented as a star with rank 0 serving as
     * the source */
    if (comm_ptr->rank == 0) {
        for (i = 1; i < comm_ptr->local_size; i++) {
            /* Get everyone's list of failed processes to aggregate */
            mpi_errno = MPIC_Recv(remote_bitarray, bitarray_size, MPI_UINT32_T,
                i, tag, comm_ptr->handle, MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno) continue;

            /* Combine the received bitarray with my own */
            for (j = 0; j < bitarray_size; j++) {
                if (remote_bitarray[j] != 0) {
                    bitarray[j] |= remote_bitarray[j];
                }
            }
        }

        for (i = 1; i < comm_ptr->local_size; i++) {
            /* Send the list to each rank to be processed locally */
            mpi_errno = MPIC_Send(bitarray, bitarray_size, MPI_UINT32_T, i,
                tag, comm_ptr->handle, &errflag);
            if (mpi_errno) errflag = 1;
        }

        /* Convert the bitarray into a group */
        *failed_group = bitarray_to_group(comm_ptr, bitarray);
    } else {
        /* Send my bitarray to rank 0 */
        mpi_errno = MPIC_Send(bitarray, bitarray_size, MPI_UINT32_T, 0,
            tag, comm_ptr->handle, &errflag);
        if (mpi_errno) errflag = 1;

        /* Get the resulting bitarray back from rank 0 */
        mpi_errno = MPIC_Recv(remote_bitarray, bitarray_size, MPI_UINT32_T, 0,
            tag, comm_ptr->handle, MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno) errflag = 1;

        /* Convert the bitarray into a group */
        *failed_group = bitarray_to_group(comm_ptr, remote_bitarray);
    }

    MPIU_Free(bitarray);
    MPIU_Free(remote_bitarray);

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_COMM_GET_ALL_FAILED_PROCS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
