##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/ze/stub

noinst_HEADERS += \
	src/backend/ze/stub/yaksuri_ze_pre.h \
	src/backend/ze/stub/yaksuri_ze_post.h
