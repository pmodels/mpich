# This Makefile.am generated automatically by f77tof90
# from test/mpi/f77/attr/Makefile.am.  DO NOT EDIT
# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f08.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.f90" lines
AM_DEFAULT_SOURCE_EXT = .f90

noinst_PROGRAMS =                   \
    attrmpi1f08                       \
    baseattrf08 baseattr2f08            \
    commattrf08 commattr2f08 commattr3f08 \
    typeattrf08 typeattr2f08 typeattr3f08

## attraints.h will be distributed because it's listed in AC_CONFIG_FILES/AC_OUTPUT

# ensure that "{comm,type}attr*f.o" will be rebuilt when attraints.h is updated


## vim: set ft=automake :

noinst_PROGRAMS += fandcattrf08 baseattr3f08 attrlangf08
fandcattrf08_SOURCES = fandcattrf08.f90 fandcattrc.c
baseattr3f08_SOURCES = baseattr3f08.f90
attrlangf08_SOURCES = attrlangf08.f90 attrlangc.c

# Remove any created module files, if we know the extension of those files
clean-local:
	if [ "@FCMODEXT@" != "" ] ; then rm -f *.@FCMODEXT@ ; fi
