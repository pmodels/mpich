/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PACK_COMMON_H
#define PACK_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "yaksa_config.h"
#include "yaksa.h"
#include "dtpools.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

typedef enum {
    MEM_TYPE__UNREGISTERED_HOST = 0,
    MEM_TYPE__REGISTERED_HOST,
    MEM_TYPE__MANAGED,
    MEM_TYPE__DEVICE,
    MEM_TYPE__NUM_MEMTYPES,
} mem_type_e;

extern const char *memtype_str[MEM_TYPE__NUM_MEMTYPES];

int pack_get_ndevices(void);
void pack_init_devices(int num_threads);
void pack_finalize_devices(void);
void pack_alloc_mem(int device_id, size_t size, mem_type_e type, void **hostbuf, void **devicebuf);
void pack_free_mem(mem_type_e type, void *hostbuf, void *devicebuf);
void pack_get_ptr_attr(const void *inbuf, void *outbuf, yaksa_info_t * info, int iter);
void pack_copy_content(int tid, const void *sbuf, void *dbuf, size_t size, mem_type_e type);
void *pack_create_stream(void);
void pack_destroy_stream(void *stream);
void pack_stream_synchronize(void *stream);

#ifdef HAVE_CUDA
int pack_cuda_get_ndevices(void);
void pack_cuda_init_devices(int num_threads);
void pack_cuda_finalize_devices(void);
void pack_cuda_alloc_mem(int device_id, size_t size, mem_type_e type, void **hostbuf,
                         void **devicebuf);
void pack_cuda_free_mem(mem_type_e type, void *hostbuf, void *devicebuf);
void pack_cuda_get_ptr_attr(const void *inbuf, void *outbuf, yaksa_info_t * info, int iter);
void pack_cuda_copy_content(int tid, const void *sbuf, void *dbuf, size_t size, mem_type_e type);
void *pack_cuda_create_stream(void);
void pack_cuda_destroy_stream(void *stream);
void pack_cuda_stream_synchronize(void *stream);
#endif

#ifdef HAVE_ZE
int pack_ze_get_ndevices(void);
void pack_ze_init_devices(int num_threads);
void pack_ze_finalize_devices(void);
void pack_ze_alloc_mem(int device_id, size_t size, mem_type_e type, void **hostbuf,
                       void **devicebuf);
void pack_ze_free_mem(mem_type_e type, void *hostbuf, void *devicebuf);
void pack_ze_get_ptr_attr(const void *inbuf, void *outbuf, yaksa_info_t * info, int iter);
void pack_ze_copy_content(int tid, const void *sbuf, void *dbuf, size_t size, mem_type_e type);
void *pack_ze_create_stream(void);
void pack_ze_destroy_stream(void *stream);
void pack_ze_stream_synchronize(void *stream);
#endif

#ifdef HAVE_HIP
int pack_hip_get_ndevices(void);
void pack_hip_init_devices(int num_threads);
void pack_hip_finalize_devices(void);
void pack_hip_alloc_mem(int device_id, size_t size, mem_type_e type, void **hostbuf,
                        void **devicebuf);
void pack_hip_free_mem(mem_type_e type, void *hostbuf, void *devicebuf);
void pack_hip_get_ptr_attr(const void *inbuf, void *outbuf, yaksa_info_t * info, int iter);
void pack_hip_copy_content(int tid, const void *sbuf, void *dbuf, size_t size, mem_type_e type);
void *pack_hip_create_stream(void);
void pack_hip_destroy_stream(void *stream);
void pack_hip_stream_synchronize(void *stream);
#endif

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* PACK_COMMON_H */
