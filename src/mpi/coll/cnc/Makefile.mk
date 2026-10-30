##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_CNC

mpi_core_sources += \
    src/mpi/coll/cnc/cnc_coll.c

noinst_HEADERS += \
    src/mpi/coll/cnc/cnc_internal.h

endif BUILD_CNC
