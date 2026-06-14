##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/seq/include

noinst_HEADERS += \
	src/backend/seq/include/yaksuri_seqi.h \
	src/backend/seq/include/yaksuri_seq_pre.h \
	src/backend/seq/include/yaksuri_seq_post.h
