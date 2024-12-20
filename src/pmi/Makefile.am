##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

ACLOCAL_AMFLAGS = -I confdb
AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES = libpmiutil.la

libpmiutil_la_SOURCES = \
    src/pmi_wire.c \
    src/pmi_msg.c \
    src/pmi_common.c \
    src/pmi_util.c

if EMBEDDED_MODE
noinst_LTLIBRARIES += libpmi.la

else

if DISABLE_PMIX
include_HEADERS = \
    include/pmi.h \
    include/pmi2.h

else

include_HEADERS = \
    include/pmi.h \
    include/pmi2.h \
    include/pmix/pmix.h \
    include/pmix/pmix_abi_support.h \
    include/pmix/pmix_abi_support_bottom.h \
    include/pmix/pmix_fns.h \
    include/pmix/pmix_macros.h \
    include/pmix/pmix_types.h
endif

lib_LTLIBRARIES = libpmi.la
endif

SUBDIRS = @mpl_srcdir@
AM_CPPFLAGS += @mpl_includedir@

libpmi_la_LIBADD = @mpl_lib@ libpmiutil.la

if DISABLE_PMIX
libpmi_la_SOURCES = \
    src/pmi_v1.c \
    src/pmi_v2.c
else

libpmi_la_SOURCES = \
    src/pmi_v1.c \
    src/pmi_v2.c \
    src/pmix.c
endif
