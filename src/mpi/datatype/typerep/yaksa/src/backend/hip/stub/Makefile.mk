##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/hip/stub

noinst_HEADERS += \
	src/backend/hip/stub/yaksuri_hip_pre.h \
	src/backend/hip/stub/yaksuri_hip_post.h
