##
## Copyright (C) by Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

ACLOCAL_AMFLAGS = -I confdb
# mpif_h/ in the build directory holds the generated mpif.h and
# mpi_fortran.h, which differs from the source directory in VPATH builds
AM_CPPFLAGS = @VISIBILITY_CFLAGS@ -I$(top_srcdir) -Impif_h
AM_FFLAGS = -Impif_h
AM_FCFLAGS =

mpi_fc_modules =
mpifort_convenience_libs =

lib_LTLIBRARIES =
bin_SCRIPTS =
noinst_LTLIBRARIES =
noinst_HEADERS =
include_HEADERS =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

pkgconfigdir = @pkgconfigdir@
nodist_pkgconfig_DATA = mpich-fortran.pc

# generic name used by e.g. CMake's FindMPI
install-data-hook:
	if test "@enable_generic_pkgconfig@" = "yes" ; then \
	    cd ${DESTDIR}${pkgconfigdir} && ln -f -s mpich-fortran.pc mpi-fort.pc ; \
	fi

uninstall-local:
	if test "@enable_generic_pkgconfig@" = "yes" ; then \
	    rm -f ${DESTDIR}${pkgconfigdir}/mpi-fort.pc ; \
	fi

include $(srcdir)/mpif_h/Makefile.mk
include $(srcdir)/use_mpi/Makefile.mk
include $(srcdir)/use_mpi_f08/Makefile.mk

lib_LTLIBRARIES += lib/lib@MPIFCLIBNAME@.la
lib_lib@MPIFCLIBNAME@_la_SOURCES =
modinc_HEADERS = $(mpi_fc_modules)
lib_lib@MPIFCLIBNAME@_la_LDFLAGS = $(ABIVERSIONFLAGS) @FCLIB_LDFLAGS@
lib_lib@MPIFCLIBNAME@_la_LIBADD = $(mpifort_convenience_libs) @mpi_libs@

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
	if test -e lib/lib@MPIFCLIBNAME@.la ; then \
	    . lib/lib@MPIFCLIBNAME@.la ; \
	fi ; \
	deps= ; \
	for x in $$dependency_libs ; do \
	    case $$x in \
		*.la) ;; \
		-R*) deps="$$deps -Wl,-rpath,$${x#-R}" ;; \
		*) deps="$$deps $$x" ;; \
	    esac ; \
	done ; \
	for e in ${DESTDIR}${bindir}/@MPIFORT_NAME@ ; do \
	    if test -e $${e} ; then \
		sed -e 's|__PREFIX_TO_BE_FILLED_AT_INSTALL_TIME__|${prefix}|g' \
		    -e 's|__EXEC_PREFIX_TO_BE_FILLED_AT_INSTALL_TIME__|${exec_prefix}|g' \
		    -e 's|__SYSCONFDIR_TO_BE_FILLED_AT_INSTALL_TIME__|${sysconfdir}|g' \
		    -e 's|__INCLUDEDIR_TO_BE_FILLED_AT_INSTALL_TIME__|${includedir}|g' \
		    -e 's|__LIBDIR_TO_BE_FILLED_AT_INSTALL_TIME__|${libdir}|g' \
		    -e "s|__LIBS_TO_BE_FILLED_AT_INSTALL_TIME__|$$deps|g" \
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

# man pages of the C functions in this library (the f2c functions), generated
# by "make mandoc" from the mansrc made by "autogen.sh --with-doc", or shipped
# pregenerated in $(srcdir)/man by a release tarball
doc3_src =
include $(srcdir)/mansrc/Makefile.mk
# the Fortran-only routines, which have no C function to generate a page from
doc3_src += doc/MPI_F_sync_reg.adoc doc/MPI_Sizeof.adoc

SUFFIXES = .man3-phony .html3-phony .adoc

# "make V=1" support for our documentation recipes
doctextman_verbose = $(doctextman_verbose_$(V))
doctextman_verbose_ = $(doctextman_verbose_$(AM_DEFAULT_VERBOSITY))
doctextman_verbose_0 = @echo "   ASCIIDOCMAN  " $@;
doctexthtml_verbose = $(doctexthtml_verbose_$(V))
doctexthtml_verbose_ = $(doctexthtml_verbose_$(AM_DEFAULT_VERBOSITY))
doctexthtml_verbose_0 = @echo "   ASCIIDOCHTML  " $@;

mandoc_path3=$(abs_top_builddir)/man/man3
# the top-level "make htmldoc" overrides this, so the pages join MPICH's
# html index and are installed with it
htmldoc_path3=$(abs_top_builddir)/www/www3

.adoc.man3-phony:
	$(doctextman_verbose)$(ASCIIDOC) -b manpage -D $(mandoc_path3) $<
.adoc.html3-phony:
	$(doctexthtml_verbose)$(ASCIIDOC) -b html5 -D $(htmldoc_path3) $<

# use mandoc-local target to force directory creation before running ASCIIDOC
mandoc:
	test -d $(mandoc_path3) || $(MKDIR_P) $(mandoc_path3)
	$(MAKE) $(AM_MAKEFLAGS) mandoc-local
mandoc-local: $(doc3_src:.adoc=.man3-phony)

htmldoc:
	test -d $(htmldoc_path3) || $(MKDIR_P) $(htmldoc_path3)
	$(MAKE) $(AM_MAKEFLAGS) htmldoc-local
htmldoc-local: $(doc3_src:.adoc=.html3-phony)

if INSTALL_DOCS
install-data-local: install-man-local
endif
install-man-local:
	@dir= ; \
	if test -d $(builddir)/man/man3 ; then \
	    dir=$(builddir)/man/man3 ; \
	elif test -d $(srcdir)/man/man3 ; then \
	    dir=$(srcdir)/man/man3 ; \
	fi ; \
	if test -n "$$dir" ; then \
	    $(MKDIR_P) $(DESTDIR)$(mandir)/man3 ; \
	    for f in $$dir/*.3 ; do \
	        if test -f $$f ; then \
	            echo "$(INSTALL_DATA) $$f $(DESTDIR)$(mandir)/man3" ; \
	            $(INSTALL_DATA) $$f $(DESTDIR)$(mandir)/man3 ; \
	        fi ; \
	    done ; \
	fi
