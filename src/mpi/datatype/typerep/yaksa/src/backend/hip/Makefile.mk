##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

EXTRA_DIST += $(top_srcdir)/src/backend/hip/hiplt.sh

if BUILD_HIP_BACKEND
include $(top_srcdir)/src/backend/hip/include/Makefile.mk
include $(top_srcdir)/src/backend/hip/hooks/Makefile.mk
include $(top_srcdir)/src/backend/hip/md/Makefile.mk
include $(top_srcdir)/src/backend/hip/pup/Makefile.mk
else
include $(top_srcdir)/src/backend/hip/stub/Makefile.mk
endif !BUILD_HIP_BACKEND

.hip.lo:
	@if $(AM_V_P) ; then \
		$(top_srcdir)/src/backend/hip/hiplt.sh --verbose $@ \
			$(HIPCC) $(AM_CPPFLAGS) -g $(HIP_GENCODE) -c $< ; \
	else \
		echo "  HIPCC     $@" ; \
		$(top_srcdir)/src/backend/hip/hiplt.sh $@ $(HIPCC) $(AM_CPPFLAGS) -g $(HIP_GENCODE) -c $< ; \
	fi

