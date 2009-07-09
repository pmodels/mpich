# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

SUBDIRS = mpl .

ACLOCAL_AMFLAGS = -I mpl/confdb
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/mpl/include \
	-I$(top_builddir)/mpl/include

noinst_LIBRARIES = libhydra.a libpm.a libui.a
bin_PROGRAMS =
libhydra_a_SOURCES =
libpm_a_SOURCES =
libui_a_SOURCES =

# The below directories contribute to libhydra.a
include utils/Makefile.mk
include rmk/Makefile.mk
include bootstrap/Makefile.mk
include css/Makefile.mk
include demux/Makefile.mk

# Build the PM
include pm/Makefile.mk

# Build the UI
include ui/Makefile.mk

# Use the mpich2-build-install target to include mpiexec in the build bin
# directory (all pm's require these targets)
mpich2-build-install: install
mpich2-build-uninstall: uninstall

# A special alternate installation target when using multiple process managers
install-alt: install-alt-ui install-alt-pm
