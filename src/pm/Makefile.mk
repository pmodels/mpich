## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# util comes first, sets some variables that may be used by each process
# manager's Makefile.mk
include $(top_srcdir)/src/pm/util/Makefile.mk

include $(top_srcdir)/src/pm/gforker/Makefile.mk
include $(top_srcdir)/src/pm/remshell/Makefile.mk

## a note about DIST_SUBDIRS:
## We conditionally add DIST_SUBDIRS entries because we conditionally configure
## these subdirectories.  See the automake manual's "Unconfigured
## Subdirectories" section, which lists this rule: "Any directory listed in
## DIST_SUBDIRS and SUBDIRS must be configured."
##
## The implication for "make dist" and friends is that we should only "make
## dist" in a tree that has been configured to enable to directories that we
## want to distribute.  Because of this, we will probably need to continue using 
## the release.pl script because various SUBDIRS are incompatible with each
## other.

# has its own full automake setup, not Makefile.mk
if BUILD_PM_HYDRA
SUBDIRS += src/pm/hydra
DIST_SUBDIRS += src/pm/hydra
MANDOC_SUBDIRS += src/pm/hydra
endif BUILD_PM_HYDRA
