##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# util comes first, sets some variables that may be used by each process
# manager's Makefile.mk
include $(top_srcdir)/src/pm/util/Makefile.mk

include $(top_srcdir)/src/pm/gforker/Makefile.mk
include $(top_srcdir)/src/pm/remshell/Makefile.mk

# Hydra has its own full automake setup, not Makefile.mk
if BUILD_PM_HYDRA
SUBDIRS += src/pm/hydra
MANDOC_SUBDIRS += src/pm/hydra
HTMLDOC_SUBDIRS += src/pm/hydra
endif BUILD_PM_HYDRA
