
AM_CPPFLAGS = -I${srcdir}/../include -I../include

mtest.$(OBJEXT): mtest.c
nbc_pmpi_adapter.$(OBJEXT): nbc_pmpi_adapter.c
all-local: mtest.$(OBJEXT)  nbc_pmpi_adapter.$(OBJEXT)

EXTRA_PROGRAMS = mtestcheck
mtestcheck_SOURCES = mtestcheck.c mtest.c

# exploiting the NBC PMPI adapter is still very much a manual process...
EXTRA_DIST = nbc_pmpi_adapter.c

