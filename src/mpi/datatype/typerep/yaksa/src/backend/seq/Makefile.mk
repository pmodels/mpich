##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

EXTRA_DIST += $(top_srcdir)/src/backend/seq/genpup.py

include $(top_srcdir)/src/backend/seq/include/Makefile.mk
include $(top_srcdir)/src/backend/seq/hooks/Makefile.mk
include $(top_srcdir)/src/backend/seq/pup/Makefile.mk

nodist_noinst_SCRIPTS += \
	$(top_srcdir)/src/backend/seq/genpup.py
