##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/hip/include

noinst_HEADERS += \
	src/backend/hip/include/yaksuri_hip_pre.h \
	src/backend/hip/include/yaksuri_hip_post.h \
	src/backend/hip/include/yaksuri_hipi_base.h \
	src/backend/hip/include/yaksuri_hipi.h
