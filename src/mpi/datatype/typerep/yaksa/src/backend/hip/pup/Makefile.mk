##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/hip/pup

libyaksa_la_SOURCES += \
	src/backend/hip/pup/yaksuri_hipi_event.c \
	src/backend/hip/pup/yaksuri_hipi_get_ptr_attr.c

include src/backend/hip/pup/Makefile.pup.mk
include src/backend/hip/pup/Makefile.populate_pupfns.mk


#.chip.c:
#	@if $(AM_V_P) ; then \
#		$(top_srcdir)/src/backend/hip/hiplt.sh --verbose $@ \
#			$(HIPCC) $(AM_CPPFLAGS) $(HIP_GENCODE) -c $< ; \
#	else \
#		echo "  HIPCC     $@" ; \
#		$(top_srcdir)/src/backend/hip/hiplt.sh $@ $(HIPCC) $(AM_CPPFLAGS) $(HIP_GENCODE) -c $< ; \
#	fi

#.cu:
#	@if $(AM_V_P) ; then \
#		$(HIPCC) $(AM_CPPFLAGS) $(HIP_GENCODE) -c $< ; \
#	else \
#		echo "  HIPCC     $@" ; \
#		$@ $(HIPCC) $(AM_CPPFLAGS) $(HIP_GENCODE) -c $< ; \
#	fi


