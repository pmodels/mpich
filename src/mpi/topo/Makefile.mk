## -*- Mode: Makefile; -*-
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                          \
    src/mpi/topo/cart/cart_coords.c          \
    src/mpi/topo/cart/cart_create.c          \
    src/mpi/topo/cart/cart_get.c             \
    src/mpi/topo/cart/cart_map.c             \
    src/mpi/topo/cart/cart_rank.c            \
    src/mpi/topo/cart/cart_shift.c           \
    src/mpi/topo/cart/cart_sub.c             \
    src/mpi/topo/cart/cartdim_get.c          \
    src/mpi/topo/dims_create.c          \
    src/mpi/topo/graph/graph_get.c            \
    src/mpi/topo/graph/graph_map.c            \
    src/mpi/topo/graph/graph_nbr.c            \
    src/mpi/topo/graph/graphcreate.c          \
    src/mpi/topo/graph/graphdimsget.c         \
    src/mpi/topo/graph/graphnbrcnt.c          \
    src/mpi/topo/topo_test.c            \
    src/mpi/topo/dist_gr/dist_gr_create_adj.c   \
    src/mpi/topo/dist_gr/dist_gr_create.c       \
    src/mpi/topo/dist_gr/dist_gr_neighb_count.c \
    src/mpi/topo/dist_gr/dist_gr_neighb.c       \
    src/mpi/topo/inhb_allgather/inhb_allgather.c       \
    src/mpi/topo/inhb_allgatherv/inhb_allgatherv.c      \
    src/mpi/topo/inhb_alltoall/inhb_alltoall.c        \
    src/mpi/topo/inhb_alltoallv/inhb_alltoallv.c       \
    src/mpi/topo/inhb_alltoallw/inhb_alltoallw.c       \
    src/mpi/topo/nhb_allgather/nhb_allgather.c        \
    src/mpi/topo/nhb_allgatherv/nhb_allgatherv.c       \
    src/mpi/topo/nhb_alltoall/nhb_alltoall.c         \
    src/mpi/topo/nhb_alltoallv/nhb_alltoallv.c        \
    src/mpi/topo/nhb_alltoallw/nhb_alltoallw.c

mpi_core_sources +=       \
    src/mpi/topo/topoutil.c
