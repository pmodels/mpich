/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_TRMEM_ARCH_H_INCLUDED
#define MPL_TRMEM_ARCH_H_INCLUDED

/* stream memcpy for general usage, using non-temporal store */
void MPL_Memcpy_stream_avx(void *dest, const void *src, size_t n);
void MPL_Memcpy_stream_avx512f(void *dest, const void *src, size_t n);

/* stream memcpy for device memory, using both non-temporal load and store.
 * non-temporal load is designed to work with write-through memory, therefore
 * only suitable for device memory, not regular memory (which is write-combined). */
void MPL_Memcpy_stream_dev_avx(void *dest, const void *src, size_t n);
void MPL_Memcpy_stream_dev_avx512f(void *dest, const void *src, size_t n);

#endif
