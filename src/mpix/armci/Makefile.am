#
# Copyright (C) 2010. See COPYRIGHT in top-level directory.
#

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_srcdir)/src

lib_LTLIBRARIES = libarmci.la
noinst_LTLIBRARIES = libarmcii.la

libarmci_la_SOURCES = $(top_srcdir)/src/buffer.c        \
                      $(top_srcdir)/src/debug.c         \
                      $(top_srcdir)/src/groups.c        \
                      $(top_srcdir)/src/internals.c     \
	              $(top_srcdir)/src/malloc.c        \
                      $(top_srcdir)/src/gmr.c           \
                      $(top_srcdir)/src/message.c       \
                      $(top_srcdir)/src/message_gop.c   \
	              $(top_srcdir)/src/mutex.c         \
                      $(top_srcdir)/src/mutex_hdl_queue.c \
	              $(top_srcdir)/src/onesided.c      \
                      $(top_srcdir)/src/onesided_nb.c   \
	              $(top_srcdir)/src/rmw.c           \
                      $(top_srcdir)/src/strided.c       \
	              $(top_srcdir)/src/topology.c      \
                      $(top_srcdir)/src/util.c          \
                      $(top_srcdir)/src/value_ops.c     \
	              $(top_srcdir)/src/vector.c        \
	              $(top_srcdir)/src/init_finalize.c \
                      $(top_srcdir)/src/conflict_tree.c

libarmci_la_LDFLAGS = -version-info $(libarmci_abi_version)

libarmcii_la_SOURCES = $(libarmci_la_SOURCES)
libarmcii_la_LDFLAGS = $(libarmci_la_LDFLAGS)

include_HEADERS = $(top_srcdir)/src/armci.h $(top_srcdir)/src/message.h \
	$(top_srcdir)/src/armcix.h

bin_PROGRAMS =
check_PROGRAMS = 
TESTS = 
XFAIL_TESTS = 

MPIEXEC ?= mpiexec -n 2
TESTS_ENVIRONMENT = $(MPIEXEC)

include benchmarks/Makefile.mk
include tests/Makefile.mk

.PHONY: checkprogs
checkprogs: $(check_PROGRAMS)
