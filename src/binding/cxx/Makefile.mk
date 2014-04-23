## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# ensure that the buildiface script ends up in the release tarball
EXTRA_DIST += src/binding/cxx/buildiface

if BUILD_CXX_BINDING

mpi_cxx_sources += src/binding/cxx/initcxx.cxx

# Update output files if the buildiface script or mpi.h.in is updated.  Use the
# buildiface-stamp to deal with the &ReplaceIfDifferent logic
cxx_buildiface_out_files = $(top_srcdir)/src/binding/cxx/mpicxx.h.in \
                           $(top_srcdir)/src/binding/cxx/initcxx.cxx
if MAINTAINER_MODE
$(cxx_buildiface_out_files): src/binding/cxx/buildiface-stamp
src/binding/cxx/buildiface-stamp: $(top_srcdir)/src/binding/cxx/buildiface $(top_srcdir)/src/include/mpi.h.in
	( cd $(top_srcdir)/src/binding/cxx && ./buildiface -nosep -initfile=cxx.vlist )
endif MAINTAINER_MODE


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
# FIXME does anything cause mpicovsimple.o to be built?
mpicovsimple.o: mpicovsimple.cxx mpicovsimple.h
	$(CXXCOMPILE) -c -DCOVERAGE_DIR='"@builddir@"' ${srcdir}/mpicovsimple.cxx
endif BUILD_COVERAGE

endif BUILD_CXX_BINDING

