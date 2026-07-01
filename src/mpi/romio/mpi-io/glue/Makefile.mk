##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/mpi-io/glue/default/Makefile.mk
include $(top_srcdir)/mpi-io/glue/mpich/Makefile.mk

if !BUILD_ROMIO_EMBEDDED
romio_other_sources += \
    mpi-io/glue/large_count.c
endif
