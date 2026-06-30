##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

bin_SCRIPTS +=           \
    src/env/mpicc        \
    src/env/parkill

sysconf_DATA += src/env/mpixxx_opts.conf

bin_PROGRAMS += src/env/mpichversion \
    src/env/mpivars

src_env_mpichversion_LDADD = lib/lib@MPILIBNAME@.la
src_env_mpivars_LDADD   = lib/lib@MPILIBNAME@.la

if INSTALL_MPICXX
bin_SCRIPTS += src/env/mpicxx
endif INSTALL_MPICXX

if BUILD_BASH_SCRIPTS
shell = bash
else !BUILD_BASH_SCRIPTS
shell = sh
endif

# create a local copy of the compiler wrapper that will actually be installed
src/env/mpicc: src/env/mpicc.$(shell)
	cp -p src/env/mpicc.$(shell) src/env/mpicc
src/env/mpicxx: src/env/mpicxx.$(shell)
	cp -p src/env/mpicxx.$(shell) src/env/mpicxx

DISTCLEANFILES += $(top_builddir)/src/env/cc_shlib.conf  \
                  $(top_builddir)/src/env/cxx_shlib.conf \
                  $(top_builddir)/src/env/mpicc          \
                  $(top_builddir)/src/env/mpicxx

wrapper_doc_src = src/env/mpicc.txt \
                  src/env/mpicxx.txt \
doc1_src += $(wrapper_doc_src)
EXTRA_DIST += $(wrapper_doc_src)
