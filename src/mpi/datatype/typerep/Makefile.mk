##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_DATALOOP_ENGINE
include $(top_srcdir)/src/mpi/datatype/typerep/dataloop/Makefile.mk
endif

include $(top_srcdir)/src/mpi/datatype/typerep/src/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/datatype/typerep/src
