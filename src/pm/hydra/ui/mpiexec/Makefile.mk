# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

bin_PROGRAMS += mpiexec

mpiexec_SOURCES = $(top_srcdir)/ui/mpiexec/callback.c \
	$(top_srcdir)/ui/mpiexec/mpiexec.c \
	$(top_srcdir)/ui/mpiexec/utils.c
mpiexec_LDADD = libui.a libpm.a libhydra.a
mpiexec_CFLAGS = -I$(top_srcdir)/ui/utils

install-alt-ui: mpiexec
	@if [ ! -d $(DESTDIR)${bindir} ] ; then \
	    echo "$(mkdir_p) $(DESTDIR)${bindir} " ;\
	    $(mkdir_p) $(DESTDIR)${bindir} ;\
	fi
	$(INSTALL_PROGRAM) $(INSTALL_STRIP_FLAG) mpiexec $(DESTDIR)${bindir}/mpiexec.hydra
