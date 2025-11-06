##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_UCC

mpi_core_sources +=  \
    src/mpid/common/ucc/mpid_ucc.c  \
    src/mpid/common/ucc/mpid_ucc_barrier.c \
    src/mpid/common/ucc/mpid_ucc_bcast.c \
    src/mpid/common/ucc/mpid_ucc_gather.c \
    src/mpid/common/ucc/mpid_ucc_gatherv.c \
    src/mpid/common/ucc/mpid_ucc_scatter.c \
    src/mpid/common/ucc/mpid_ucc_scatterv.c \
    src/mpid/common/ucc/mpid_ucc_reduce.c \
    src/mpid/common/ucc/mpid_ucc_reduce_scatter.c \
    src/mpid/common/ucc/mpid_ucc_reduce_scatter_block.c \
    src/mpid/common/ucc/mpid_ucc_allgather.c \
    src/mpid/common/ucc/mpid_ucc_allgatherv.c \
    src/mpid/common/ucc/mpid_ucc_allreduce.c \
    src/mpid/common/ucc/mpid_ucc_alltoall.c \
    src/mpid/common/ucc/mpid_ucc_alltoallv.c

noinst_HEADERS += \
    src/mpid/common/ucc/mpid_ucc.h \
    src/mpid/common/ucc/mpid_ucc_pre.h \
    src/mpid/common/ucc/mpid_ucc_collops.h \
    src/mpid/common/ucc/mpid_ucc_dtypes.h \
    src/mpid/common/ucc/mpid_ucc_debug.h

endif BUILD_UCC
