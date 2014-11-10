# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2014 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f08.mtest

EXTRA_DIST = testlist.in

# avoid having to write many "foo_SOURCES = foo.f90" lines
AM_DEFAULT_SOURCE_EXT = .f90

noinst_PROGRAMS = \
    iwriteatf90 \
    iwritef90 \
    iwriteshf90 \
    writef90 \
    writeatf90 \
    writeallf90 \
    writeallbef90 \
    writeordf90 \
    writeordbef90 \
    writeshf90 \
    writeatallf90 \
    writeatallbef90 \
    fileerrf90 \
    fileinfof90 \
    shpositionf90 \
    atomicityf90 \
    miscfilef90 \
    setviewcurf90 \
    c2f90multio \
    c2f2ciof90

if BUILD_MPIX_TESTS
noinst_PROGRAMS += \
    i_fcoll_test \
    iwriteatallf90
endif

# We don't want to distribute these source files because they are created by
# "testmerge", hence "nodist_foo_SOURCES"
nodist_iwriteatf90_SOURCES     = iwriteatf90.f90
nodist_iwritef90_SOURCES       = iwritef90.f90
nodist_iwriteshf90_SOURCES     = iwriteshf90.f90
nodist_writeallbef90_SOURCES   = writeallbef90.f90
nodist_writeallf90_SOURCES     = writeallf90.f90
nodist_writeatallbef90_SOURCES = writeatallbef90.f90
nodist_writeatallf90_SOURCES   = writeatallf90.f90
nodist_writeatf90_SOURCES      = writeatf90.f90
nodist_writef90_SOURCES        = writef90.f90
nodist_writeordbef90_SOURCES   = writeordbef90.f90
nodist_writeordf90_SOURCES     = writeordf90.f90
nodist_writeshf90_SOURCES      = writeshf90.f90
nodist_iwriteatallf90_SOURCES  = iwriteatallf90.f90

c2f90multio_SOURCES     = c2f90multio.c
# this is a C only program, so we must either:
# A) prevent the makefile-wide "LDADD=mtestf90.o" from affecting this program, or
# B) link with the fortran compiler, otherwise we'll get link failures from
# compilers with runtime support libs, such as PGI
c2f90multio_LDADD =

c2f2ciof90_SOURCES = c2f2ciof90.f90 c2f902cio.c

## these header files will be distributed because they're listed in
## AC_CONFIG_FILES/AC_OUTPUT

# ensure that dependent tests will be rebuilt when headers are updated


# these files are genereated using testmerge (see below)
generated_io_sources = \
    iwriteatf90.f90        \
    iwriteatallf90.f90     \
    iwritef90.f90          \
    iwriteshf90.f90        \
    writeallbef90.f90      \
    writeallf90.f90        \
    writeatallbef90.f90    \
    writeatallf90.f90      \
    writeatf90.f90         \
    writef90.f90           \
    writeordbef90.f90      \
    writeordf90.f90        \
    writeshf90.f90

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

c2f902cio.c: $(srcdir)/../../f90/io/c2f902cio.c
	cp $(srcdir)/../../f90/io/c2f902cio.c c2f902cio.c
