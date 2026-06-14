##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/circ_graph

mpi_core_sources += \
    src/mpi/coll/algorithms/circ_graph/circ_graph.c \
    src/mpi/coll/algorithms/circ_graph/cga_init.c \
    src/mpi/coll/algorithms/circ_graph/cga_request_queue.c
