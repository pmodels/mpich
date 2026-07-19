##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if MPIO_GLUE_DEFAULT
romio_other_sources +=              \
    mpi-io/glue/default/mpio_file.c \
    mpi-io/glue/default/mpio_err.c
endif MPIO_GLUE_DEFAULT
