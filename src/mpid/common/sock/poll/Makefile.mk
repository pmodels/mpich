## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

errnames_txt_files += src/mpid/common/sock/poll/errnames.txt

## this whole file is also already guarded by "if BUILD_MPID_COMMON_SOCK"
if BUILD_MPID_COMMON_SOCK_POLL

mpi_core_sources +=    \
    src/mpid/common/sock/poll/sock.c

# FIXME these ".i" files are awful and should be fixed somehow.  They are
# cobbled together via "#include" into sock.c.  They are not idempotent ".h"
# files, but rather a giant ".c" file that has been split into several files
# that should be compiled only once, and only together.
noinst_HEADERS += \
    src/mpid/common/sock/poll/sock_init.i \
    src/mpid/common/sock/poll/sock_set.i \
    src/mpid/common/sock/poll/sock_post.i \
    src/mpid/common/sock/poll/sock_immed.i \
    src/mpid/common/sock/poll/sock_misc.i \
    src/mpid/common/sock/poll/sock_wait.i \
    src/mpid/common/sock/poll/socki_util.i \
    src/mpid/common/sock/poll/mpidu_socki.h

# FIXME is top_builddir the right way to handle VPATH builds?
AM_CPPFLAGS +=                                  \
    -I${top_srcdir}/src/mpid/common/sock/poll   \
    -I${top_builddir}/src/mpid/common/sock/poll

endif BUILD_MPID_COMMON_SOCK_POLL

