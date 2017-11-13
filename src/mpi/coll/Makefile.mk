## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
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
include $(top_srcdir)/src/mpi/coll/red_scat/Makefile.mk
include $(top_srcdir)/src/mpi/coll/red_scat_block/Makefile.mk
include $(top_srcdir)/src/mpi/coll/reduce/Makefile.mk
include $(top_srcdir)/src/mpi/coll/scan/Makefile.mk
include $(top_srcdir)/src/mpi/coll/scatter/Makefile.mk
include $(top_srcdir)/src/mpi/coll/scatterv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/nhb_allgather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/nhb_allgatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/nhb_alltoall/Makefile.mk
include $(top_srcdir)/src/mpi/coll/nhb_alltoallv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/nhb_alltoallw/Makefile.mk

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
include $(top_srcdir)/src/mpi/coll/ired_scat/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ired_scat_block/Makefile.mk
include $(top_srcdir)/src/mpi/coll/ireduce/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iscan/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iscatter/Makefile.mk
include $(top_srcdir)/src/mpi/coll/iscatterv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/inhb_allgather/Makefile.mk
include $(top_srcdir)/src/mpi/coll/inhb_allgatherv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/inhb_alltoall/Makefile.mk
include $(top_srcdir)/src/mpi/coll/inhb_alltoallv/Makefile.mk
include $(top_srcdir)/src/mpi/coll/inhb_alltoallw/Makefile.mk

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     \
    src/mpi/coll/op/op_create.c       \
    src/mpi/coll/op/op_free.c         \
    src/mpi/coll/reduce_local/reduce_local.c    \
    src/mpi/coll/op/op_commutative.c

mpi_core_sources += \
    src/mpi/coll/allred_group/allred_group.c   \
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

