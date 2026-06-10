/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_CUDAI_OPS_CUH_INCLUDED
#define YAKSURI_CUDAI_OPS_CUH_INCLUDED

template<typename T> struct YaksuriOpReplace {
    __device__ static void apply(const T& in, T& out) { out = in; }
};
template<typename T> struct YaksuriOpSum {
    __device__ static void apply(const T& in, T& out) { out += in; }
};
template<typename T> struct YaksuriOpProd {
    __device__ static void apply(const T& in, T& out) { out *= in; }
};
template<typename T> struct YaksuriOpLand {
    __device__ static void apply(const T& in, T& out) { out = out && in; }
};
template<typename T> struct YaksuriOpBand {
    __device__ static void apply(const T& in, T& out) { out &= in; }
};
template<typename T> struct YaksuriOpLor {
    __device__ static void apply(const T& in, T& out) { out = out || in; }
};
template<typename T> struct YaksuriOpBor {
    __device__ static void apply(const T& in, T& out) { out |= in; }
};
template<typename T> struct YaksuriOpLxor {
    __device__ static void apply(const T& in, T& out) { out = !out != !in; }
};
template<typename T> struct YaksuriOpBxor {
    __device__ static void apply(const T& in, T& out) { out ^= in; }
};

/* MAX/MIN: CUDA overloaded max()/min() map to hardware instructions for all types */
template<typename T> struct YaksuriOpMax {
    __device__ static void apply(const T& in, T& out) { out = max(in, out); }
};
template<typename T> struct YaksuriOpMin {
    __device__ static void apply(const T& in, T& out) { out = min(in, out); }
};

#endif  /* YAKSURI_CUDAI_OPS_CUH_INCLUDED */
