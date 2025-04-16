##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_common.c

if MPL_HAVE_CUDA
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_cuda.c
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_cuda_kernels.cu

.cu.lo:
	@if $(AM_V_P) ; then \
		$(top_srcdir)/confdb/cudalt.sh --verbose $@ \
			$(NVCC) $(AM_CPPFLAGS) -c $< ; \
	else \
		echo "  NVCC     $@" ; \
		$(top_srcdir)/confdb/cudalt.sh $@ $(NVCC) $(AM_CPPFLAGS) -c $< ; \
	fi
else
if MPL_HAVE_ZE
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_ze.c \
	src/gpu/mpl_gpu_ze_avx.c \
	src/gpu/mpl_gpu_ze_avx512f.c
if MPL_BUILD_AVX
src/gpu/mpl_gpu_ze_avx.lo: CFLAGS += -mavx2
endif
if MPL_BUILD_AVX512F
src/gpu/mpl_gpu_ze_avx512f.lo: CFLAGS += -mavx512f
endif
else
if MPL_HAVE_HIP
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_hip.c
else
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_fallback.c
endif
endif
endif
