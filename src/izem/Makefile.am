# -*- Mode: Makefile; -*-
#
# See COPYRIGHT in top-level directory.
#

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src
DIST_SUBDIRS = $(SUBDIRS) test

EXTRA_DIST = autogen.sh

.PHONY: build-all clean-all doxygen

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = maint/izem.pc

build-all:
	$(MAKE)
	$(MAKE) -C test

clean-all:
	$(MAKE) clean
	$(MAKE) -C test clean

doxygen:
	mkdir -p doc
	doxygen Doxyfile
