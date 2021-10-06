##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_SHM_POSIX

include $(top_srcdir)/src/mpid/ch4/shm/posix/release_gather/Makefile.mk

noinst_HEADERS += src/mpid/ch4/shm/posix/posix_am.h        \
                  src/mpid/ch4/shm/posix/posix_coll.h      \
                  src/mpid/ch4/shm/posix/shm_inline.h      \
                  src/mpid/ch4/shm/posix/posix_noinline.h  \
                  src/mpid/ch4/shm/posix/posix_recv.h      \
                  src/mpid/ch4/shm/posix/posix_rma.h       \
                  src/mpid/ch4/shm/posix/posix_win.h       \
                  src/mpid/ch4/shm/posix/posix_impl.h      \
                  src/mpid/ch4/shm/posix/posix_probe.h     \
                  src/mpid/ch4/shm/posix/posix_request.h   \
                  src/mpid/ch4/shm/posix/posix_send.h      \
                  src/mpid/ch4/shm/posix/posix_part.h      \
                  src/mpid/ch4/shm/posix/posix_unimpl.h    \
                  src/mpid/ch4/shm/posix/posix_am_impl.h   \
                  src/mpid/ch4/shm/posix/posix_pre.h       \
                  src/mpid/ch4/shm/posix/posix_types.h     \
                  src/mpid/ch4/shm/posix/posix_progress.h  \
                  src/mpid/ch4/shm/posix/posix_coll_release_gather.h

mpi_core_sources += src/mpid/ch4/shm/posix/globals.c    \
                    src/mpid/ch4/shm/posix/posix_comm.c \
                    src/mpid/ch4/shm/posix/posix_init.c \
                    src/mpid/ch4/shm/posix/posix_op.c \
                    src/mpid/ch4/shm/posix/posix_datatype.c \
                    src/mpid/ch4/shm/posix/posix_win.c \
                    src/mpid/ch4/shm/posix/posix_part.c \
                    src/mpid/ch4/shm/posix/posix_eager_array.c

include $(top_srcdir)/src/mpid/ch4/shm/posix/eager/Makefile.mk

endif
