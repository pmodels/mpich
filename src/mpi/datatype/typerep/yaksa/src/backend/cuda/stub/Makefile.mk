##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/cuda/stub

noinst_HEADERS += \
	src/backend/cuda/stub/yaksuri_cuda_pre.h \
	src/backend/cuda/stub/yaksuri_cuda_post.h
