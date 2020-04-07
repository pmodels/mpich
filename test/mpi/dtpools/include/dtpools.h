/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DTPOOLS_H_INCLUDED
#define DTPOOLS_H_INCLUDED

/* errors */
#define DTP_SUCCESS                (0)
#define DTP_ERR_ARG                (-1)
#define DTP_ERR_OUT_OF_RESOURCES   (-2)
#define DTP_ERR_MPI                (-3)
#define DTP_ERR_OTHER              (-4)

typedef struct {
    MPI_Datatype DTP_datatype;
    MPI_Aint DTP_type_count;

    MPI_Aint DTP_bufsize;
    MPI_Aint DTP_buf_offset;

    void *priv;
} DTP_obj_s;

typedef struct {
    MPI_Datatype DTP_base_type;

    void *priv;
} DTP_pool_s;

/* DTP manipulation functions */
int DTP_pool_create(const char *basic_type_str, MPI_Aint basic_type_count, int seed,
                    DTP_pool_s * dtp);
int DTP_pool_free(DTP_pool_s dtp);

int DTP_obj_create(DTP_pool_s dtp, DTP_obj_s * obj, MPI_Aint maxbufsize);
int DTP_obj_free(DTP_obj_s obj);
int DTP_obj_get_description(DTP_obj_s obj, char **desc);

int DTP_obj_buf_init(DTP_obj_s obj, void *buf, int val_start, int val_stride, MPI_Aint val_count);
int DTP_obj_buf_check(DTP_obj_s obj, void *buf, int val_start, int val_stride, MPI_Aint val_count);

#endif /* DTPOOLS_H_INCLUDED */
