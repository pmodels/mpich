#define ALLTOALL_ARGS(Dtype, b) \
    __global Dtype *in_buf##b, __global Dtype *out_buf##b,

#define ALLTOALL_ARGS1(Dtype) \
    ALLTOALL_ARGS(Dtype, 0)
#define ALLTOALL_ARGS2(Dtype) \
    ALLTOALL_ARGS1(Dtype) ALLTOALL_ARGS(Dtype, 1)
#define ALLTOALL_ARGS3(Dtype) \
    ALLTOALL_ARGS2(Dtype) ALLTOALL_ARGS(Dtype, 2)
#define ALLTOALL_ARGS4(Dtype) \
    ALLTOALL_ARGS3(Dtype) ALLTOALL_ARGS(Dtype, 3)
#define ALLTOALL_ARGS5(Dtype) \
    ALLTOALL_ARGS4(Dtype) ALLTOALL_ARGS(Dtype, 4)
#define ALLTOALL_ARGS6(Dtype) \
    ALLTOALL_ARGS5(Dtype) ALLTOALL_ARGS(Dtype, 5)
#define ALLTOALL_ARGS7(Dtype) \
    ALLTOALL_ARGS6(Dtype) ALLTOALL_ARGS(Dtype, 6)
#define ALLTOALL_ARGS8(Dtype) \
    ALLTOALL_ARGS7(Dtype) ALLTOALL_ARGS(Dtype, 7)
#define ALLTOALL_ARGS9(Dtype) \
    ALLTOALL_ARGS8(Dtype) ALLTOALL_ARGS(Dtype, 8)
#define ALLTOALL_ARGS10(Dtype) \
    ALLTOALL_ARGS9(Dtype) ALLTOALL_ARGS(Dtype, 9)
#define ALLTOALL_ARGS11(Dtype) \
    ALLTOALL_ARGS10(Dtype) ALLTOALL_ARGS(Dtype, 10)
#define ALLTOALL_ARGS12(Dtype) \
    ALLTOALL_ARGS11(Dtype) ALLTOALL_ARGS(Dtype, 11)

#define ALLTOALL_COPY(b) \
    for (size_t idx = thread_id; idx < count; idx += work_group_size) { \
        out_buf##b[idx] = in_buf##b[idx]; \
    }

#define ALLTOALL_COPY1 \
    ALLTOALL_COPY(0)
#define ALLTOALL_COPY2 \
    ALLTOALL_COPY1 ALLTOALL_COPY(1)
#define ALLTOALL_COPY3 \
    ALLTOALL_COPY2 ALLTOALL_COPY(2)
#define ALLTOALL_COPY4 \
    ALLTOALL_COPY3 ALLTOALL_COPY(3)
#define ALLTOALL_COPY5 \
    ALLTOALL_COPY4 ALLTOALL_COPY(4)
#define ALLTOALL_COPY6 \
    ALLTOALL_COPY5 ALLTOALL_COPY(5)
#define ALLTOALL_COPY7 \
    ALLTOALL_COPY6 ALLTOALL_COPY(6)
#define ALLTOALL_COPY8 \
    ALLTOALL_COPY7 ALLTOALL_COPY(7)
#define ALLTOALL_COPY9 \
    ALLTOALL_COPY8 ALLTOALL_COPY(8)
#define ALLTOALL_COPY10 \
    ALLTOALL_COPY9 ALLTOALL_COPY(9)
#define ALLTOALL_COPY11 \
    ALLTOALL_COPY10 ALLTOALL_COPY(10)
#define ALLTOALL_COPY12 \
    ALLTOALL_COPY11 ALLTOALL_COPY(11)

#define DEFINE_ALLTOALL_KERNEL(Dtype, N) \
    __kernel void alltoall_kernel_##N##_##Dtype( \
        ALLTOALL_ARGS##N(Dtype) \
        unsigned long count) { \
        size_t work_group_size = get_global_size(0); \
        size_t thread_id = get_global_id(0); \
        ALLTOALL_COPY##N \
    }

// Define the kernels with all kinds of datatypes
#define DEFINE_KERNELS_PEERS(KernelName, N) \
    DEFINE_##KernelName##_KERNEL(char, N) \
    DEFINE_##KernelName##_KERNEL(uchar, N) \
\
    DEFINE_##KernelName##_KERNEL(short, N) \
    DEFINE_##KernelName##_KERNEL(ushort, N) \
\
    DEFINE_##KernelName##_KERNEL(int, N) \
    DEFINE_##KernelName##_KERNEL(uint, N) \
\
    DEFINE_##KernelName##_KERNEL(long, N) \
    DEFINE_##KernelName##_KERNEL(ulong, N) \
\
    DEFINE_##KernelName##_KERNEL(float, N) \
    DEFINE_##KernelName##_KERNEL(double, N)


// Define the kernels for all peer_counts
#define DEFINE_KERNELS(KernelName) \
    DEFINE_KERNELS_PEERS(KernelName, 1) \
    DEFINE_KERNELS_PEERS(KernelName, 2) \
    DEFINE_KERNELS_PEERS(KernelName, 3) \
    DEFINE_KERNELS_PEERS(KernelName, 4) \
    DEFINE_KERNELS_PEERS(KernelName, 5) \
    DEFINE_KERNELS_PEERS(KernelName, 6) \
    DEFINE_KERNELS_PEERS(KernelName, 7) \
    DEFINE_KERNELS_PEERS(KernelName, 8) \
    DEFINE_KERNELS_PEERS(KernelName, 9) \
    DEFINE_KERNELS_PEERS(KernelName, 10) \
    DEFINE_KERNELS_PEERS(KernelName, 11) \
    DEFINE_KERNELS_PEERS(KernelName, 12)

DEFINE_KERNELS(ALLTOALL)
