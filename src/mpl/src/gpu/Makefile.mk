##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

libmpl_la_SOURCES += src/gpu/mpl_gpu_common.c

if MPL_HAVE_CUDA
libmpl_la_SOURCES += src/gpu/mpl_gpu_cuda.c
libmpl_la_SOURCES += src/gpu/mpl_gpu_cuda_kernels.cu

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
libmpl_la_SOURCES += src/gpu/mpl_gpu_ze.c
else
if MPL_HAVE_HIP
libmpl_la_SOURCES += src/gpu/mpl_gpu_hip.c
else
libmpl_la_SOURCES += src/gpu/mpl_gpu_fallback.c
endif
endif
endif
