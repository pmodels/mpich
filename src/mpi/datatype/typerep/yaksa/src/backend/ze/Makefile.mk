##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_ZE_BACKEND
include $(top_srcdir)/src/backend/ze/include/Makefile.mk
include $(top_srcdir)/src/backend/ze/hooks/Makefile.mk
include $(top_srcdir)/src/backend/ze/md/Makefile.mk
include $(top_srcdir)/src/backend/ze/pup/Makefile.mk
else
include $(top_srcdir)/src/backend/ze/stub/Makefile.mk
endif !BUILD_ZE_BACKEND
