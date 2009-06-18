# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include ui/utils/Makefile.mk

if hydra_ui_mpiexec
include ui/mpiexec/Makefile.mk
endif
