## -*- Mode: Makefile; -*-
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                          \
    src/mpi/topo/cart_coords.c          \
    src/mpi/topo/cart_create.c          \
    src/mpi/topo/cart_get.c             \
    src/mpi/topo/cart_map.c             \
    src/mpi/topo/cart_rank.c            \
    src/mpi/topo/cart_shift.c           \
    src/mpi/topo/cart_sub.c             \
    src/mpi/topo/dims_create.c          \
    src/mpi/topo/graph_get.c            \
    src/mpi/topo/graph_map.c            \
    src/mpi/topo/graph_nbr.c            \
    src/mpi/topo/graphcreate.c          \
    src/mpi/topo/graphdimsget.c         \
    src/mpi/topo/graphnbrcnt.c          \
    src/mpi/topo/cartdim_get.c          \
    src/mpi/topo/topo_test.c            \
    src/mpi/topo/dist_gr_create_adj.c   \
    src/mpi/topo/dist_gr_create.c       \
    src/mpi/topo/dist_gr_neighb_count.c \
    src/mpi/topo/dist_gr_neighb.c       \
    src/mpi/topo/inhb_allgather.c       \
    src/mpi/topo/inhb_allgatherv.c      \
    src/mpi/topo/inhb_alltoall.c        \
    src/mpi/topo/inhb_alltoallv.c       \
    src/mpi/topo/inhb_alltoallw.c       \
    src/mpi/topo/nhb_allgather.c        \
    src/mpi/topo/nhb_allgatherv.c       \
    src/mpi/topo/nhb_alltoall.c         \
    src/mpi/topo/nhb_alltoallv.c        \
    src/mpi/topo/nhb_alltoallw.c

mpi_core_sources +=       \
    src/mpi/topo/topoutil.c

noinst_HEADERS += src/mpi/topo/topo.h
