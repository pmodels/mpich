# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist.in

# FIXME should "datatype" be included in this list?  It was not in the
# simplemake version for some reason and is also missing from the testlist
# file
static_subdirs = attr coll comm datatype group pt2pt rma topo basic faults
all_lang_subdirs = cxx f77 f90 f08
SUBDIRS = $(static_subdirs) $(iodir) $(otherlangs) $(spawndir)
DIST_SUBDIRS = $(static_subdirs) io $(all_lang_subdirs) spawn
