##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

lib@MPLLIBNAME@_la_SOURCES += \
    src/mpl_rankmap.c

include src/arch/Makefile.mk
include src/atomic/Makefile.mk
include src/bt/Makefile.mk
include src/dbg/Makefile.mk
include src/env/Makefile.mk
include src/mem/Makefile.mk
include src/msg/Makefile.mk
include src/sock/Makefile.mk
include src/str/Makefile.mk
include src/misc/Makefile.mk
include src/thread/Makefile.mk
include src/timer/Makefile.mk
include src/shm/Makefile.mk
include src/gpu/Makefile.mk
include src/gavl/Makefile.mk
