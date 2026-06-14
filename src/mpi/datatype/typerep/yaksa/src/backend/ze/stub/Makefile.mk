##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/ze/stub

noinst_HEADERS += \
	src/backend/ze/stub/yaksuri_ze_pre.h \
	src/backend/ze/stub/yaksuri_ze_post.h
