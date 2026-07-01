##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_DATALOOP_ENGINE
include $(top_srcdir)/src/mpi/datatype/typerep/dataloop/Makefile.mk
endif

include $(top_srcdir)/src/mpi/datatype/typerep/src/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/datatype/typerep/src
