## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/src/mpi/coll/allgather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/allgatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/alltoall/Makefile.mk
include $(top_srcdir)/src/mpi/coll/alltoallv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/alltoallw/Makefile.mk
include $(top_srcdir)/src/mpi/coll/barrier/Makefile.mk
include $(top_srcdir)/src/mpi/coll/bcast/Makefile.mk
include $(top_srcdir)/src/mpi/coll/exscan/Makefile.mk
include $(top_srcdir)/src/mpi/coll/gather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/gatherv/Makefile.mk

include $(top_srcdir)/src/mpi/coll/iallgather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iallgatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iallreduce/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ialltoall/Makefile.mk

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     \
    src/mpi/coll/allreduce/allreduce.c       \
    src/mpi/coll/op/op_create.c       \
    src/mpi/coll/op/op_free.c         \
    src/mpi/coll/reduce/reduce.c          \
    src/mpi/coll/scatter/scatter.c         \
    src/mpi/coll/scatterv/scatterv.c        \
    src/mpi/coll/scan/scan.c            \
    src/mpi/coll/red_scat/red_scat.c        \
    src/mpi/coll/reduce_local/reduce_local.c    \
    src/mpi/coll/op/op_commutative.c  \
    src/mpi/coll/red_scat_block/red_scat_block.c  \
    src/mpi/coll/ialltoallv/ialltoallv.c      \
    src/mpi/coll/ialltoallw/ialltoallw.c      \
    src/mpi/coll/ibarrier/ibarrier.c        \
    src/mpi/coll/ibcast/ibcast.c          \
    src/mpi/coll/iexscan/iexscan.c         \
    src/mpi/coll/igather/igather.c         \
    src/mpi/coll/igatherv/igatherv.c        \
    src/mpi/coll/ired_scat/ired_scat.c       \
    src/mpi/coll/ired_scat_block/ired_scat_block.c \
    src/mpi/coll/ireduce/ireduce.c         \
    src/mpi/coll/iscan/iscan.c           \
    src/mpi/coll/iscatter/iscatter.c        \
    src/mpi/coll/iscatterv/iscatterv.c

mpi_core_sources += \
    src/mpi/coll/allred_group/allred_group.c   \
    src/mpi/coll/barrier_group/barrier_group.c  \
    src/mpi/coll/helper_fns.c     \
    src/mpi/coll/op/opsum.c          \
    src/mpi/coll/op/opmax.c          \
    src/mpi/coll/op/opmin.c          \
    src/mpi/coll/op/opband.c         \
    src/mpi/coll/op/opbor.c          \
    src/mpi/coll/op/opbxor.c         \
    src/mpi/coll/op/opland.c         \
    src/mpi/coll/op/oplor.c          \
    src/mpi/coll/op/oplxor.c         \
    src/mpi/coll/op/opprod.c         \
    src/mpi/coll/op/opminloc.c       \
    src/mpi/coll/op/opmaxloc.c       \
    src/mpi/coll/op/opno_op.c        \
    src/mpi/coll/op/opreplace.c      \
    src/mpi/coll/nbcutil.c

noinst_HEADERS +=           \
    src/mpi/coll/collutil.h

