##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

ACLOCAL_AMFLAGS = -I confdb
AM_CPPFLAGS = @VISIBILITY_CFLAGS@ -I$(top_srcdir)
AM_FFLAGS =
AM_FCFLAGS =

mpi_fc_modules =
mpifort_convenience_libs =

lib_LTLIBRARIES =
noinst_LTLIBRARIES =
noinst_HEADERS =
nodist_include_HEADERS =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

include $(srcdir)/mpif_h/Makefile.mk
include $(srcdir)/use_mpi/Makefile.mk
include $(srcdir)/use_mpi_f08/Makefile.mk

lib_LTLIBRARIES += lib/lib@MPIFCLIBNAME@.la
lib_lib@MPIFCLIBNAME@_la_SOURCES =
modinc_HEADERS = $(mpi_fc_modules)
lib_lib@MPIFCLIBNAME@_la_LDFLAGS = $(ABIVERSIONFLAGS) @FCLIB_LDFLAGS@
lib_lib@MPIFCLIBNAME@_la_LIBADD = @mpi_libs@ $(mpifort_convenience_libs)

if BUILD_F77_BINDING
if BUILD_FC_BINDING
lib_lib@MPIFCLIBNAME@_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=FC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link $(FCLD) \
	$(AM_FCFLAGS) $(FCFLAGS) $(lib_lib@MPIFCLIBNAME@_la_LDFLAGS) \
	$(LDFLAGS) -o $@
else !BUILD_FC_BINDING
lib_lib@MPIFCLIBNAME@_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=F77 \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link $(F77LD) \
	$(AM_FFLAGS) $(FFLAGS) $(lib_lib@MPIFCLIBNAME@_la_LDFLAGS) \
	$(LDFLAGS) -o $@
endif !BUILD_FC_BINDING
endif BUILD_F77_BINDING
