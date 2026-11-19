/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adio.h"
#include "adio_extern.h"

#include <pthread.h>
/* Function for running in another thread for doing the file reading while the
 * main thread is doing data aggregation - useful only when multiple rounds are
 * needed due to file size relative to the read buffer size and number of
 * aggregators */

void *ADIOI_IO_Thread_Func(void *vptr_args)
{
    ADIOI_IO_ThreadFuncData *args = (ADIOI_IO_ThreadFuncData *) vptr_args;

    ADIOI_Assert(args->size == (int) (args->size));

    if (args->io_kind == ADIOI_READ) {
        ADIO_ReadContig(args->fd, args->buf, args->size, MPI_BYTE,
                        ADIO_EXPLICIT_OFFSET, args->offset, args->status, &(args->error_code));
    } else {
        ADIO_WriteContig(args->fd, args->buf, args->size, MPI_BYTE,
                         ADIO_EXPLICIT_OFFSET, args->offset, args->status, &(args->error_code));
    }
    pthread_exit(&(args->error_code));
    return NULL;
}

void ADIOI_IO_Thread_init(ADIOI_IO_Thread * t)
{
    t->active = 0;
}

/* Wait for the write in flight, if any, and return its error code. */
int ADIOI_IO_Thread_wait(ADIOI_IO_Thread * t)
{
    if (!t->active)
        return MPI_SUCCESS;
    pthread_join(t->thread, NULL);
    t->active = 0;
    return t->args.error_code;
}

/* Write size bytes of *buf at offset in a background thread, once the
 * previous write has completed, and swap *buf and *next_buf so that the
 * caller fills the other buffer meanwhile.  If no thread can be created, the
 * write is done synchronously.  Returns the first error of the previous write
 * or of this one; nothing is written if the previous write failed. */
int ADIOI_IO_Thread_write(ADIOI_IO_Thread * t, ADIO_File fd, char **buf, char **next_buf,
                          MPI_Count size, ADIO_Offset offset)
{
    char *tmp;
    int err;

    err = ADIOI_IO_Thread_wait(t);
    if (err != MPI_SUCCESS)
        return err;

    t->args.fd = fd;
    t->args.io_kind = ADIOI_WRITE;
    t->args.buf = *buf;
    t->args.size = size;
    t->args.offset = offset;
    t->args.status = &t->status;
    t->args.error_code = MPI_SUCCESS;

    tmp = *buf;
    *buf = *next_buf;
    *next_buf = tmp;

    if (pthread_create(&t->thread, NULL, ADIOI_IO_Thread_Func, &t->args) == 0) {
        t->active = 1;
        return MPI_SUCCESS;
    }
    ADIO_WriteContig(fd, t->args.buf, size, MPI_BYTE, ADIO_EXPLICIT_OFFSET, offset,
                     &t->status, &err);
    return err;
}
