
AM_CPPFLAGS = -I${srcdir}/../include -I../include

mtest.$(OBJEXT): mtest.c
mtest_datatype.$(OBJEXT): mtest_datatype.c mtest_datatype.h
dtypes.$(OBJEXT): dtypes.c
all-local: mtest.$(OBJEXT) dtypes.$(OBJEXT)

EXTRA_PROGRAMS = mtestcheck dtypes
mtestcheck_SOURCES = mtestcheck.c mtest.c

# FIXME: mtest_datatype.h belongs with the other include files, in
# ../include
EXTRA_DIST = mtest_datatype.c mtest_datatype.h \
	mtest_datatype_gen.c

