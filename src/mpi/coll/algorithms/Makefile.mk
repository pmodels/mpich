##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/src/mpi/coll/algorithms/treealgo/Makefile.mk
include $(top_srcdir)/src/mpi/coll/algorithms/recexchalgo/Makefile.mk
include $(top_srcdir)/src/mpi/coll/algorithms/stubalgo/Makefile.mk
include $(top_srcdir)/src/mpi/coll/algorithms/circ_graph/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/common
