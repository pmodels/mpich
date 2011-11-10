## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# ensure that the buildiface script ends up in the release tarball
EXTRA_DIST += src/binding/cxx/buildiface

if BUILD_CXX_LIB

# No profile library for C++.  All routines call the MPI, not PMPI, routines.
lib_LTLIBRARIES += lib/lib@MPICXXLIBNAME@.la

lib_lib@MPICXXLIBNAME@_la_SOURCES = \
    src/binding/cxx/initcxx.cxx
lib_lib@MPICXXLIBNAME@_la_LDFLAGS = -version-info $(ABIVERSION)

# Be careful here, "multi-target" rules don't work intuitively and require
# special tricks.  See the automake-1.11.1 manual section entitled "Handling
# Tools that Produce Many Outputs".  We use the "simple" fix here, since there
# are no phony dependencies and there are only two output files.
$(top_srcdir)/src/binding/cxx/mpicxx.h.in: src/binding/cxx/buildiface
	( cd $(top_srcdir)/src/binding/cxx && ./buildiface )

$(top_srcdir)/src/binding/cxx/initcxx.cxx: $(top_srcdir)/src/binding/cxx/mpicxx.h.in


# avoid dependency problems and attain an effect similar to simplemake's "all-preamble"
BUILT_SOURCES += src/binding/cxx/mpicxx.h

nodist_include_HEADERS +=    \
    src/binding/cxx/mpicxx.h

AM_CPPFLAGS += -I$(top_builddir)/src/binding/cxx

# TODO add documentation rules here, old simplemake rules follow:
#doc_sources =
#DOCDESTDIRS = html:www/www1,man:man/man1,latex:doc/refman
#doc_HTML_SOURCES  = \${doc_sources}
#doc_MAN_SOURCES   = \${doc_sources}
#doc_LATEX_SOURCES = \${doc_sources}

if BUILD_COVERAGE
mpicovsimple.o: mpicovsimple.cxx mpicovsimple.h
	$(CXXCOMPILE) -c -DCOVERAGE_DIR='"@builddir@"' ${srcdir}/mpicovsimple.cxx
endif BUILD_COVERAGE

endif BUILD_CXX_LIB

