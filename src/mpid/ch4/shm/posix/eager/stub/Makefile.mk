##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_CH4_SHM_POSIX_EAGER_STUB

noinst_HEADERS += src/mpid/ch4/shm/posix/eager/stub/stub_inline.h \
                  src/mpid/ch4/shm/posix/eager/stub/posix_eager_inline.h

mpi_core_sources += src/mpid/ch4/shm/posix/eager/stub/globals.c \
                    src/mpid/ch4/shm/posix/eager/stub/func_table.c \
                    src/mpid/ch4/shm/posix/eager/stub/stub_noinline.c
endif
