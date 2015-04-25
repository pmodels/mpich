# -*- Mode: Makefile; -*-
#
# (C) 2009 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

external_subdirs = @mpl_srcdir@
external_dist_subdirs = @mpl_dist_srcdir@
external_ldflags = -L$(top_builddir)/@mpl_srcdir@
external_libs = -lmpl @WRAPPER_LIBS@

bin_PROGRAMS =
noinst_HEADERS =
DISTCLEANFILES =
EXTRA_DIST =
SUFFIXES =
doc1_src_txt =

ACLOCAL_AMFLAGS = -I confdb
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/@mpl_srcdir@/include \
	-I$(top_builddir)/@mpl_srcdir@/include

noinst_HEADERS += include/hydra.h include/hydra_server.h

EXTRA_DIST += autogen.sh

noinst_LTLIBRARIES = libhydra.la
libhydra_la_SOURCES =

# The below directories contribute to libhydra
include utils/Makefile.mk
include tools/Makefile.mk
include pm/Makefile.mk
include ui/Makefile.mk

# External subdirs should be built first, as we might depend on them
SUBDIRS = ${external_subdirs} .
# see note in MPICH Makefile.am about our usage of DIST_SUBDIRS
DIST_SUBDIRS = ${external_dist_subdirs}

# Provide an alias for mpirun to support tools that assume that
# mpiexec is called mpirun
install-exec-hook:
	if [ -x ${DESTDIR}${bindir}/@MPIEXEC_HYDRA_NAME@ ] ; then \
	    cd ${DESTDIR}${bindir} && ln -f -s @MPIEXEC_HYDRA_NAME@ @MPIEXEC_NAME@ ; \
	    cd ${DESTDIR}${bindir} && ln -f -s @MPIEXEC_HYDRA_NAME@ @MPIRUN_NAME@ ; \
	fi

# A special alternate installation target when using multiple process
# managers; this is basically the install target, but without the
# install-exec-hook
install-alt: $(bin_PROGRAMS)
	@for foo in $? ; do $(MAKE) install $$foo ; done

uninstall-local:
	rm -f ${DESTDIR}${bindir}/@MPIRUN_NAME@ ${DESTDIR}${bindir}/@MPIEXEC_NAME@

# --------------------------------------------------------------------------
#.PHONY: mandoc mandoc-local htmldoc htmldoc-local
SUFFIXES += .man-phony .html-phony .man1-phony .html1-phony .txt

# "make V=1" support for our documentation recipes
doctextman_verbose = $(doctextman_verbose_$(V))
doctextman_verbose_ = $(doctextman_verbose_$(AM_DEFAULT_VERBOSITY))
doctextman_verbose_0 = @echo "   DOCTEXTMAN  " $@;
doctexthtml_verbose = $(doctexthtml_verbose_$(V))
doctexthtml_verbose_ = $(doctexthtml_verbose_$(AM_DEFAULT_VERBOSITY))
doctexthtml_verbose_0 = @echo "   DOCTEXTHTML  " $@;

# Build dir paths where the man pages will be created. Will usually be
# overridden by MPICH make.
mandoc_path1=${DESTDIR}${sharedir}/man/man1
mandoc_path3=${DESTDIR}${sharedir}/man/man3
htmldoc_path1=${DESTDIR}${sharedir}/www/www1
htmldoc_path3=${DESTDIR}${sharedir}/www/www3
doctext_docnotes=

.c.man-phony:
	$(doctextman_verbose)$(DOCTEXT) -man -mpath $(mandoc_path3) -ext 3 \
	    -heading HYDRA -quotefmt $(doctext_docnotes) $<
.c.html-phony:
	$(doctexthtml_verbose)$(DOCTEXT) -html -mpath $(htmldoc_path3) \
	    -heading HYDRA -quotefmt $(doctext_docnotes) $<

.txt.man1-phony:
	$(doctextman_verbose)$(DOCTEXT) -man -mpath $(mandoc_path1) -ext 1 \
	    -heading HYDRA -quotefmt $(doctext_docnotes) $<
.txt.html1-phony:
	$(doctexthtml_verbose)$(DOCTEXT) -html -mpath $(htmldoc_path1) \
	    -heading HYDRA -quotefmt $(doctext_docnotes) $<

# use mandoc-local target to force directory creation before running DOCTEXT
mandoc:
	test -d $(mandoc_path1) || $(MKDIR_P) $(mandoc_path1)
	test -d $(mandoc_path3) || $(MKDIR_P) $(mandoc_path3)
	$(MAKE) $(AM_MAKEFLAGS) mandoc-local
mandoc-local: $(doc1_src_txt:.txt=.man1-phony)

# use htmldoc-local target to force directory creation before running DOCTEXT
htmldoc:
	if test "@DOXYGEN@" != "" ; then @DOXYGEN@ hydra-doxygen.cfg ; fi
	test -d $(top_builddir)/www/www1 || $(MKDIR_P) $(top_builddir)/www/www1
	test -d $(top_builddir)/www/www3 || $(MKDIR_P) $(top_builddir)/www/www3
	$(MAKE) $(AM_MAKEFLAGS) htmldoc-local
htmldoc-local: $(doc1_src_txt:.txt=.man1-phony)

# created by confdb/aclocal_cache.m4
DISTCLEANFILES += config.system

# --------------------------------------------------------------------------
.PHONY: coverage
gcov_sources = $(libmpl_la_SOURCES)
# assumes that these sources were compiled appropriately ("-fprofile-arcs"
# and "-ftest-coverage")
coverage:
	@for file in $(gcov_sources) ; do \
	    dir=`dirname $$file` ; \
	    bname=`basename $$file` ; \
	    aux=`echo $$bname | sed -e 's,\.*$$,,'` ; \
	    echo "( $(GCOV) -b -f -o $$file $$file && mv $${bname}.gcov $$dir )" ; \
	    ( $(GCOV) -b -f -o $$file $$file && mv $${bname}.gcov $$dir ) ; \
	    rm -f *.gcov ; \
	done
## we don't recurse into SUBDIRS in hydra, since they are only really
## external packages like hwloc
##	for subdir in $(SUBDIRS) ; do \
##	    ( cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) coverage ) ; \
##	done
# --------------------------------------------------------------------------

