# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include utils/args/Makefile.mk
include utils/dbg/Makefile.mk
include utils/env/Makefile.mk
include utils/launch/Makefile.mk
include utils/signals/Makefile.mk
include utils/sock/Makefile.mk
include utils/string/Makefile.mk
include utils/timer/Makefile.mk

if hydra_procbind
include utils/bind/Makefile.mk
include utils/plpa/Makefile.mk
endif
