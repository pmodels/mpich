## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/include

if BUILD_SRC_MPE2
DIST_SUBDIRS += src/mpe2
SUBDIRS += src/mpe2
endif BUILD_SRC_MPE2

include $(top_srcdir)/src/mpi/Makefile.mk
include $(top_srcdir)/src/util/Makefile.mk

include $(top_srcdir)/src/binding/Makefile.mk
include $(top_srcdir)/src/env/Makefile.mk
include $(top_srcdir)/src/glue/Makefile.mk
include $(top_srcdir)/src/include/Makefile.mk
include $(top_srcdir)/src/mpid/Makefile.mk
include $(top_srcdir)/src/mpix/Makefile.mk
include $(top_srcdir)/src/nameserv/Makefile.mk
include $(top_srcdir)/src/packaging/Makefile.mk
include $(top_srcdir)/src/pm/Makefile.mk
include $(top_srcdir)/src/pmi/Makefile.mk

