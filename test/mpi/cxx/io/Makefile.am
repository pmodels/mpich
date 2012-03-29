# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_cxx.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.cxx" lines
AM_DEFAULT_SOURCE_EXT = .cxx

noinst_PROGRAMS =    \
    iwriteatx        \
    iwritex          \
    iwriteshx        \
    writex           \
    writeatx         \
    writeallx        \
    writeatallx      \
    writeallbex      \
    writeordx        \
    writeordbex      \
    writeshx         \
    writeatallbex    \
    iwriteatnosx     \
    iwritenosx       \
    iwriteshnosx     \
    writenosx        \
    writeatnosx      \
    writeallnosx     \
    writeatallnosx   \
    writeallbenosx   \
    writeordnosx     \
    writeordbenosx   \
    writeshnosx      \
    writeatallbenosx \
    fileerrx         \
    fileinfox        \
    shpositionx      \
    setinfox         \
    filemiscx        \
    seekavail

# We don't want to distribute these source files because they are created by
# "testmerge", hence "nodist_foo_SOURCES"
nodist_iwriteatx_SOURCES =        iwriteatx.cxx
nodist_iwritex_SOURCES =          iwritex.cxx
nodist_iwriteshx_SOURCES =        iwriteshx.cxx
nodist_writex_SOURCES =           writex.cxx
nodist_writeatx_SOURCES =         writeatx.cxx
nodist_writeallx_SOURCES =        writeallx.cxx
nodist_writeatallx_SOURCES =      writeatallx.cxx
nodist_writeallbex_SOURCES =      writeallbex.cxx
nodist_writeordx_SOURCES =        writeordx.cxx
nodist_writeordbex_SOURCES =      writeordbex.cxx
nodist_writeshx_SOURCES =         writeshx.cxx
nodist_writeatallbex_SOURCES =    writeatallbex.cxx
nodist_iwriteatnosx_SOURCES =     iwriteatnosx.cxx
nodist_iwritenosx_SOURCES =       iwritenosx.cxx
nodist_iwriteshnosx_SOURCES =     iwriteshnosx.cxx
nodist_writenosx_SOURCES =        writenosx.cxx
nodist_writeatnosx_SOURCES =      writeatnosx.cxx
nodist_writeallnosx_SOURCES =     writeallnosx.cxx
nodist_writeatallnosx_SOURCES =   writeatallnosx.cxx
nodist_writeallbenosx_SOURCES =   writeallbenosx.cxx
nodist_writeordnosx_SOURCES =     writeordnosx.cxx
nodist_writeordbenosx_SOURCES =   writeordbenosx.cxx
nodist_writeshnosx_SOURCES =      writeshnosx.cxx
nodist_writeatallbenosx_SOURCES = writeatallbenosx.cxx

# these files are genereated using testmerge (see below)
generated_io_sources =   \
    iwriteatx.cxx        \
    iwritex.cxx          \
    iwriteshx.cxx        \
    writex.cxx           \
    writeatx.cxx         \
    writeallx.cxx        \
    writeatallx.cxx      \
    writeallbex.cxx      \
    writeordx.cxx        \
    writeordbex.cxx      \
    writeshx.cxx         \
    writeatallbex.cxx    \
    iwriteatnosx.cxx     \
    iwritenosx.cxx       \
    iwriteshnosx.cxx     \
    writenosx.cxx        \
    writeatnosx.cxx      \
    writeallnosx.cxx     \
    writeatallnosx.cxx   \
    writeallbenosx.cxx   \
    writeordnosx.cxx     \
    writeordbenosx.cxx   \
    writeshnosx.cxx      \
    writeatallbenosx.cxx

EXTRA_DIST += ioharness.defn ioharness.tlt

# clean up after the test programs
clean-local:
	-rm -f .iotest.txt.*

# a parallel-safe scheme to generate the tests, see the automake-1.11.1 manual,
# section 27.9 "Handling Tools That Produce Multiple Outputs" for an explanation
# 
# Note that using testmerge at make-time adds a dependency on perl for users.
# See the note in test/mpi/f77/io/Makefile.am for more info.
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

DISTCLEANFILES = $(generated_io_sources)

