##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

MOD              = @FCMODEXT@
MPIMOD           = @MPIMODNAME@
MPICONSTMOD      = @MPICONSTMODNAME@
MPISIZEOFMOD     = @MPISIZEOFMODNAME@
MPIBASEMOD       = @MPIBASEMODNAME@
PMPIBASEMOD       = @PMPIBASEMODNAME@
FC_COMPILE_MODS  = $(LTFCCOMPILE)

# variables for custom "silent-rules" for F90 modules
mod_verbose = $(mod_verbose_$(V))
mod_verbose_ = $(mod_verbose_$(AM_DEFAULT_VERBOSITY))
mod_verbose_0 = @echo "  MOD     " $@;

if BUILD_FC_BINDING

# We need to tell some compilers (e.g., Solaris f90) to look for modules in the
# current directory when the source file is not in the working directory (i.e.,
# in a VPATH build)
AM_FCFLAGS += @FCINCFLAG@src/binding/fortran/use_mpi

AM_CPPFLAGS += -Isrc/binding/fortran/use_mpi
noinst_HEADERS +=                     \
    src/binding/fortran/use_mpi/mpifnoext.h

# cause any .$(MOD) files to be output in the f90 bindings directory instead of
# the current directory
FC_COMPILE_MODS += $(FCMODOUTFLAG)src/binding/fortran/use_mpi

mpifort_convenience_libs += lib/libf90_mpi.la
noinst_LTLIBRARIES += lib/libf90_mpi.la

lib_libf90_mpi_la_SOURCES = \
    src/binding/fortran/use_mpi/mpi.f90 \
    src/binding/fortran/use_mpi/mpi_constants.f90 \
    src/binding/fortran/use_mpi/mpi_sizeofs.f90 \
    src/binding/fortran/use_mpi/mpi_base.f90 \
    src/binding/fortran/use_mpi/pmpi_base.f90

# FIXME: We may want to edit the mpif.h to convert Fortran77-specific
# items (such as an integer*8 used for file offsets) into the
# corresponding Fortran 90 KIND type, to accommodate compilers that
# reject non-standard features such as integer*8 (such as the Intel
# Fortran compiler with -std95).
# We need the MPI constants in a separate module for some of the
# interface definitions (the ones that need MPI_ADDRESS_KIND or
# MPI_OFFSET_KIND)
src/binding/fortran/use_mpi/mpi.$(MOD)-stamp: src/binding/fortran/use_mpi/$(MPICONSTMOD).$(MOD) src/binding/fortran/use_mpi/$(MPISIZEOFMOD).$(MOD) src/binding/fortran/use_mpi/$(MPIBASEMOD).$(MOD) src/binding/fortran/use_mpi/$(PMPIBASEMOD).$(MOD) $(srcdir)/src/binding/fortran/use_mpi/mpi.f90 src/binding/fortran/use_mpi/mpifnoext.h
	@rm -f src/binding/fortran/use_mpi/mpi-tmp
	@touch src/binding/fortran/use_mpi/mpi-tmp
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] || [ ! -f mpi.$(FCEXT) ] ; then \
	       rm -f mpi.$(FCEXT) ; \
	       $(LN_S) $(abs_top_srcdir)/src/binding/fortran/use_mpi/mpi.f90 mpi.$(FCEXT) ; \
	   fi )
	$(mod_verbose)$(FC_COMPILE_MODS) -c src/binding/fortran/use_mpi/mpi.$(FCEXT) -o src/binding/fortran/use_mpi/mpi.lo
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] || [ ! -f mpi.$(FCEXT) ] ; then \
	       rm -f mpi.$(FCEXT) ; \
	   fi )
	@mv src/binding/fortran/use_mpi/mpi-tmp src/binding/fortran/use_mpi/mpi.$(MOD)-stamp

src/binding/fortran/use_mpi/mpi.lo src/binding/fortran/use_mpi/$(MPIMOD).$(MOD): src/binding/fortran/use_mpi/mpi.$(MOD)-stamp
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi/mpi-lock src/binding/fortran/use_mpi/mpi.$(MOD)-stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi/mpi-lock 2>/dev/null; then \
## This code is being executed by the first process.
	    rm -f src/binding/fortran/use_mpi/mpi.$(MOD)-stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi/mpi.$(MOD)-stamp; \
	    rmdir src/binding/fortran/use_mpi/mpi-lock; \
	  else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	    while test -d src/binding/fortran/use_mpi/mpi-lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f src/binding/fortran/use_mpi/mpi.$(MOD)-stamp; exit $$?; \
	  fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi/mpi.$(MOD)-stamp src/binding/fortran/use_mpi/$(MPIMOD).$(MOD) src/binding/fortran/use_mpi/mpi.lo src/binding/fortran/use_mpi/mpi-tmp


src/binding/fortran/use_mpi/mpi_constants.$(MOD)-stamp: src/binding/fortran/use_mpi/mpi_constants.f90 src/binding/fortran/use_mpi/mpifnoext.h
	@rm -f src/binding/fortran/use_mpi/mpi_constants-tmp
	@touch src/binding/fortran/use_mpi/mpi_constants-tmp
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] || [ ! -f mpi_constants.$(FCEXT) ] ; then \
	       rm -f mpi_constants.$(FCEXT) ; \
	       $(LN_S) $(abs_top_srcdir)/src/binding/fortran/use_mpi/mpi_constants.f90 mpi_constants.$(FCEXT) ; \
	   fi )
	$(mod_verbose)$(FC_COMPILE_MODS) -c src/binding/fortran/use_mpi/mpi_constants.$(FCEXT) -o src/binding/fortran/use_mpi/mpi_constants.lo
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] || [ ! -f mpi_constants.$(FCEXT) ] ; then \
	       rm -f mpi_constants.$(FCEXT) ; \
	   fi )
	@mv src/binding/fortran/use_mpi/mpi_constants-tmp src/binding/fortran/use_mpi/mpi_constants.$(MOD)-stamp

src/binding/fortran/use_mpi/mpi_constants.lo src/binding/fortran/use_mpi/$(MPICONSTMOD).$(MOD): src/binding/fortran/use_mpi/mpi_constants.$(MOD)-stamp
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi/mpi_constants-lock src/binding/fortran/use_mpi/mpi_constants.$(MOD)-stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi/mpi_constants-lock 2>/dev/null; then \
## This code is being executed by the first process.
	    rm -f src/binding/fortran/use_mpi/mpi_constants.$(MOD)-stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi/mpi_constants.$(MOD)-stamp; \
	    rmdir src/binding/fortran/use_mpi/mpi_constants-lock; \
	  else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	    while test -d src/binding/fortran/use_mpi/mpi_constants-lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f src/binding/fortran/use_mpi/mpi_constants.$(MOD)-stamp; exit $$?; \
	  fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi/mpi_constants.$(MOD)-stamp src/binding/fortran/use_mpi/$(MPICONSTMOD).$(MOD) src/binding/fortran/use_mpi/mpi_constants.lo src/binding/fortran/use_mpi/mpi_constants-tmp


src/binding/fortran/use_mpi/mpi_sizeofs.$(MOD)-stamp: src/binding/fortran/use_mpi/mpi_sizeofs.f90 src/binding/fortran/use_mpi/mpifnoext.h
	@rm -f src/binding/fortran/use_mpi/mpi_sizeofs-tmp
	@touch src/binding/fortran/use_mpi/mpi_sizeofs-tmp
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] ; then \
	       rm -f mpi_sizeofs.$(FCEXT) ; \
	       $(LN_S) mpi_sizeofs.f90 mpi_sizeofs.$(FCEXT) ; \
	   fi )
	$(mod_verbose)$(FC_COMPILE_MODS) -c src/binding/fortran/use_mpi/mpi_sizeofs.$(FCEXT) -o src/binding/fortran/use_mpi/mpi_sizeofs.lo
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] ; then \
	       rm -f mpi_sizeofs.$(FCEXT) ; \
	   fi )
	@mv src/binding/fortran/use_mpi/mpi_sizeofs-tmp src/binding/fortran/use_mpi/mpi_sizeofs.$(MOD)-stamp

src/binding/fortran/use_mpi/mpi_sizeofs.lo src/binding/fortran/use_mpi/$(MPISIZEOFMOD).$(MOD): src/binding/fortran/use_mpi/mpi_sizeofs.$(MOD)-stamp
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi/mpi_sizeofs-lock src/binding/fortran/use_mpi/mpi_sizeofs.$(MOD)-stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi/mpi_sizeofs-lock 2>/dev/null; then \
## This code is being executed by the first process.
	    rm -f src/binding/fortran/use_mpi/mpi_sizeofs.$(MOD)-stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi/mpi_sizeofs.$(MOD)-stamp; \
	    rmdir src/binding/fortran/use_mpi/mpi_sizeofs-lock; \
	  else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	    while test -d src/binding/fortran/use_mpi/mpi_sizeofs-lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f src/binding/fortran/use_mpi/mpi_sizeofs.$(MOD)-stamp; exit $$?; \
	  fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi/mpi_sizeofs.$(MOD)-stamp src/binding/fortran/use_mpi/$(MPISIZEOFMOD).$(MOD) src/binding/fortran/use_mpi/mpi_sizeofs.lo src/binding/fortran/use_mpi/mpi_sizeofs-tmp


src/binding/fortran/use_mpi/mpi_base.$(MOD)-stamp: src/binding/fortran/use_mpi/mpi_base.f90 src/binding/fortran/use_mpi/$(MPICONSTMOD).$(MOD)
	@rm -f src/binding/fortran/use_mpi/mpi_base-tmp
	@touch src/binding/fortran/use_mpi/mpi_base-tmp
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] ; then \
	       rm -f mpi_base.$(FCEXT) ; \
	       $(LN_S) mpi_base.f90 mpi_base.$(FCEXT) ; \
	   fi )
	$(mod_verbose)$(FC_COMPILE_MODS) -c src/binding/fortran/use_mpi/mpi_base.$(FCEXT) -o src/binding/fortran/use_mpi/mpi_base.lo
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] ; then \
	       rm -f mpi_base.$(FCEXT) ; \
	   fi )
	@mv src/binding/fortran/use_mpi/mpi_base-tmp src/binding/fortran/use_mpi/mpi_base.$(MOD)-stamp

src/binding/fortran/use_mpi/mpi_base.lo src/binding/fortran/use_mpi/$(MPIBASEMOD).$(MOD): src/binding/fortran/use_mpi/mpi_base.$(MOD)-stamp
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi/mpi_base-lock src/binding/fortran/use_mpi/mpi_base.$(MOD)-stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi/mpi_base-lock 2>/dev/null; then \
## This code is being executed by the first process.
	    rm -f src/binding/fortran/use_mpi/mpi_base.$(MOD)-stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi/mpi_base.$(MOD)-stamp; \
	    rmdir src/binding/fortran/use_mpi/mpi_base-lock; \
	  else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	    while test -d src/binding/fortran/use_mpi/mpi_base-lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f src/binding/fortran/use_mpi/mpi_base.$(MOD)-stamp; exit $$?; \
	  fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi/mpi_base.$(MOD)-stamp src/binding/fortran/use_mpi/$(MPIBASEMOD).$(MOD) src/binding/fortran/use_mpi/mpi_base.lo src/binding/fortran/use_mpi/mpi_base-tmp

src/binding/fortran/use_mpi/pmpi_base.$(MOD)-stamp: src/binding/fortran/use_mpi/pmpi_base.f90 src/binding/fortran/use_mpi/$(MPICONSTMOD).$(MOD)
	@rm -f src/binding/fortran/use_mpi/pmpi_base-tmp
	@touch src/binding/fortran/use_mpi/pmpi_base-tmp
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] ; then \
	       rm -f pmpi_base.$(FCEXT) ; \
	       $(LN_S) pmpi_base.f90 pmpi_base.$(FCEXT) ; \
	   fi )
	$(mod_verbose)$(FC_COMPILE_MODS) -c src/binding/fortran/use_mpi/pmpi_base.$(FCEXT) -o src/binding/fortran/use_mpi/pmpi_base.lo
	@( cd src/binding/fortran/use_mpi && \
	   if [ "$(FCEXT)" != "f90" ] ; then \
	       rm -f pmpi_base.$(FCEXT) ; \
	   fi )
	@mv src/binding/fortran/use_mpi/pmpi_base-tmp src/binding/fortran/use_mpi/pmpi_base.$(MOD)-stamp

src/binding/fortran/use_mpi/pmpi_base.lo src/binding/fortran/use_mpi/$(PMPIBASEMOD).$(MOD): src/binding/fortran/use_mpi/pmpi_base.$(MOD)-stamp
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi/pmpi_base-lock src/binding/fortran/use_mpi/pmpi_base.$(MOD)-stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi/pmpi_base-lock 2>/dev/null; then \
## This code is being executed by the first process.
	    rm -f src/binding/fortran/use_mpi/pmpi_base.$(MOD)-stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi/pmpi_base.$(MOD)-stamp; \
	    rmdir src/binding/fortran/use_mpi/pmpi_base-lock; \
	  else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	    while test -d src/binding/fortran/use_mpi/pmpi_base-lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f src/binding/fortran/use_mpi/pmpi_base.$(MOD)-stamp; exit $$?; \
	  fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi/pmpi_base.$(MOD)-stamp src/binding/fortran/use_mpi/$(PMPIBASEMOD).$(MOD) src/binding/fortran/use_mpi/pmpi_base.lo src/binding/fortran/use_mpi/pmpi_base-tmp


mpi_fc_modules += \
    src/binding/fortran/use_mpi/$(MPIMOD).$(MOD) \
    src/binding/fortran/use_mpi/$(MPISIZEOFMOD).$(MOD) \
    src/binding/fortran/use_mpi/$(MPICONSTMOD).$(MOD) \
    src/binding/fortran/use_mpi/$(MPIBASEMOD).$(MOD) \
    src/binding/fortran/use_mpi/$(PMPIBASEMOD).$(MOD)

# We need a free-format version of mpif.h with no external commands,
# including declarations such as `REAL*8 MPI_WTIME, PMPI_WTIME`.
# Filtering on `PMPI_` covers those lines.
# NOTE: do not use extended regex to allow posix sed.
src/binding/fortran/use_mpi/mpifnoext.h: src/binding/fortran/mpif_h/mpif.h
	rm -f $@
	sed -e 's/^C/\!/g' -e '/EXTERNAL/d' \
	-e '/PMPI_/d' src/binding/fortran/mpif_h/mpif.h > $@

CLEANFILES += src/binding/fortran/use_mpi/mpifnoext.h

MAINTAINERCLEANFILES += $(mpi_sources) fproto.h
CLEANFILES += $(mpi_fc_modules)

# Documentation sources
# FIXME disabled for now until we sort out how to handle docs correctly in the
# new build system
#doc_sources =
#DOCDESTDIRS = html:www/www1,man:man/man1,latex:doc/refman
#doc_HTML_SOURCES  = ${doc_sources}
#doc_MAN_SOURCES   = ${doc_sources}
#doc_LATEX_SOURCES = ${doc_sources}

endif BUILD_FC_BINDING
