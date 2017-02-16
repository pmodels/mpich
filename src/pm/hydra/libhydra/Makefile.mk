## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

noinst_LTLIBRARIES += libhydra.la
libhydra_la_SOURCES =

include libhydra/arg/Makefile.mk
include libhydra/demux/Makefile.mk
include libhydra/env/Makefile.mk
include libhydra/exec/Makefile.mk
include libhydra/fs/Makefile.mk
include libhydra/include/Makefile.mk
include libhydra/mem/Makefile.mk
include libhydra/node/Makefile.mk
include libhydra/err/Makefile.mk
include libhydra/signal/Makefile.mk
include libhydra/sock/Makefile.mk
include libhydra/spawn/Makefile.mk
include libhydra/str/Makefile.mk
include libhydra/topo/Makefile.mk
include libhydra/tree/Makefile.mk
include libhydra/bstrap/Makefile.mk
include libhydra/hostfile/Makefile.mk
include libhydra/rmk/Makefile.mk
include libhydra/debugger/Makefile.mk
