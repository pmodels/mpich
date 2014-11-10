# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f77.mtest

EXTRA_DIST = testlist.in

# avoid having to write many "foo_SOURCES = foo.f" lines
AM_DEFAULT_SOURCE_EXT = .f

noinst_PROGRAMS = \
    iwriteatf \
    iwritef \
    iwriteshf \
    writef \
    writeatf \
    writeallf \
    writeallbef \
    writeordf \
    writeordbef \
    writeshf \
    writeatallf \
    writeatallbef \
    fileerrf \
    fileinfof \
    shpositionf \
    atomicityf \
    miscfilef \
    setviewcurf \
    c2fmultio \
    c2f2ciof

if BUILD_MPIX_TESTS
noinst_PROGRAMS += \
    i_setviewcurf  \
    iwriteatallf
endif


# We don't want to distribute these source files because they are created by
# "testmerge", hence "nodist_foo_SOURCES"
nodist_iwriteatf_SOURCES     = iwriteatf.f
nodist_iwriteatallf_SOURCES  = iwriteatallf.f
nodist_iwritef_SOURCES       = iwritef.f
nodist_iwriteshf_SOURCES     = iwriteshf.f
nodist_writeallbef_SOURCES   = writeallbef.f
nodist_writeallf_SOURCES     = writeallf.f
nodist_writeatallbef_SOURCES = writeatallbef.f
nodist_writeatallf_SOURCES   = writeatallf.f
nodist_writeatf_SOURCES      = writeatf.f
nodist_writef_SOURCES        = writef.f
nodist_writeordbef_SOURCES   = writeordbef.f
nodist_writeordf_SOURCES     = writeordf.f
nodist_writeshf_SOURCES      = writeshf.f

c2fmultio_SOURCES     = c2fmultio.c
# this is a C only program, so we must either:
# A) prevent the makefile-wide "LDADD=mtestf.o" from affecting this program, or
# B) link with the fortran compiler, otherwise we'll get link failures from
# compilers with runtime support libs, such as PGI
c2fmultio_LDADD =

c2f2ciof_SOURCES      = c2f2cio.c c2f2ciof.f

## these header files will be distributed because they're listed in
## AC_CONFIG_FILES/AC_OUTPUT

# ensure that dependent tests will be rebuilt when headers are updated
atomicityf.$(OBJEXT): iodisp.h
iwriteatf.$(OBJEXT): iooffset.h
iwritef.$(OBJEXT): iooffset.h
miscfilef.$(OBJEXT): iooffset.h iodisp.h
setviewcurf.$(OBJEXT): iooffset.h
shpositionf.$(OBJEXT): iooffset.h ioaint.h
writeallbef.$(OBJEXT): iooffset.h
writeallf.$(OBJEXT): iooffset.h
writeatallbef.$(OBJEXT): iooffset.h
writeatallf.$(OBJEXT): iooffset.h
writeatf.$(OBJEXT): iooffset.h
writef.$(OBJEXT): iooffset.h
writeordbef.$(OBJEXT): iooffset.h
writeordf.$(OBJEXT): iooffset.h


# these files are genereated using testmerge (see below)
generated_io_sources = \
    iwriteatf.f        \
    iwriteatallf.f     \
    iwritef.f          \
    iwriteshf.f        \
    writeallbef.f      \
    writeallf.f        \
    writeatallbef.f    \
    writeatallf.f      \
    writeatf.f         \
    writef.f           \
    writeordbef.f      \
    writeordf.f        \
    writeshf.f

EXTRA_DIST += ioharness.defn ioharness.tlt

# a parallel-safe scheme to generate the tests, see the automake-1.11.1 manual,
# section 27.9 "Handling Tools That Produce Multiple Outputs" for an explanation
# 
# Note that using testmerge at make-time adds a dependency on perl for users.
# It's been like this for a long time, so I don't think it's a problem in
# practice.  The best way to fix this is to drop the nodist_foo_SOURCES lines,
# modify testmerge to output the stamp file, and then enclose modified versions
# of these rules in an "if MAINAINER_MODE".  The files also should then move to
# MAINTAINERCLEANFILES from DISTCLEANFILES. [goodell@ 2011-07-11]
gen-src-stamp: $(srcdir)/ioharness.defn $(srcdir)/ioharness.tlt
	@rm -f gen-src-tmp
	@touch gen-src-tmp
	$(top_builddir)/maint/testmerge -defn=$(top_srcdir)/maint/common.defn \
		$(srcdir)/ioharness.defn $(srcdir)/ioharness.tlt
	@mv -f gen-src-tmp $@

$(generated_io_sources): gen-src-stamp
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf gen-src-lock gen-src-stamp' 1 2 13 15; \
## mkdir is a portable test-and-set
	  if mkdir gen-src-lock 2>/dev/null; then \
## This code is being executed by the first process.
	    rm -f gen-src-stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) gen-src-stamp; \
	    rmdir gen-src-lock; \
	  else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	    while test -d gen-src-lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f gen-src-stamp; exit $$?; \
	  fi; \
	fi

# just in case
CLEANFILES += gen-src-tmp gen-src-stamp

# clean up after the test programs
clean-local:
	-rm -f .iotest.txt.*

DISTCLEANFILES = $(generated_io_sources)

