##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/seq/pup

include src/backend/seq/pup/Makefile.pup.mk
include src/backend/seq/pup/Makefile.populate_pupfns.mk
