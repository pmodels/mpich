##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_f08.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.f90" lines
AM_DEFAULT_SOURCE_EXT = .f90

noinst_PROGRAMS = \
	sizeof2 \
	statusconv \
	hello \
	ring

statusconv_SOURCES = statusconvf08.f90 statusconv.c

