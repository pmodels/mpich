##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_common.c

if MPL_HAVE_CUDA
if MPL_CUDA_DLOPEN
# The code that calls CUDA is built as a separate convenience library, which
# the parent project links into a module that is dlopen-ed at runtime.
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_cuda_dl.c
noinst_LTLIBRARIES += lib@MPLLIBNAME@_cuda.la
lib@MPLLIBNAME@_cuda_la_SOURCES = src/gpu/mpl_gpu_cuda.c src/gpu/mpl_gpu_cuda_kernels.cu
lib@MPLLIBNAME@_cuda_la_LIBADD = $(MPL_CUDA_LIBS)
else
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_cuda.c
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_cuda_kernels.cu
endif

.cu.lo:
	@if $(AM_V_P) ; then \
		$(top_srcdir)/src/gpu/cudalt.sh --verbose $@ \
			$(NVCC) $(AM_CPPFLAGS) -c $< ; \
	else \
		echo "  NVCC     $@" ; \
		$(top_srcdir)/src/gpu/cudalt.sh $@ $(NVCC) $(AM_CPPFLAGS) -c $< ; \
	fi
else
if MPL_HAVE_ZE
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_ze.c
else
if MPL_HAVE_HIP
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_hip.c
else
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_fallback.c
endif
endif
endif
