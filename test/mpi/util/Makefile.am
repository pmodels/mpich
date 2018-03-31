
AM_CPPFLAGS = -I${srcdir}/../include -I../include

mtest.$(OBJEXT): mtest.c
dtypes.$(OBJEXT): dtypes.c
all-local: mtest.$(OBJEXT) dtypes.$(OBJEXT)

EXTRA_PROGRAMS = mtestcheck dtypes
mtestcheck_SOURCES = mtestcheck.c mtest.c
