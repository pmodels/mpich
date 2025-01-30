##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/seq/pup

include src/backend/seq/pup/Makefile.pup.mk
include src/backend/seq/pup/Makefile.populate_pupfns.mk
