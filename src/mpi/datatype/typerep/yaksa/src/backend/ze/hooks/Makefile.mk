##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/ze/hooks

libyaksa_la_SOURCES += \
	src/backend/ze/hooks/yaksuri_ze_init_hooks.c \
	src/backend/ze/hooks/yaksuri_zei_type_hooks.c \
	src/backend/ze/hooks/yaksuri_zei_info_hooks.c \
	src/backend/ze/hooks/yaksuri_zei_init_kernels.c \
	src/backend/ze/hooks/yaksuri_zei_finalize_kernels.c
