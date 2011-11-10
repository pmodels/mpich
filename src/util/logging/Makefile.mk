## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/src/util/logging/common/Makefile.mk

# these logging subsystems have conditionals internally to control their builds
include $(top_srcdir)/src/util/logging/rlog/Makefile.mk

# "slog_api" conditionally existed in Makefile.sm, but doesn't seem to be
# present in the current source tree, so this is commented out for now
#include $(top_srcdir)/src/util/logging/slog_api/Makefile.mk

