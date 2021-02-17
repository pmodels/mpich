##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                     \
    src/mpi/coll/op/op_create.c       \
    src/mpi/coll/op/op_free.c         \
    src/mpi/coll/op/op_commutative.c

mpi_core_sources += \
    src/mpi/coll/op/oputil.c         \
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
    src/mpi/coll/op/opequal.c
