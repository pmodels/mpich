# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

# Set external subdirs to NULL. They are built first.
external_subdirs = @mpl_srcdir@
external_ldflags = -L$(top_builddir)/@mpl_srcdir@
external_libs = -lmpl

bin_PROGRAMS =

ACLOCAL_AMFLAGS = -I mpl/confdb
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/@mpl_srcdir@/include \
	-I$(top_builddir)/@mpl_srcdir@/include

noinst_LTLIBRARIES = libhydra.la
libhydra_la_SOURCES =

# The below directories contribute to libhydra
include utils/Makefile.mk
include tools/Makefile.mk
include pm/Makefile.mk
include ui/Makefile.mk

# External subdirs should be built first, as we might depend on them
SUBDIRS = ${external_subdirs} .

# Use the mpich2-build-install target to include mpiexec in the build bin
# directory (all pm's require these targets)
mpich2-build-install: install
mpich2-build-uninstall: uninstall

# A special alternate installation target when using multiple process managers
install-alt: $(bin_PROGRAMS)
	@if [ ! -d $(DESTDIR)${bindir} ] ; then \
	    echo "$(mkdir_p) $(DESTDIR)${bindir} " ;\
	    $(mkdir_p) $(DESTDIR)${bindir} ;\
	fi
	@for foo in $? ; do \
		if test "$$foo" = "mpiexec" ; then \
			$(INSTALL_PROGRAM) $(INSTALL_STRIP_FLAG) mpiexec \
				$(DESTDIR)${bindir}/mpiexec.hydra ; \
		else \
			$(INSTALL_PROGRAM) $(INSTALL_STRIP_FLAG) $$foo $(DESTDIR)${bindir}/ ; \
		fi ; \
	done

htmldoc:
	if test "@DOXYGEN@" != "" ; then @DOXYGEN@ hydra-doxygen.cfg ; fi
