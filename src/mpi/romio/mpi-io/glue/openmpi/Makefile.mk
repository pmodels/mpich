##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if MPIO_GLUE_OPENMPI
romio_other_sources +=              \
    mpi-io/glue/openmpi/mpio_file.c \
    mpi-io/glue/openmpi/mpio_err.c
endif MPIO_GLUE_OPENMPI
