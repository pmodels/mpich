##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/ze/pup

libyaksa_la_SOURCES += \
	src/backend/ze/pup/yaksuri_zei_event.c \
	src/backend/ze/pup/yaksuri_zei_get_ptr_attr.c

include src/backend/ze/pup/Makefile.pup.mk
include src/backend/ze/pup/Makefile.populate_pupfns.mk

ze_native_TARGET = @enable_ze_native@

if BUILD_ZE_NATIVE

.cl.c:
	@echo "  OCLOC (native)  $<" ; \
	ocloc compile -file $< -device $(ze_native_TARGET) -out_dir `dirname $@` -output_no_suffix -q -options "-I $(top_srcdir)/src/backend/ze/include -cl-std=CL2.0" @extra_ocloc_options@ && \
	(test -f $(@:.c=) && mv $(@:.c=) $(@:.c=.bin) || true) && /bin/rm -f $(@:.c=.gen) && \
	$(top_srcdir)/src/backend/ze/pup/inline.py $(@:.c=.bin) $@ $(top_srcdir) 1

else

if BUILD_ZE_NATIVE_MULTIPLE

.cl.c:
	@echo "  OCLOC (multiple native)  $<" ; \
	ocloc compile -file $< -device $(ze_native_TARGET) -q -options "-I $(top_srcdir)/src/backend/ze/include -cl-std=CL2.0" @extra_ocloc_options@ && \
	mv `basename $(@:.c=.ar)` `dirname $@` && \
	$(top_srcdir)/src/backend/ze/pup/inline.py $(@:.c=.ar) $@ $(top_srcdir) 2

else

.cl.c:
	@echo " OCLOC (spirv)  $<"; \
	ocloc compile -file $< -device skl -spv_only -out_dir `dirname $@` -output_no_suffix -q -options "-I $(top_srcdir)/src/backend/ze/include -cl-std=CL2.0" && \
	/bin/rm -f $(@:.c=.gen) &&  \
	$(top_srcdir)/src/backend/ze/pup/inline.py $(@:.c=.spv) $@ $(top_srcdir) 0

endif

endif
