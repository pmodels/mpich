/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef WIN_SYNC_H_INCLUDED
#define WIN_SYNC_H_INCLUDED

#define CHECK_ERR(stmt)                                                               \
    do {                                                                                \
        int err_class, err, rank;                                                       \
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);                                           \
        err = stmt;                                                                     \
        if (err == MPI_SUCCESS) {                                                       \
            printf("%d: Operation succeeded, when it should have failed\n", rank);      \
            errors++;                                                                   \
        } else {                                                                        \
            MPI_Error_class(err, &err_class);                                         \
            if (err_class != MPI_ERR_RMA_SYNC)  {                                       \
                char str[MPI_MAX_ERROR_STRING];                                         \
                int  len;                                                               \
                MPI_Error_string(err, str, &len);                                       \
                printf("%d: Expected MPI_ERR_RMA_SYNC, got:\n%s\n", rank, str);         \
                errors++;                                                               \
            }                                                                           \
        }                                                                               \
    } while (0)

#endif
