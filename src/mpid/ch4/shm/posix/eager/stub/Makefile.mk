##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_CH4_SHM_POSIX_EAGER_STUB

noinst_HEADERS += src/mpid/ch4/shm/posix/eager/stub/stub_send.h \
                  src/mpid/ch4/shm/posix/eager/stub/stub_recv.h \
                  src/mpid/ch4/shm/posix/eager/stub/posix_eager_inline.h

mpi_core_sources += src/mpid/ch4/shm/posix/eager/stub/globals.c \
                    src/mpid/ch4/shm/posix/eager/stub/func_table.c \
                    src/mpid/ch4/shm/posix/eager/stub/stub_init.c
endif
