##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/src/mpi/datatype/typerep/dataloop/Makefile.mk
include $(top_srcdir)/src/mpi/datatype/typerep/src/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/datatype/typerep/src
