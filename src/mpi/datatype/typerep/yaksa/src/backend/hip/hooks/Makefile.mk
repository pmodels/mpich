##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/hip/hooks

libyaksa_la_SOURCES += \
	src/backend/hip/hooks/yaksuri_hip_init_hooks.c \
	src/backend/hip/hooks/yaksuri_hipi_type_hooks.c \
	src/backend/hip/hooks/yaksuri_hipi_info_hooks.c
