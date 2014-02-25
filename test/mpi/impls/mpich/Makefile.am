# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2013 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist.in

static_subdirs = mpi_t
# For future tests - note that the IO and RMA directories are optional,
# and need to be handled as shown in this comment
#SUBDIRS      = $(static_subdirs) $(iodir) $(rmadir)
SUBDIRS      = $(static_subdirs)
# For future tests, as noted above
#DIST_SUBDIRS = $(static_subdirs) io rma
DIST_SUBDIRS = $(static_subdirs)
