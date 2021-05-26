/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ch4r_recvq.h"

int unexp_message_indices[2];

int MPIDIG_recvq_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_PVAR_LEVEL_REGISTER_STATIC(RECVQ, MPI_UNSIGNED, posted_recvq_length, 0,      /* init value */
                                      MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",      /* category name */
                                      "length of the posted message receive queue");

    MPIR_T_PVAR_LEVEL_REGISTER_STATIC(RECVQ, MPI_UNSIGNED, unexpected_recvq_length, 0,  /* init value */
                                      MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",      /* category name */
                                      "length of the unexpected message receive queue");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(RECVQ, MPI_UNSIGNED_LONG_LONG, posted_recvq_match_attempts, MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",        /* category name */
                                        "number of search passes on the posted message receive queue");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(RECVQ,
                                        MPI_UNSIGNED_LONG_LONG,
                                        unexpected_recvq_match_attempts,
                                        MPI_T_VERBOSITY_USER_DETAIL,
                                        MPI_T_BIND_NO_OBJECT,
                                        (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS),
                                        "CH4",
                                        "number of search passes on the unexpected message receive queue");

    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RECVQ, MPI_DOUBLE, time_failed_matching_postedq, MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",     /* category name */
                                      "total time spent on unsuccessful search passes on the posted receives queue");

    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RECVQ, MPI_DOUBLE, time_matching_unexpectedq, MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",        /* category name */
                                      "total time spent on search passes on the unexpected receive queue");

    int source_index;
    MPIR_T_register_source("RECVQ", "active message receive queue", MPI_T_SOURCE_ORDERED, NULL, -1,
                           -1, &source_index);

    MPI_Datatype array_of_datatypes[2] = { MPI_INT, MPI_INT };
    MPI_Aint array_of_displacements[2] = { 0, 4 };
    MPIR_T_register_event(source_index, "unexp_message_enqueued", MPI_T_VERBOSITY_USER_BASIC,
                          array_of_datatypes, array_of_displacements, 2,
                          "message added to unexpected queue", MPI_T_BIND_NO_OBJECT, "CH4",
                          &unexp_message_indices[0]);
    MPIR_T_register_event(source_index, "unexp_message_dequeued", MPI_T_VERBOSITY_USER_BASIC,
                          array_of_datatypes, array_of_displacements, 2,
                          "message removed from unexpected queue", MPI_T_BIND_NO_OBJECT, "CH4",
                          &unexp_message_indices[1]);

    return mpi_errno;
}
