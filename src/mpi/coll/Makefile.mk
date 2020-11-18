##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/src/mpi/coll/allgather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/allgatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/allreduce/Makefile.mk
include $(top_srcdir)/src/mpi/coll/alltoall/Makefile.mk
include $(top_srcdir)/src/mpi/coll/alltoallv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/alltoallw/Makefile.mk
include $(top_srcdir)/src/mpi/coll/barrier/Makefile.mk
include $(top_srcdir)/src/mpi/coll/bcast/Makefile.mk
include $(top_srcdir)/src/mpi/coll/exscan/Makefile.mk
include $(top_srcdir)/src/mpi/coll/gather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/gatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/reduce_scatter/Makefile.mk
include $(top_srcdir)/src/mpi/coll/reduce_scatter_block/Makefile.mk
include $(top_srcdir)/src/mpi/coll/reduce/Makefile.mk
include $(top_srcdir)/src/mpi/coll/scan/Makefile.mk
include $(top_srcdir)/src/mpi/coll/scatter/Makefile.mk
include $(top_srcdir)/src/mpi/coll/scatterv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/neighbor_allgather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/neighbor_allgatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/neighbor_alltoall/Makefile.mk
include $(top_srcdir)/src/mpi/coll/neighbor_alltoallv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/neighbor_alltoallw/Makefile.mk

include $(top_srcdir)/src/mpi/coll/iallgather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iallgatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iallreduce/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ialltoall/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ialltoallv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ialltoallw/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ibarrier/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ibcast/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iexscan/Makefile.mk
include $(top_srcdir)/src/mpi/coll/igather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/igatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ireduce_scatter/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ireduce_scatter_block/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ireduce/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iscan/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iscatter/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iscatterv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ineighbor_allgather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ineighbor_allgatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ineighbor_alltoall/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ineighbor_alltoallv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ineighbor_alltoallw/Makefile.mk

include $(top_srcdir)/src/mpi/coll/op/Makefile.mk
include $(top_srcdir)/src/mpi/coll/reduce_local/Makefile.mk
include $(top_srcdir)/src/mpi/coll/allreduce_group/Makefile.mk
include $(top_srcdir)/src/mpi/coll/src/Makefile.mk

# build collectives transport
include $(top_srcdir)/src/mpi/coll/transports/Makefile.mk

# build collectives algorithms
include $(top_srcdir)/src/mpi/coll/algorithms/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/include/

mpi_sources += \
    src/mpi/coll/allgather.c \
    src/mpi/coll/allgatherv.c \
    src/mpi/coll/allreduce.c \
    src/mpi/coll/alltoall.c \
    src/mpi/coll/alltoallv.c \
    src/mpi/coll/alltoallw.c \
    src/mpi/coll/barrier.c \
    src/mpi/coll/bcast.c \
    src/mpi/coll/exscan.c \
    src/mpi/coll/gather.c \
    src/mpi/coll/gatherv.c \
    src/mpi/coll/iallgather.c \
    src/mpi/coll/iallgatherv.c \
    src/mpi/coll/iallreduce.c \
    src/mpi/coll/ialltoall.c \
    src/mpi/coll/ialltoallv.c \
    src/mpi/coll/ialltoallw.c \
    src/mpi/coll/ibarrier.c \
    src/mpi/coll/ibcast.c \
    src/mpi/coll/iexscan.c \
    src/mpi/coll/igather.c \
    src/mpi/coll/igatherv.c \
    src/mpi/coll/ineighbor_allgather.c \
    src/mpi/coll/ineighbor_allgatherv.c \
    src/mpi/coll/ineighbor_alltoall.c \
    src/mpi/coll/ineighbor_alltoallv.c \
    src/mpi/coll/ineighbor_alltoallw.c \
    src/mpi/coll/ireduce.c \
    src/mpi/coll/ireduce_scatter.c \
    src/mpi/coll/ireduce_scatter_block.c \
    src/mpi/coll/iscan.c \
    src/mpi/coll/iscatter.c \
    src/mpi/coll/iscatterv.c \
    src/mpi/coll/neighbor_allgather.c \
    src/mpi/coll/neighbor_allgatherv.c \
    src/mpi/coll/neighbor_alltoall.c \
    src/mpi/coll/neighbor_alltoallv.c \
    src/mpi/coll/neighbor_alltoallw.c \
    src/mpi/coll/reduce.c \
    src/mpi/coll/reduce_local.c \
    src/mpi/coll/reduce_scatter.c \
    src/mpi/coll/reduce_scatter_block.c \
    src/mpi/coll/scan.c \
    src/mpi/coll/scatter.c \
    src/mpi/coll/scatterv.c

mpi_core_sources += \
    src/mpi/coll/helper_fns.c     \
    src/mpi/coll/nbcutil.c

noinst_HEADERS +=                    \
    src/mpi/coll/include/coll_impl.h
