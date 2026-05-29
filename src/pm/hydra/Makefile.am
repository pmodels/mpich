##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

external_subdirs = @pmi_srcdir@ @mpl_srcdir@ @hwloc_srcdir@
external_dist_subdirs = @pmi_srcdir@ @mpl_dist_srcdir@ @hwloc_srcdir@
external_includes = @pmi_includedir@ @mpl_includedir@ @hwloc_includedir@
external_ldflags = @RPATHS@
# NOTE: the order matters, libpmi depends on libmpl.
external_libs = @pmi_lib@ @mpl_lib@ @hwloc_lib@ @WRAPPER_LIBS@

internal_libs = libhydra.la

bin_PROGRAMS =
noinst_HEADERS =
DISTCLEANFILES =
EXTRA_DIST =
SUFFIXES =
doc1_src = \
    mansrc/mpiexec.adoc \
    mansrc/hydra_pmi_proxy.adoc \
    mansrc/hydra_nameserver.adoc \
    mansrc/hydra_persist.adoc

ACLOCAL_AMFLAGS = -I confdb
AM_CPPFLAGS = -I$(top_srcdir)/include $(external_includes)
AM_CFLAGS =

EXTRA_DIST += autogen.sh

noinst_LTLIBRARIES = libhydra.la
libhydra_la_SOURCES =

# The below directories contribute to libhydra
include lib/Makefile.mk
include mpiexec/Makefile.mk
include proxy/Makefile.mk
include nameserver/Makefile.mk
if hydra_bss_persist
include persist_server/Makefile.mk
endif

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
SUFFIXES += .man1-phony .html1-phony .adoc

# "make V=1" support for our documentation recipes
doctextman_verbose = $(doctextman_verbose_$(V))
doctextman_verbose_ = $(doctextman_verbose_$(AM_DEFAULT_VERBOSITY))
doctextman_verbose_0 = @echo "   ASCIIDOCMAN  " $@;
doctexthtml_verbose = $(doctexthtml_verbose_$(V))
doctexthtml_verbose_ = $(doctexthtml_verbose_$(AM_DEFAULT_VERBOSITY))
doctexthtml_verbose_0 = @echo "   ASCIIDOCHTML  " $@;

# Build dir paths where the man pages will be created. Will usually be
# overridden by MPICH make.
mandoc_path1=${DESTDIR}${sharedir}/man/man1
mandoc_path3=${DESTDIR}${sharedir}/man/man3
htmldoc_path1=${DESTDIR}${sharedir}/www/www1
htmldoc_path3=${DESTDIR}${sharedir}/www/www3

.adoc.man1-phony:
	$(doctextman_verbose)$(ASCIIDOC) -b manpage -D $(mandoc_path1) $<
.adoc.html1-phony:
	$(doctexthtml_verbose)$(ASCIIDOC) -b html5 -D $(htmldoc_path1) $<

# use mandoc-local target to force directory creation before running ASCIIDOC
mandoc:
	test -d $(mandoc_path1) || $(MKDIR_P) $(mandoc_path1)
	test -d $(mandoc_path3) || $(MKDIR_P) $(mandoc_path3)
	$(MAKE) $(AM_MAKEFLAGS) mandoc-local
mandoc-local: $(doc1_src:.adoc=.man1-phony)

# use htmldoc-local target to force directory creation before running ASCIIDOC
htmldoc:
	if test "@DOXYGEN@" != "" ; then @DOXYGEN@ hydra-doxygen.cfg ; fi
	test -d $(top_builddir)/www/www1 || $(MKDIR_P) $(top_builddir)/www/www1
	test -d $(top_builddir)/www/www3 || $(MKDIR_P) $(top_builddir)/www/www3
	$(MAKE) $(AM_MAKEFLAGS) htmldoc-local
htmldoc-local: $(doc1_src:.adoc=.html1-phony)

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

