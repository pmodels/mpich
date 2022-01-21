#
# Copyright (C) by Argonne National Laboratory
#     See COPYRIGHT in top-level directory
#

# ensure that the buildiface script ends up in the release tarball
EXTRA_DIST += src/binding/fortran/use_mpi_f08/buildiface

if BUILD_F08_BINDING

AM_FCFLAGS += @FCINCFLAG@src/binding/fortran/use_mpi_f08

mpi_fc_sources += \
	src/binding/fortran/use_mpi_f08/pmpi_f08.f90 \
	src/binding/fortran/use_mpi_f08/mpi_f08.f90 \
	src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.f90 \
	src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90 \
	src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.f90 \
	src/binding/fortran/use_mpi_f08/mpi_f08_types.f90 \
	src/binding/fortran/use_mpi_f08/mpi_c_interface.f90 \
	src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.f90 \
	src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.f90 \
	src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.f90 \
	src/binding/fortran/use_mpi_f08/mpi_c_interface_types.f90

mpi_fc_modules += \
  src/binding/fortran/use_mpi_f08/$(PMPI_F08_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_F08_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_F08_CALLBACKS_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_F08_COMPILE_CONSTANTS_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_F08_LINK_CONSTANTS_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_F08_TYPES_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_CDESC_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_GLUE_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_NOBUF_NAME).$(MOD) \
  src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_TYPES_NAME).$(MOD)

f08_module_files = \
    src/binding/fortran/use_mpi_f08/$(MPI_F08_TYPES_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(MPI_F08_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_GLUE_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_TYPES_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(MPI_F08_COMPILE_CONSTANTS_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_CDESC_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(MPI_F08_CALLBACKS_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_NOBUF_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(MPI_F08_LINK_CONSTANTS_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(PMPI_F08_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_NAME).$(MOD)

BUILT_SOURCES += $(f08_module_files)
CLEANFILES += $(f08_module_files)

mpi_fc_sources += \
	src/binding/fortran/use_mpi_f08/wrappers_f/f08ts.f90 \
	src/binding/fortran/use_mpi_f08/wrappers_f/pf08ts.f90

F08_COMPILE_MODS = $(LTFCCOMPILE)
F08_COMPILE_MODS += $(FCMODOUTFLAG)src/binding/fortran/use_mpi_f08

src/binding/fortran/use_mpi_f08/mpi_f08_types.stamp: src/binding/fortran/use_mpi_f08/mpi_f08_types.f90 src/binding/fortran/use_mpi_f08/mpi_c_interface_types.stamp
	@rm -f src/binding/fortran/use_mpi_f08/mpi_f08_types.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_f08_types.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_f08_types.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_f08_types.f90 -o src/binding/fortran/use_mpi_f08/mpi_f08_types.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_f08_types.tmp src/binding/fortran/use_mpi_f08/mpi_f08_types.stamp

src/binding/fortran/use_mpi_f08/$(MPI_F08_TYPES_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_f08_types.lo : src/binding/fortran/use_mpi_f08/mpi_f08_types.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_f08_types-lock src/binding/fortran/use_mpi_f08/mpi_f08_types.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_f08_types-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_f08_types.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_f08_types.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_f08_types-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_f08_types-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_f08_types.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_F08_TYPES_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_f08_types.lo \
    src/binding/fortran/use_mpi_f08/mpi_f08_types.stamp \
    src/binding/fortran/use_mpi_f08/mpi_f08_types.tmp

src/binding/fortran/use_mpi_f08/mpi_f08.stamp: src/binding/fortran/use_mpi_f08/mpi_f08.f90 src/binding/fortran/use_mpi_f08/pmpi_f08.stamp
	@rm -f src/binding/fortran/use_mpi_f08/mpi_f08.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_f08.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_f08.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_f08.f90 -o src/binding/fortran/use_mpi_f08/mpi_f08.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_f08.tmp src/binding/fortran/use_mpi_f08/mpi_f08.stamp

src/binding/fortran/use_mpi_f08/$(MPI_F08_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_f08.lo : src/binding/fortran/use_mpi_f08/mpi_f08.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_f08-lock src/binding/fortran/use_mpi_f08/mpi_f08.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_f08-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_f08.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_f08.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_f08-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_f08-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_f08.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_F08_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_f08.lo \
    src/binding/fortran/use_mpi_f08/mpi_f08.stamp \
    src/binding/fortran/use_mpi_f08/mpi_f08.tmp

src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.stamp: src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.f90 src/binding/fortran/use_mpi_f08/mpi_f08.stamp
	@rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.f90 -o src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.tmp src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.stamp

src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_GLUE_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.lo : src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_c_interface_glue-lock src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_c_interface_glue-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_c_interface_glue-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_c_interface_glue-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_GLUE_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.lo \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.stamp \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.tmp

src/binding/fortran/use_mpi_f08/mpi_c_interface_types.stamp: src/binding/fortran/use_mpi_f08/mpi_c_interface_types.f90
	@rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface_types.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_c_interface_types.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_c_interface_types.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_c_interface_types.f90 -o src/binding/fortran/use_mpi_f08/mpi_c_interface_types.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_c_interface_types.tmp src/binding/fortran/use_mpi_f08/mpi_c_interface_types.stamp

src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_TYPES_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_c_interface_types.lo : src/binding/fortran/use_mpi_f08/mpi_c_interface_types.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_c_interface_types-lock src/binding/fortran/use_mpi_f08/mpi_c_interface_types.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_c_interface_types-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface_types.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_c_interface_types.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_c_interface_types-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_c_interface_types-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_c_interface_types.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_TYPES_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_types.lo \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_types.stamp \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_types.tmp

src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.stamp: src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90 src/binding/fortran/use_mpi_f08/mpi_f08_types.stamp
	@rm -f src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90 -o src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.tmp src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.stamp

src/binding/fortran/use_mpi_f08/$(MPI_F08_COMPILE_CONSTANTS_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.lo : src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants-lock src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_F08_COMPILE_CONSTANTS_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.lo \
    src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.stamp \
    src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.tmp

src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.stamp: src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.f90 src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.stamp
	@rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.f90 -o src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.tmp src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.stamp

src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_CDESC_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.lo : src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc-lock src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_CDESC_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.lo \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.stamp \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.tmp

src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.stamp: src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.f90 src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.stamp
	@rm -f src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.f90 -o src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.tmp src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.stamp

src/binding/fortran/use_mpi_f08/$(MPI_F08_CALLBACKS_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.lo : src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_f08_callbacks-lock src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_f08_callbacks-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_f08_callbacks-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_f08_callbacks-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_F08_CALLBACKS_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.lo \
    src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.stamp \
    src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.tmp

src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.stamp: src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.f90 src/binding/fortran/use_mpi_f08/mpi_c_interface_glue.stamp
	@rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.f90 -o src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.tmp src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.stamp

src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_NOBUF_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.lo : src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf-lock src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_NOBUF_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.lo \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.stamp \
    src/binding/fortran/use_mpi_f08/mpi_c_interface_nobuf.tmp

src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.stamp: src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.f90 src/binding/fortran/use_mpi_f08/mpi_f08_callbacks.stamp
	@rm -f src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.f90 -o src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.tmp src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.stamp

src/binding/fortran/use_mpi_f08/$(MPI_F08_LINK_CONSTANTS_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.lo : src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_f08_link_constants-lock src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_f08_link_constants-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_f08_link_constants-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_f08_link_constants-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_F08_LINK_CONSTANTS_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.lo \
    src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.stamp \
    src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.tmp

src/binding/fortran/use_mpi_f08/pmpi_f08.stamp: src/binding/fortran/use_mpi_f08/pmpi_f08.f90 src/binding/fortran/use_mpi_f08/mpi_f08_link_constants.stamp
	@rm -f src/binding/fortran/use_mpi_f08/pmpi_f08.tmp
	@touch src/binding/fortran/use_mpi_f08/pmpi_f08.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/pmpi_f08.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/pmpi_f08.f90 -o src/binding/fortran/use_mpi_f08/pmpi_f08.lo
	@mv src/binding/fortran/use_mpi_f08/pmpi_f08.tmp src/binding/fortran/use_mpi_f08/pmpi_f08.stamp

src/binding/fortran/use_mpi_f08/$(PMPI_F08_NAME).$(MOD) src/binding/fortran/use_mpi_f08/pmpi_f08.lo : src/binding/fortran/use_mpi_f08/pmpi_f08.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/pmpi_f08-lock src/binding/fortran/use_mpi_f08/pmpi_f08.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/pmpi_f08-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/pmpi_f08.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/pmpi_f08.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/pmpi_f08-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/pmpi_f08-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/pmpi_f08.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(PMPI_F08_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/pmpi_f08.lo \
    src/binding/fortran/use_mpi_f08/pmpi_f08.stamp \
    src/binding/fortran/use_mpi_f08/pmpi_f08.tmp

src/binding/fortran/use_mpi_f08/mpi_c_interface.stamp: src/binding/fortran/use_mpi_f08/mpi_c_interface.f90 src/binding/fortran/use_mpi_f08/mpi_c_interface_cdesc.stamp
	@rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface.tmp
	@touch src/binding/fortran/use_mpi_f08/mpi_c_interface.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'src/binding/fortran/use_mpi_f08/mpi_c_interface.f90' || echo '$(srcdir)/'`src/binding/fortran/use_mpi_f08/mpi_c_interface.f90 -o src/binding/fortran/use_mpi_f08/mpi_c_interface.lo
	@mv src/binding/fortran/use_mpi_f08/mpi_c_interface.tmp src/binding/fortran/use_mpi_f08/mpi_c_interface.stamp

src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_NAME).$(MOD) src/binding/fortran/use_mpi_f08/mpi_c_interface.lo : src/binding/fortran/use_mpi_f08/mpi_c_interface.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf src/binding/fortran/use_mpi_f08/mpi_c_interface-lock src/binding/fortran/use_mpi_f08/mpi_c_interface.stamp' 1 2 13 15; \
	  if mkdir src/binding/fortran/use_mpi_f08/mpi_c_interface-lock 2>/dev/null; then \
	    rm -f src/binding/fortran/use_mpi_f08/mpi_c_interface.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) src/binding/fortran/use_mpi_f08/mpi_c_interface.stamp; \
	    rmdir src/binding/fortran/use_mpi_f08/mpi_c_interface-lock; \
	  else \
	    while test -d src/binding/fortran/use_mpi_f08/mpi_c_interface-lock; do sleep 1; done; \
	    test -f src/binding/fortran/use_mpi_f08/mpi_c_interface.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += src/binding/fortran/use_mpi_f08/$(MPI_C_INTERFACE_NAME).$(MOD) \
    src/binding/fortran/use_mpi_f08/mpi_c_interface.lo \
    src/binding/fortran/use_mpi_f08/mpi_c_interface.stamp \
    src/binding/fortran/use_mpi_f08/mpi_c_interface.tmp

include $(top_srcdir)/src/binding/fortran/use_mpi_f08/wrappers_c/Makefile.mk

endif BUILD_F08_BINDING
