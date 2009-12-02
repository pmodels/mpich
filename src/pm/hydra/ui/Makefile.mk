# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

noinst_LIBRARIES += libui.a
libui_a_SOURCES =

include ui/utils/Makefile.mk

if hydra_ui_mpiexec
include ui/mpiexec/Makefile.mk
endif
