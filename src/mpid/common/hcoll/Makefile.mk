##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_HCOLL

mpi_core_sources +=  \
    src/mpid/common/hcoll/hcoll_init.c  \
    src/mpid/common/hcoll/hcoll_ops.c  \
    src/mpid/common/hcoll/hcoll_dtypes.c \
    src/mpid/common/hcoll/hcoll_rte.c

noinst_HEADERS += \
    src/mpid/common/hcoll/hcoll.h \
    src/mpid/common/hcoll/hcoll_dtypes.h

endif BUILD_HCOLL
