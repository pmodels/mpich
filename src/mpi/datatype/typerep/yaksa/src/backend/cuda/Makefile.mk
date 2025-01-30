##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

EXTRA_DIST += $(top_srcdir)/src/backend/cuda/cudalt.sh

if BUILD_CUDA_BACKEND
include $(top_srcdir)/src/backend/cuda/include/Makefile.mk
include $(top_srcdir)/src/backend/cuda/hooks/Makefile.mk
include $(top_srcdir)/src/backend/cuda/md/Makefile.mk
include $(top_srcdir)/src/backend/cuda/pup/Makefile.mk
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
