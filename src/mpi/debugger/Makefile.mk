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
noinst_LTLIBRARIES = lib/libdbginitdummy.la
lib_libdbginitdummy_la_SOURCES = src/mpi/debugger/dbginit.c
lib_libdbginitdummy_la_CFLAGS = -g
lib_lib@MPILIBNAME@_la_LIBADD += $(top_builddir)/src/mpi/debugger/libdbginitdummy.la


lib_LTLIBRARIES += lib/libtvmpich2.la
# There is no static debugger interface library
lib_libtvmpich2_la_SOURCES = src/mpi/debugger/dll_mpich2.c
lib_libtvmpich2_la_CFLAGS = -g
lib_libtvmpich2_la_LDFLAGS = -g

# FIXME work is needed here to force these to build late, after mpicc is working
#
# tvtest builds a main program that uses the routines in dll_mpich2 to 
# access the internal structure of an MPICH2 program.  This is only a partial
# test, but it allows a developer to check out the basic functioning of 
# dll_mpich2 (but without loading it).
# tvtest can no longer be built using the mpich libraries - mpich2 no longer
# places all necessary object files into the MPILIBNAME or PMPILIBNAME libraries
#tvtest_SOURCES = tvtest.c dll_mpich2.c dbgstub.c
#qdemo_SOURCES = qdemo.c
#EXTRA_PROGRAMS = tvtest qdemo

endif BUILD_DEBUGGER_DLL

