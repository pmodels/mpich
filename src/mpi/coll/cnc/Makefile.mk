##
## Copyright (C) by Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_CNC

mpi_core_sources += \
    src/mpi/coll/cnc/cnc_coll.c

noinst_HEADERS += \
    src/mpi/coll/cnc/cnc_internal.h

endif BUILD_CNC
