##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

lib@MPLLIBNAME@_la_SOURCES += src/mem/mpl_trmem.c \
	src/mem/mpl_trmem_avx.c \
	src/mem/mpl_trmem_avx512f.c

if MPL_BUILD_AVX
src/mem/mpl_trmem_avx.lo: CFLAGS += -mavx2
endif

if MPL_BUILD_AVX512F
src/mem/mpl_trmem_avx512f.lo: CFLAGS += -mavx512f
endif
