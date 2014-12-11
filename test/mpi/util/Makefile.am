
AM_CPPFLAGS = -I${srcdir}/../include -I../include

mtest.$(OBJEXT): mtest.c
mtest_datatype.$(OBJEXT): mtest_datatype.c mtest_datatype.h
dtypes.$(OBJEXT): dtypes.c
nbc_pmpi_adapter.$(OBJEXT): nbc_pmpi_adapter.c
all-local: mtest.$(OBJEXT) dtypes.$(OBJEXT) nbc_pmpi_adapter.$(OBJEXT)

EXTRA_PROGRAMS = mtestcheck dtypes
mtestcheck_SOURCES = mtestcheck.c mtest.c

# exploiting the NBC PMPI adapter is still very much a manual process...
# mtest_datatype.c and mtest_datatype_gen.c also needed
# FIXME: mtest_datatype.h belongs with the other include files, in
# ../include
EXTRA_DIST = nbc_pmpi_adapter.c mtest_datatype.c mtest_datatype.h \
	mtest_datatype_gen.c

