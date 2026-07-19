##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/hip/hooks

libyaksa_la_SOURCES += \
	src/backend/hip/hooks/yaksuri_hip_init_hooks.c \
	src/backend/hip/hooks/yaksuri_hipi_type_hooks.c \
	src/backend/hip/hooks/yaksuri_hipi_info_hooks.c
