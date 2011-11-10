## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

lib_lib@MPILIBNAME@_la_SOURCES += \
    src/util/mem/trmem.c      \
    src/util/mem/handlemem.c  \
    src/util/mem/safestr.c    \
    src/util/mem/argstr.c     \
    src/util/mem/strerror.c

