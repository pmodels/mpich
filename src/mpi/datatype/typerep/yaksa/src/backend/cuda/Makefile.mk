##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

yaksa_cuda_sources =

if BUILD_CUDA_BACKEND
include $(top_srcdir)/src/backend/cuda/include/Makefile.mk
include $(top_srcdir)/src/backend/cuda/hooks/Makefile.mk
include $(top_srcdir)/src/backend/cuda/md/Makefile.mk
include $(top_srcdir)/src/backend/cuda/pup/Makefile.mk
if BUILD_CUDA_DLOPEN
# The backend is built as a separate convenience library, which the parent
# project links into a module that is dlopen-ed at runtime.
noinst_LTLIBRARIES += libyaksa_cuda.la
libyaksa_cuda_la_SOURCES = $(yaksa_cuda_sources)
libyaksa_cuda_la_LIBADD = $(YAKSA_CUDA_LIBS)
libyaksa_la_SOURCES += src/backend/cuda/dl/yaksuri_cuda_dl.c
else
libyaksa_la_SOURCES += $(yaksa_cuda_sources)
endif !BUILD_CUDA_DLOPEN
else
include $(top_srcdir)/src/backend/cuda/stub/Makefile.mk
endif !BUILD_CUDA_BACKEND

.cu.lo:
	@if $(AM_V_P) ; then \
		$(top_srcdir)/src/backend/cuda/cudalt.sh --verbose $@ \
			$(NVCC) $(AM_CPPFLAGS) $(CUDA_GENCODE) -c $< ; \
	else \
		echo "  NVCC     $@" ; \
		$(top_srcdir)/src/backend/cuda/cudalt.sh $@ $(NVCC) $(AM_CPPFLAGS) $(CUDA_GENCODE) -c $< ; \
	fi
