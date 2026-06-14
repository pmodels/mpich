##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/src/backend/seq/include/Makefile.mk
include $(top_srcdir)/src/backend/seq/hooks/Makefile.mk
include $(top_srcdir)/src/backend/seq/pup/Makefile.mk

nodist_noinst_SCRIPTS += \
	$(top_srcdir)/src/backend/seq/genpup.py
