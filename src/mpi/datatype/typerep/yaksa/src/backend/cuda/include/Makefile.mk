##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/cuda/include

noinst_HEADERS += \
	src/backend/cuda/include/yaksuri_cuda_pre.h \
	src/backend/cuda/include/yaksuri_cuda_post.h \
	src/backend/cuda/include/yaksuri_cudai_base.h \
	src/backend/cuda/include/yaksuri_cudai.h
