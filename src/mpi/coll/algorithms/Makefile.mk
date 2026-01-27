##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/src/mpi/coll/algorithms/treealgo/Makefile.mk
include $(top_srcdir)/src/mpi/coll/algorithms/recexchalgo/Makefile.mk
include $(top_srcdir)/src/mpi/coll/algorithms/stubalgo/Makefile.mk
include $(top_srcdir)/src/mpi/coll/algorithms/circ_graph/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/common
