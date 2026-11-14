##
## Copyright (C) by Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/Makefile_cxx.mtest


noinst_PROGRAMS = typecntsx packsizex

typecntsx_SOURCES = typecntsx.cxx
typenamex_SOURCES = typenamex.cxx
typemiscx_SOURCES = typemiscx.cxx
packsizex_SOURCES = packsizex.cxx

if HAS_F77
noinst_PROGRAMS += typenamex typemiscx
typenamex_LDFLAGS = -mpi-fortran
typemiscx_LDFLAGS = -mpi-fortran
endif
