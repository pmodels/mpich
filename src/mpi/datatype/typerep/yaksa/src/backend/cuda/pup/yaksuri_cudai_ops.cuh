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

/* Integer MAX/MIN: branchless bitwise trick */
template<typename T> struct YaksuriOpMax {
    __device__ static void apply(const T& in, T& out) {
        out = in ^ ((in ^ out) & -(in < out));
    }
};
template<typename T> struct YaksuriOpMin {
    __device__ static void apply(const T& in, T& out) {
        out = out ^ ((in ^ out) & -(in < out));
    }
};

/* Float/double MAX/MIN */
template<> struct YaksuriOpMax<float> {
    __device__ static void apply(const float& in, float& out) {
        if (in > out) out = in;
    }
};
template<> struct YaksuriOpMax<double> {
    __device__ static void apply(const double& in, double& out) {
        if (in > out) out = in;
    }
};
template<> struct YaksuriOpMin<float> {
    __device__ static void apply(const float& in, float& out) {
        if (in < out) out = in;
    }
};
template<> struct YaksuriOpMin<double> {
    __device__ static void apply(const double& in, double& out) {
        if (in < out) out = in;
    }
};

#endif  /* YAKSURI_CUDAI_OPS_CUH_INCLUDED */
