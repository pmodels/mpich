##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/circ_graph

mpi_core_sources += \
    src/mpi/coll/algorithms/circ_graph/circ_graph.c \
    src/mpi/coll/algorithms/circ_graph/cga_request_queue.c
