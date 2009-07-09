# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/bootstrap/include -I$(top_builddir)/bootstrap/include

include bootstrap/src/Makefile.mk
include bootstrap/utils/Makefile.mk

if hydra_bss_ssh
include bootstrap/ssh/Makefile.mk
endif

if hydra_bss_rsh
include bootstrap/rsh/Makefile.mk
endif

if hydra_bss_fork
include bootstrap/fork/Makefile.mk
endif

if hydra_bss_slurm
include bootstrap/slurm/Makefile.mk
endif
