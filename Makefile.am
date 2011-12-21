# -*- Mode: Makefile; -*-
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

ACLOCAL_AMFLAGS = -I confdb

# automake requires that we initialize variables to something, even just empty,
# before appending to them with "+="
AM_CPPFLAGS =
AM_FFLAGS =
AM_FCFLAGS =
include_HEADERS =
nodist_include_HEADERS =
nodist_noinst_HEADERS =
BUILT_SOURCES =
bin_PROGRAMS =
bin_SCRIPTS =
dist_noinst_SCRIPTS =
noinst_DATA =
dist_noinst_DATA =
noinst_HEADERS =
noinst_LIBRARIES =
sysconf_DATA =
nodist_pkgconfig_DATA =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
SUFFIXES = 
EXTRA_DIST =
DOC_SUBDIRS =
INSTALL_DATA_LOCAL_TARGETS =

# used by nodist_pkgconfig_DATA
pkgconfigdir = $(libdir)/pkgconfig

# contains all of the "errnames.txt" files that are used by maint/extracterrmsgs
# to build src/mpi/errhan/defmsg.h
errnames_txt_files = 

external_subdirs = src/mpl src/openpa
# FIXME how/where does external_libs/ldflags get used?
external_ldflags = -L$(top_builddir)/src/mpl -L$(top_builddir)/src/openpa/src
external_libs = -lmpl -lopa

# NOTE on our semi-unconventional usage of DIST_SUBDIRS:
# The automake manual recommends thinking of DIST_SUBDIRS as the list of all
# *configured* subdirectories.  The normal autotools model involves
# unconditional configure steps for all/most directories with conditional make
# steps (i.e., conditional inclusion in SUBDIRS).  But in the MPICH2 case we
# almost always configure exactly the directories that will be included in
# SUBDIRS.  So most additions to DIST_SUBDIRS should be conditional on the same
# condition as the corresponding SUBDIRS addition.
DIST_SUBDIRS = ${external_subdirs}
SUBDIRS = ${external_subdirs}


# lib/lib@MPILIBNAME@.la always contains the "PMPI_" functions and all of the
# functions and symbols that actually make up the MPICH2 implementation.  If
# weak symbols are fully supported (such as on Linux), then this library also
# contains weak symbols for "MPI_" aliasing the "PMPI_" symbols.  This lib also
# always contains the F77 bindings, but not the F90 or C++ bindings.
#
# if weak symbols are not supported (such as on Darwin), then
# lib/lib@PMPILIBNAME@.la contains duplicate builds of all of the "PMPI_" routines
# as "MPI_" routines, but none of the implementation routines under the hood
# (such as MPIR_Bcast_impl).
lib_LTLIBRARIES = lib/lib@MPILIBNAME@.la
lib_lib@MPILIBNAME@_la_SOURCES =
nodist_lib_lib@MPILIBNAME@_la_SOURCES =
lib_lib@MPILIBNAME@_la_LIBADD = $(external_libs)
lib_lib@MPILIBNAME@_la_LDFLAGS = $(external_ldflags) -version-info $(ABIVERSION)

## Automake attempts to guess the correct linker among the various compilers
## for each language (see "How the Linker is Chosen" in the AM manual).
## However, this process is static and doesn't assume that you will "disable"
## Fortran support for a library and still actually build that library.
## lib@MPILIBNAME@.la contains both C and F77 source, so AM picks "F77LD" as the
## linker.  Instead we manually override automake's choice based on the value of
## enable_f77.
if BUILD_F77_BINDING
# link with libtool+F77LD
lib_lib@MPILIBNAME@_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=F77 \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link $(F77LD) \
	$(AM_FFLAGS) $(FFLAGS) $(lib_lib@MPILIBNAME@_la_LDFLAGS) \
	$(LDFLAGS) -o $@
else !BUILD_F77_BINDING
# link with libtool+CCLD
lib_lib@MPILIBNAME@_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link $(CCLD) \
	$(lib_lib@MPILIBNAME@_la_CFLAGS) $(CFLAGS) \
	$(lib_lib@MPILIBNAME@_la_LDFLAGS) $(LDFLAGS) -o $@
endif !BUILD_F77_BINDING

if BUILD_PROFILING_LIB
lib_LTLIBRARIES += lib/lib@PMPILIBNAME@.la
## FIXME XXX DJG is this "-DMPICH..." flag is in CFLAGS now, should it really be in CPPFLAGS?
lib_lib@PMPILIBNAME@_la_CFLAGS = $(AM_CFLAGS) -DMPICH_MPI_FROM_PMPI
lib_lib@PMPILIBNAME@_la_SOURCES =
lib_lib@PMPILIBNAME@_la_LIBADD =
lib_lib@PMPILIBNAME@_la_LDFLAGS = -version-info $(ABIVERSION)
endif BUILD_PROFILING_LIB

# these are files that provide "MPI_" functions with "PMPI_" profiling versions
# (for the C language bindings only, fortran & C++ are handled separately)
mpi_sources = 

# include our subdir automake fragments
include maint/Makefile.mk
include src/Makefile.mk

lib_lib@MPILIBNAME@_la_SOURCES += $(mpi_sources)

if BUILD_PROFILING_LIB
lib_lib@PMPILIBNAME@_la_SOURCES += $(mpi_sources)
endif BUILD_PROFILING_LIB

# build the current directory and then the examples directory after everything else
SUBDIRS += . examples
DIST_SUBDIRS += . examples test

EXTRA_DIST += README CHANGES COPYRIGHT README.envvar

# README.vin is tricky, we don't actually distribute it when we use release.pl,
# but we do need to distribute it in order for ./maint/updatefiles to work
# correctly on a distribution tarball.  For now we distribute it when we "make
# dist".
EXTRA_DIST += README.vin

testing:
	( cd test && $(MAKE) testing )

install-exec-local:
## FIXME this should live in src/mpi/debugger/Makefile.mk, right?
	-if [ "$(BUILD_TVDLL)" = "yes" ] ; then \
	    $(INSTALL_DATA) src/mpi/debugger/libtvmpich2.$(SHLIB_EXT) \
			${DESTDIR}${libdir}/libtvmpich2.$(SHLIB_EXT) ; fi

# the configure step checks for the 'lib' dir to decide whether a source
# directory has been previously used for an in-path build before allowing a
# VPATH build to continue
distclean-local:
	-rm -rf $(top_builddir)/lib
	-rm -rf $(top_builddir)/bin
	-rm -rf $(top_builddir)/include
	-rm -rf $(top_builddir)/share

# created by confdb/aclocal_cache.m4
DISTCLEANFILES += config.system
# created by configure
DISTCLEANFILES += .mpich2

# we can only have one clean-local, so we hook into it via conditionally
# defined variables in the dependencies section
clean-local: $(COVERAGE_CLEAN_TARGET)

# --------------------------------------------------------------------------
#  coverage rules
# --------------------------------------------------------------------------

.PHONY: coverage

if BUILD_COVERAGE

# $(lib_lib@MPILIBNAME@_la_SOURCES) already contains $(mpi_sources)
# We don't include the $(lib_lib@PMPILIBNAME@_la_SOURCES) here because of gcov's
# inability to correctly handle multiple object files with the same source file.
gcov_sources = $(lib_lib@MPILIBNAME@_la_SOURCES)

# local var meaning (for profiling foo/bar/baz.c):
#   dir=foo/bar
#   bname=baz.c
#   aux=baz
#
# Note that we delete all extra ".gcov" files that were not the direct goal,
# even though they contain useful information.  A more robust solution is needed
# in the long run, probably one that uses "--long-file-names --preserve-paths"
# and some postprocessing in a perl script.
#
# Gcov has no good way to deal with source files that have been compiled twice
# into two different object files, such as
#   bcast.c --> {bcast.o,lib_libpmpich_la-bcast.o}
# It still generates a bcast.c.gcov output for each object file.  Long file
# names and preserved paths do not help with this problem.
coverage:
	@for file in $(gcov_sources) ; do \
	    dir=`dirname $$file` ; \
	    bname=`basename $$file` ; \
	    aux=`echo $$bname | sed -e 's,\.*$$,,'` ; \
	    echo "( $(GCOV) -b -f -o $$file $$file && mv $${bname}.gcov $$dir )" ; \
	    ( $(GCOV) -b -f -o $$file $$file && mv $${bname}.gcov $$dir ) ; \
	    rm -f *.gcov ; \
	done
	for subdir in $(SUBDIRS) - ; do \
	    if test $$subdir = "-" ; then break ; fi ; \
	    ( cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) coverage ) ; \
	done

COVERAGE_CLEAN_TARGET=coverage-clean
# heavy handed, but it gets the job done
coverage-clean:
	-find $(top_builddir) -name "*.gcda" -o -name "*.gcno" -o -name "*.gcov" | \
	    xargs -n 10 rm -f

else !BUILD_COVERAGE
coverage:
	@echo 'ERROR: MPICH2 was not configured with "--enable-coverage"'
	exit 1
endif !BUILD_COVERAGE

# --------------------------------------------------------------------------
#  documentation rules
# --------------------------------------------------------------------------

# Tell make (via the automake SUFFIXES var) that our phony suffixes exist and
# should be considered when dissecting suffix-based implicit rules.  These phony
# suffix files should NOT be added to a .PHONY target, otherwise make will not
# use the implicit rules to attempt to build them.
SUFFIXES += .man-phony .html-phony

# "make V=1" support for our documentation recipes
doctextman_verbose = $(doctextman_verbose_$(V))
doctextman_verbose_ = $(doctextman_verbose_$(AM_DEFAULT_VERBOSITY))
doctextman_verbose_0 = @echo "   DOCTEXTMAN  " $@;
doctexthtml_verbose = $(doctexthtml_verbose_$(V))
doctexthtml_verbose_ = $(doctexthtml_verbose_$(AM_DEFAULT_VERBOSITY))
doctexthtml_verbose_0 = @echo "   DOCTEXTHTML  " $@;

# Implicit suffix rules to cause the docs to be built.  These are a bit of a
# hack, but using the implicit rules like this instead of a shell for loop
# permits make -jN to work.
.c.man-phony:
	$(doctextman_verbose)$(DOCTEXT) -man -mpath man/man3 -ext 3 -heading MPI \
		-quotefmt ${top_srcdir}/maint/docnotes $<
.c.html-phony:
	$(doctexthtml_verbose)$(DOCTEXT) -html -mpath www/www3  -heading MPI \
		-quotefmt ${top_srcdir}/maint/docnotes $<
mandir-create:
	$(MKDIR_P) $(top_builddir)/man/man3
htmldir-create:
	$(MKDIR_P) $(top_builddir)/www/www3
mandoc: mandir-create $(mpi_sources:.c=.man-phony)
htmldoc: htmldir-create $(mpi_sources:.c=.html-phony)

# install-man does not have a special "-local" target for some reason
INSTALL_DATA_LOCAL_TARGETS += install-man-local
# this is a variation on the recipe that was previously generated by simplemake
install-man-local:
	@if test -d $(builddir)/man && cd $(builddir)/man ; then \
	 for name in * ; do \
	  if [ "$$name" = "*" ] ; then continue ; fi ; \
	  if [ -f $$name ] ; then \
	    echo "$(INSTALL_DATA) $$name ${DESTDIR}${mandir}/$$name" ; \
	    $(INSTALL_DATA) $$name ${DESTDIR}${mandir}/$$name ; \
	  elif [ -d $$name ] ; then \
	    if [ ! -d ${DESTDIR}${mandir}/$$name ] ; then $(MKDIR_P) ${DESTDIR}${mandir}/$$name ; fi ;\
	    ( cd $$name && for name2 in * ; do \
	        if [ "$$name2" = "*" ] ; then continue ; fi ; \
	        if [ -f $$name2 ] ; then \
	            echo "$(INSTALL_DATA) $$name2 ${DESTDIR}${mandir}/$$name/$$name2" ; \
	            $(INSTALL_DATA) $$name2 ${DESTDIR}${mandir}/$$name/$$name2 ; \
	        elif [ -d $$name2 ] ; then  \
	            echo "cp -rp $$name2 ${DESTDIR}${mandir}/$$name" ; \
	            cp -rp $$name2 ${DESTDIR}${mandir}/$$name ; \
	        fi \
	    done ) ; \
	  else \
	    echo "Unknown file type for $$name" ; \
	  fi ; \
	 done ; \
	fi

install-html-local:
	@if test -d $(builddir)/www && cd $(builddir)/www ; then \
	 for name in * ; do \
	  if [ "$$name" = "*" ] ; then continue ; fi ; \
	  if [ -f $$name ] ; then \
	    echo "$(INSTALL_DATA) $$name ${DESTDIR}${htmldir}/$$name" ; \
	    $(INSTALL_DATA) $$name ${DESTDIR}${htmldir}/$$name ; \
	  elif [ -d $$name ] ; then \
	    if [ ! -d ${DESTDIR}${htmldir}/$$name ] ; then $(MKDIR_P) ${DESTDIR}${htmldir}/$$name ; fi ;\
	    ( cd $$name && for name2 in * ; do \
	        if [ "$$name2" = "*" ] ; then continue ; fi ; \
	        if [ -f $$name2 ] ; then \
	            echo "$(INSTALL_DATA) $$name2 ${DESTDIR}${htmldir}/$$name/$$name2" ; \
	            $(INSTALL_DATA) $$name2 ${DESTDIR}${htmldir}/$$name/$$name2 ; \
	        elif [ -d $$name2 ] ; then  \
	            echo "cp -rp $$name2 ${DESTDIR}${htmldir}/$$name" ; \
	            cp -rp $$name2 ${DESTDIR}${htmldir}/$$name ; \
	        fi \
	    done ) ; \
	  else \
	    echo "Unknown file type for $$name" ; \
	  fi ; \
	 done ; \
	fi

.PHONY: mandir-create htmldir-create mandoc htmldoc install-html-local install-man-local

# build the latex docs in the old style
latexdoc:
	for dir in $(DOC_SUBDIRS) ; do \
		(cd $$dir && ${MAKE} latexdoc ) ; done

# install-pdf will not be run by default in a "make install" unless we do this
INSTALL_DATA_LOCAL_TARGETS += install-pdf-local
# note that these PDFs are currently built in the srcdir, even in a VPATH
# build...
install-pdf-local:
	if [ -s $(srcdir)/doc/userguide/user.pdf ] ; then $(INSTALL_DATA) $(srcdir)/doc/userguide/user.pdf ${DESTDIR}${docdir}/user.pdf ; fi
	if [ -s $(srcdir)/doc/installguide/install.pdf ] ; then $(INSTALL_DATA) $(srcdir)/doc/installguide/install.pdf ${DESTDIR}${docdir}/install.pdf ; fi
	if [ -s $(srcdir)/doc/smpd/smpd_pmi.pdf ] ; then $(INSTALL_DATA) $(srcdir)/doc/smpd/smpd_pmi.pdf ${DESTDIR}${docdir}/smpd_pmi.pdf ; fi
	if [ -s $(srcdir)/doc/logging/logging.pdf ] ; then $(INSTALL_DATA) $(srcdir)/doc/logging/logging.pdf ${DESTDIR}${docdir}/logging.pdf ; fi

include $(top_srcdir)/doc/Makefile.mk

# use the _TARGETS var as a workaround for being unable to define multiple
# -local targets
install-data-local: $(INSTALL_DATA_LOCAL_TARGETS)

