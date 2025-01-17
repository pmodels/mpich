##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/include -I$(top_srcdir)/src/util

include $(top_srcdir)/src/mpi/Makefile.mk
include $(top_srcdir)/src/util/Makefile.mk
include $(top_srcdir)/src/binding/Makefile.mk
include $(top_srcdir)/src/env/Makefile.mk
include $(top_srcdir)/src/glue/Makefile.mk
include $(top_srcdir)/src/include/Makefile.mk
include $(top_srcdir)/src/mpid/Makefile.mk
include $(top_srcdir)/src/mpi_t/Makefile.mk
include $(top_srcdir)/src/nameserv/Makefile.mk
include $(top_srcdir)/src/packaging/Makefile.mk
include $(top_srcdir)/src/pm/Makefile.mk
