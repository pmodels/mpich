/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
    yaksa_type_t DTP_datatype;
    uintptr_t DTP_type_count;

    uintptr_t DTP_bufsize;
    uintptr_t DTP_buf_offset;

    void *priv;
} DTP_obj_s;

typedef struct {
    yaksa_type_t DTP_base_type;

    void *priv;
} DTP_pool_s;

/* DTP manipulation functions */
int DTP_pool_create(const char *basic_type_str, uintptr_t basic_type_count, int seed,
                    DTP_pool_s * dtp);
int DTP_pool_free(DTP_pool_s dtp);

int DTP_obj_create(DTP_pool_s dtp, DTP_obj_s * obj, uintptr_t maxbufsize);
int DTP_obj_create_custom(DTP_pool_s dtp, DTP_obj_s * obj, const char *desc);
int DTP_obj_free(DTP_obj_s obj);
int DTP_obj_get_description(DTP_obj_s obj, char **desc);

int DTP_obj_buf_init(DTP_obj_s obj, void *buf, int val_start, int val_stride, uintptr_t val_count);
int DTP_obj_buf_check(DTP_obj_s obj, void *buf, int val_start, int val_stride, uintptr_t val_count);

#endif /* DTPOOLS_H_INCLUDED */
