##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

ACLOCAL_AMFLAGS = -I confdb
AM_CPPFLAGS = @VISIBILITY_CFLAGS@ -I$(top_srcdir)
AM_FFLAGS =
AM_FCFLAGS =

mpi_fc_modules =
mpifort_convenience_libs =

lib_LTLIBRARIES =
bin_SCRIPTS =
noinst_LTLIBRARIES =
noinst_HEADERS =
nodist_include_HEADERS =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

include $(srcdir)/mpif_h/Makefile.mk
include $(srcdir)/use_mpi/Makefile.mk
include $(srcdir)/use_mpi_f08/Makefile.mk

lib_LTLIBRARIES += lib/lib@MPIFCLIBNAME@.la
lib_lib@MPIFCLIBNAME@_la_SOURCES =
modinc_HEADERS = $(mpi_fc_modules)
lib_lib@MPIFCLIBNAME@_la_LDFLAGS = $(ABIVERSIONFLAGS) @FCLIB_LDFLAGS@
lib_lib@MPIFCLIBNAME@_la_LIBADD = $(mpifort_convenience_libs)

if BUILD_F77_BINDING
if BUILD_FC_BINDING
lib_lib@MPIFCLIBNAME@_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=FC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link $(FCLD) \
	$(AM_FCFLAGS) $(FCFLAGS) $(lib_lib@MPIFCLIBNAME@_la_LDFLAGS) \
	$(LDFLAGS) -o $@
else !BUILD_FC_BINDING
lib_lib@MPIFCLIBNAME@_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=F77 \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link $(F77LD) \
	$(AM_FFLAGS) $(FFLAGS) $(lib_lib@MPIFCLIBNAME@_la_LDFLAGS) \
	$(LDFLAGS) -o $@
endif !BUILD_FC_BINDING
endif BUILD_F77_BINDING

# ---- install mpifort --------------------

bin_SCRIPTS += env/@MPIFORT_NAME@

if BUILD_BASH_SCRIPTS
shell = bash
else
shell = sh
endif

env/@MPIFORT_NAME@: env/mpifort.$(shell)
	cp -p env/mpifort.$(shell) env/@MPIFORT_NAME@

install-exec-hook:
	for e in ${DESTDIR}${bindir}/@MPIFORT_NAME@ ; do \
	    if test -e $${e} ; then \
		sed -e 's|__PREFIX_TO_BE_FILLED_AT_INSTALL_TIME__|${prefix}|g' \
		    -e 's|__EXEC_PREFIX_TO_BE_FILLED_AT_INSTALL_TIME__|${exec_prefix}|g' \
		    -e 's|__SYSCONFDIR_TO_BE_FILLED_AT_INSTALL_TIME__|${sysconfdir}|g' \
		    -e 's|__INCLUDEDIR_TO_BE_FILLED_AT_INSTALL_TIME__|${includedir}|g' \
		    -e 's|__LIBDIR_TO_BE_FILLED_AT_INSTALL_TIME__|${libdir}|g' \
		    -e "s|__LIBS_TO_BE_FILLED_AT_INSTALL_TIME__|$$dependency_libs|g" \
		    $${e} > $${e}.tmp ; \
		$(INSTALL_SCRIPT) $${e}.tmp $${e} ; \
		rm -f $${e}.tmp ; \
	    fi ; \
	done ;
if INSTALL_MPIF90
	if test -e ${DESTDIR}${bindir}/@MPIFORT_NAME@ ; then \
	    if test "@MPIFORT_NAME@" != "@MPIF90_NAME@" ; then \
		cd ${DESTDIR}${bindir} && ln -f -s @MPIFORT_NAME@ @MPIF90_NAME@ ; \
	    fi; \
	fi;
endif INSTALL_MPIF90
if INSTALL_MPIF77
	if test -e ${DESTDIR}${bindir}/@MPIFORT_NAME@ ; then \
	    if test "@MPIFORT_NAME@" != "@MPIF77_NAME@" ; then \
		cd ${DESTDIR}${bindir} && ln -f -s @MPIFORT_NAME@ @MPIF77_NAME@ ; \
	    fi; \
	fi;
endif INSTALL_MPIF77

# ---- install man pages --------------------

SUFFIXES =
doc1_src_txt = env/mpifort.txt

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
	test -d $(top_builddir)/www/www1 || $(MKDIR_P) $(top_builddir)/www/www1
	test -d $(top_builddir)/www/www3 || $(MKDIR_P) $(top_builddir)/www/www3
	$(MAKE) $(AM_MAKEFLAGS) htmldoc-local
htmldoc-local: $(doc1_src_txt:.txt=.html1-phony)

