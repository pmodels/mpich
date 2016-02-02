## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

errnames_txt_files += src/mpid/common/sock/errnames.txt

if BUILD_MPID_COMMON_SOCK

# FIXME is top_builddir the right way to handle VPATH builds?
AM_CPPFLAGS +=                             \
    -I${top_srcdir}/src/mpid/common/sock   \
    -I${top_builddir}/src/mpid/common/sock

noinst_HEADERS += src/mpid/common/sock/mpidu_sock.h

mpi_core_sources +=    \
    src/mpid/common/sock/sock.c

# FIXME these ".i" files are awful and should be fixed somehow.  They are
# cobbled together via "#include" into sock.c.  They are not idempotent ".h"
# files, but rather a giant ".c" file that has been split into several files
# that should be compiled only once, and only together.
noinst_HEADERS += \
    src/mpid/common/sock/sock_init.i \
    src/mpid/common/sock/sock_set.i \
    src/mpid/common/sock/sock_post.i \
    src/mpid/common/sock/sock_immed.i \
    src/mpid/common/sock/sock_misc.i \
    src/mpid/common/sock/sock_wait.i \
    src/mpid/common/sock/socki_util.i \
    src/mpid/common/sock/mpidu_socki.h

# FIXME is top_builddir the right way to handle VPATH builds?
AM_CPPFLAGS +=                                  \
    -I${top_srcdir}/src/mpid/common/sock   \
    -I${top_builddir}/src/mpid/common/sock

endif BUILD_MPID_COMMON_SOCK

