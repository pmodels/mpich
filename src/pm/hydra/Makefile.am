# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

SUBDIRS = mpl .

ACLOCAL_AMFLAGS = -I mpl/confdb
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/mpl/include \
	-I$(top_builddir)/mpl/include \
	-I$(top_srcdir)/bootstrap/include -I$(top_builddir)/bootstrap/include \
	-I$(top_srcdir)/bootstrap/utils \
	-I$(top_srcdir)/rmk/include -I$(top_builddir)/rmk/include -I$(top_srcdir)/rmk/utils \
	-I$(top_srcdir)/css/include -I$(top_builddir)/css/include -I$(top_srcdir)/css/utils \
	-I$(top_srcdir)/demux -I$(top_srcdir)/pm/include -I$(top_srcdir)/utils/plpa

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
