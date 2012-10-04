## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# all of these routines are internal, nothing gets added to mpi_sources

if BUILD_DEBUGGER_DLL

noinst_HEADERS += src/mpi/debugger/mpich2_dll_defs.h

# workaround the fact that autoconf is missing per-object flags by using a dummy
# convenience library that isn't installed
noinst_LTLIBRARIES += src/mpi/debugger/libdbginitdummy.la
src_mpi_debugger_libdbginitdummy_la_SOURCES = src/mpi/debugger/dbginit.c
src_mpi_debugger_libdbginitdummy_la_CFLAGS = -g
lib_lib@MPILIBNAME@_la_LIBADD += $(top_builddir)/src/mpi/debugger/libdbginitdummy.la

lib_LTLIBRARIES += lib/libtvmpich2.la
# There is no static debugger interface library
lib_libtvmpich2_la_SOURCES = src/mpi/debugger/dll_mpich2.c
lib_libtvmpich2_la_CFLAGS = -g
lib_libtvmpich2_la_LDFLAGS = -g $(ABIVERSIONFLAGS)

# tvtest builds a main program that uses the routines in dll_mpich2 to 
# access the internal structure of an MPICH2 program.  This is only a partial
# test, but it allows a developer to check out the basic functioning of 
# dll_mpich2 (but without loading it).
#
# NB: these tests can only be built *after* a "make install" step.  They must be
# built explicitly
tvtest_objs = src/mpi/debugger/tvtest.o src/mpi/debugger/dbgstub.o
src/mpi/debugger/tvtest: $(tvtest_objs)
	$(bindir)/mpicc -o $@ $(tvtest_objs) -ltvmpich2

src/mpi/debugger/tvtest.o: src/mpi/debugger/tvtest.c
	$(bindir)/mpicc -c -o $@ $?

# no rule for dbgstub.o, it needs to be built by suffix rules in the usual
# fashion because it needs direct access to MPICH2 internal headers

src/mpi/debugger/qdemo: src/mpi/debugger/qdemo.c
	$(bindir)/mpicc -o $@ $?

endif BUILD_DEBUGGER_DLL

