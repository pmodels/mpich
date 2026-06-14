##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/cuda/hooks

libyaksa_la_SOURCES += \
	src/backend/cuda/hooks/yaksuri_cuda_init_hooks.c \
	src/backend/cuda/hooks/yaksuri_cudai_type_hooks.c \
	src/backend/cuda/hooks/yaksuri_cudai_info_hooks.c
