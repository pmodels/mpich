

#ifndef CH4_CUDA_HELPER_H_INCLUDED
#define CH4_CUDA_HELPER_H_INCLUDED

#include <stdio.h>
#include <cuda_runtime.h>
#include <cuda.h>

static inline void cuda_func(cudaError_t status)
{
    if (status != cudaSuccess) {
        fprintf(stderr, "CUDA Error: %s\n", cudaGetErrorString(status));
    }
}

int is_mem_type_device(const void *address);

#endif /* CH4_CUDA_HELPER_H_INCLUDED */
