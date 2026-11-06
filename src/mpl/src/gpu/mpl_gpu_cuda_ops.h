/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_GPU_CUDA_OPS_H_INCLUDED
#define MPL_GPU_CUDA_OPS_H_INCLUDED

/* With --enable-dlopen, the code that calls CUDA (mpl_gpu_cuda.c and
 * mpl_gpu_cuda_kernels.cu) is not part of libmpl. It is built into a module
 * that is dlopen-ed at runtime. The module exports a single symbol,
 * MPL_gpu_cuda_ops, a table of the MPL_gpu_ functions it implements.
 * The MPL_gpu_ functions in libmpl (mpl_gpu_cuda_dl.c) forward to the table.
 *
 * MPL_GPU_CUDA_OPS lists the functions in the table as
 *     X(return type, name without the MPL_gpu_ prefix, parameters)
 */
#define MPL_GPU_CUDA_OPS(X) \
    X(int, init, (int debug_summary)) \
    X(int, finalize, (void)) \
    X(int, get_dev_count, (int *dev_cnt, int *dev_id, int *subdevice_id)) \
    X(int, query_pointer_attr, (const void *ptr, MPL_pointer_attr_t * attr)) \
    X(int, ipc_handle_create, (const void *ptr, MPL_gpu_device_attr * ptr_attr, \
                               MPL_gpu_ipc_mem_handle_t * mpl_ipc_handle)) \
    X(int, ipc_handle_map, (MPL_gpu_ipc_mem_handle_t * mpl_ipc_handle, int dev_id, \
                            MPL_gpu_map_t * map_out, bool is_mmap, size_t len)) \
    X(int, ipc_handle_unmap, (MPL_gpu_map_t * map_ptr)) \
    X(MPL_gpu_buffer_id_t, get_buffer_id, (void *ptr)) \
    X(bool, ipc_handle_is_valid, (MPL_gpu_ipc_mem_handle_t * handle, void *ptr)) \
    X(int, malloc_host, (void **ptr, size_t size)) \
    X(int, free_host, (void *ptr)) \
    X(int, register_host, (const void *ptr, size_t size)) \
    X(int, unregister_host, (const void *ptr)) \
    X(int, malloc, (void **ptr, size_t size, MPL_gpu_device_handle_t h_device)) \
    X(int, free, (void *ptr)) \
    X(int, global_to_local_dev_id, (int global_dev_id)) \
    X(int, local_to_global_dev_id, (int local_dev_id)) \
    X(int, get_current_dev_bdf, (int *domain, int *bus, int *dev, int *func)) \
    X(int, get_buffer_bounds, (const void *ptr, void **pbase, uintptr_t * len)) \
    X(int, launch_hostfn, (MPL_gpu_stream_t stream, MPL_gpu_hostfn fn, void *data)) \
    X(bool, stream_is_valid, (MPL_gpu_stream_t stream)) \
    X(void, enqueue_trigger, (MPL_gpu_event_t * var, MPL_gpu_stream_t stream)) \
    X(void, enqueue_wait, (MPL_gpu_event_t * var, MPL_gpu_stream_t stream))

typedef struct {
    /* the module does not link with libmpl, libmpl passes in the address of MPL_gpu_info */
    void (*set_gpu_info) (MPL_gpu_info_t * gpu_info);
#define MPL_GPU_CUDA_OPS_FIELD(ret, name, params) ret (*name) params;
     MPL_GPU_CUDA_OPS(MPL_GPU_CUDA_OPS_FIELD)
#undef MPL_GPU_CUDA_OPS_FIELD
} MPL_gpu_cuda_ops_t;

#define MPL_GPU_CUDA_OPS_SYMBOL "MPL_gpu_cuda_ops"

#endif /* ifndef MPL_GPU_CUDA_OPS_H_INCLUDED */
