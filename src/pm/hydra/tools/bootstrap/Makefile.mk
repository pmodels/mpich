# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/tools/bootstrap/include -I$(top_builddir)/tools/bootstrap/include

include tools/bootstrap/src/Makefile.mk
include tools/bootstrap/utils/Makefile.mk

if hydra_bss_ssh
include tools/bootstrap/ssh/Makefile.mk
endif

if hydra_bss_rsh
include tools/bootstrap/rsh/Makefile.mk
endif

if hydra_bss_fork
include tools/bootstrap/fork/Makefile.mk
endif

if hydra_bss_slurm
include tools/bootstrap/slurm/Makefile.mk
endif

if hydra_bss_persist
include tools/bootstrap/persist/Makefile.mk
endif

if hydra_bss_poe
include tools/bootstrap/poe/Makefile.mk
endif
