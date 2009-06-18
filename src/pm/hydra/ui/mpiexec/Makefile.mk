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
mpiexec_LDFLAGS = -lpthread
mpiexec_CFLAGS = -I$(top_srcdir)/ui/utils

# Use the mpich2-build-install target to include mpiexec in the build bin
# directory (all pm's require these targets)
mpich2-build-install: install
mpich2-build-uninstall: uninstall

# A special alternate installation target when using multiple process managers
install-alt: mpiexec
	@if [ ! -d $(DESTDIR)${bindir} ] ; then \
	    echo "mkdir -p $(DESTDIR)${bindir} " ;\
	    mkdir -p $(DESTDIR)${bindir} ;\
	fi
	$(INSTALL_PROGRAM) $(INSTALL_STRIP_FLAG) mpiexec $(DESTDIR)${bindir}/mpiexec.hydra
