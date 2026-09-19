#
# Copyright (C) by Argonne National Laboratory
#     See COPYRIGHT in top-level directory
#

if BUILD_F08_BINDING

AM_FCFLAGS += @FCINCFLAG@use_mpi_f08

mpifort_convenience_libs += lib/libf08_mpi.la
noinst_LTLIBRARIES += lib/libf08_mpi.la

lib_libf08_mpi_la_SOURCES = \
	use_mpi_f08/pmpi_f08.f90 \
	use_mpi_f08/mpi_f08.f90 \
	use_mpi_f08/mpi_f08_callbacks.f90 \
	use_mpi_f08/mpi_f08_compile_constants.f90 \
	use_mpi_f08/mpi_f08_link_constants.f90 \
	use_mpi_f08/mpi_f08_types.f90 \
	use_mpi_f08/mpi_c_interface.f90 \
	use_mpi_f08/mpi_c_interface_cdesc.f90 \
	use_mpi_f08/mpi_c_interface_glue.f90 \
	use_mpi_f08/mpi_c_interface_nobuf.f90 \
	use_mpi_f08/mpi_c_interface_types.f90 \
	use_mpi_f08/wrappers_f/f08ts.f90 \
	use_mpi_f08/wrappers_f/pf08ts.f90 \
	use_mpi_f08/wrappers_c/f08_cdesc.c \
	use_mpi_f08/wrappers_c/cdesc.c \
	use_mpi_f08/wrappers_c/comm_spawn_c.c \
	use_mpi_f08/wrappers_c/comm_spawn_multiple_c.c \
	use_mpi_f08/wrappers_c/utils.c

lib_libf08_mpi_la_CPPFLAGS = $(AM_CPPFLAGS) -I${srcdir}/use_mpi_f08/wrappers_c -Iuse_mpi_f08/wrappers_c

noinst_HEADERS += use_mpi_f08/wrappers_c/cdesc.h

mpi_fc_modules += \
  use_mpi_f08/$(PMPI_F08_NAME).$(MOD) \
  use_mpi_f08/$(MPI_F08_NAME).$(MOD) \
  use_mpi_f08/$(MPI_F08_CALLBACKS_NAME).$(MOD) \
  use_mpi_f08/$(MPI_F08_COMPILE_CONSTANTS_NAME).$(MOD) \
  use_mpi_f08/$(MPI_F08_LINK_CONSTANTS_NAME).$(MOD) \
  use_mpi_f08/$(MPI_F08_TYPES_NAME).$(MOD) \
  use_mpi_f08/$(MPI_C_INTERFACE_NAME).$(MOD) \
  use_mpi_f08/$(MPI_C_INTERFACE_CDESC_NAME).$(MOD) \
  use_mpi_f08/$(MPI_C_INTERFACE_GLUE_NAME).$(MOD) \
  use_mpi_f08/$(MPI_C_INTERFACE_NOBUF_NAME).$(MOD) \
  use_mpi_f08/$(MPI_C_INTERFACE_TYPES_NAME).$(MOD)

f08_module_files = \
    use_mpi_f08/$(MPI_F08_TYPES_NAME).$(MOD) \
    use_mpi_f08/$(MPI_F08_NAME).$(MOD) \
    use_mpi_f08/$(MPI_C_INTERFACE_GLUE_NAME).$(MOD) \
    use_mpi_f08/$(MPI_C_INTERFACE_TYPES_NAME).$(MOD) \
    use_mpi_f08/$(MPI_F08_COMPILE_CONSTANTS_NAME).$(MOD) \
    use_mpi_f08/$(MPI_C_INTERFACE_CDESC_NAME).$(MOD) \
    use_mpi_f08/$(MPI_F08_CALLBACKS_NAME).$(MOD) \
    use_mpi_f08/$(MPI_C_INTERFACE_NOBUF_NAME).$(MOD) \
    use_mpi_f08/$(MPI_F08_LINK_CONSTANTS_NAME).$(MOD) \
    use_mpi_f08/$(PMPI_F08_NAME).$(MOD) \
    use_mpi_f08/$(MPI_C_INTERFACE_NAME).$(MOD)

BUILT_SOURCES += $(f08_module_files)
CLEANFILES += $(f08_module_files)

F08_COMPILE_MODS = $(LTFCCOMPILE)
F08_COMPILE_MODS += $(FCMODOUTFLAG)use_mpi_f08

use_mpi_f08/mpi_f08_types.stamp: use_mpi_f08/mpi_f08_types.f90 use_mpi_f08/mpi_c_interface_types.stamp
	@rm -f use_mpi_f08/mpi_f08_types.tmp
	@touch use_mpi_f08/mpi_f08_types.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_f08_types.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_f08_types.f90 -o use_mpi_f08/mpi_f08_types.lo
	@mv use_mpi_f08/mpi_f08_types.tmp use_mpi_f08/mpi_f08_types.stamp

use_mpi_f08/$(MPI_F08_TYPES_NAME).$(MOD) use_mpi_f08/mpi_f08_types.lo : use_mpi_f08/mpi_f08_types.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_f08_types-lock use_mpi_f08/mpi_f08_types.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_f08_types-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_f08_types.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_f08_types.stamp; \
	    rmdir use_mpi_f08/mpi_f08_types-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_f08_types-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_f08_types.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_F08_TYPES_NAME).$(MOD) \
    use_mpi_f08/mpi_f08_types.lo \
    use_mpi_f08/mpi_f08_types.stamp \
    use_mpi_f08/mpi_f08_types.tmp

use_mpi_f08/mpi_f08.stamp: use_mpi_f08/mpi_f08.f90 use_mpi_f08/pmpi_f08.stamp
	@rm -f use_mpi_f08/mpi_f08.tmp
	@touch use_mpi_f08/mpi_f08.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_f08.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_f08.f90 -o use_mpi_f08/mpi_f08.lo
	@mv use_mpi_f08/mpi_f08.tmp use_mpi_f08/mpi_f08.stamp

use_mpi_f08/$(MPI_F08_NAME).$(MOD) use_mpi_f08/mpi_f08.lo : use_mpi_f08/mpi_f08.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_f08-lock use_mpi_f08/mpi_f08.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_f08-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_f08.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_f08.stamp; \
	    rmdir use_mpi_f08/mpi_f08-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_f08-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_f08.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_F08_NAME).$(MOD) \
    use_mpi_f08/mpi_f08.lo \
    use_mpi_f08/mpi_f08.stamp \
    use_mpi_f08/mpi_f08.tmp

use_mpi_f08/mpi_c_interface_glue.stamp: use_mpi_f08/mpi_c_interface_glue.f90 use_mpi_f08/mpi_f08.stamp
	@rm -f use_mpi_f08/mpi_c_interface_glue.tmp
	@touch use_mpi_f08/mpi_c_interface_glue.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_c_interface_glue.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_c_interface_glue.f90 -o use_mpi_f08/mpi_c_interface_glue.lo
	@mv use_mpi_f08/mpi_c_interface_glue.tmp use_mpi_f08/mpi_c_interface_glue.stamp

use_mpi_f08/$(MPI_C_INTERFACE_GLUE_NAME).$(MOD) use_mpi_f08/mpi_c_interface_glue.lo : use_mpi_f08/mpi_c_interface_glue.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_c_interface_glue-lock use_mpi_f08/mpi_c_interface_glue.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_c_interface_glue-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_c_interface_glue.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_c_interface_glue.stamp; \
	    rmdir use_mpi_f08/mpi_c_interface_glue-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_c_interface_glue-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_c_interface_glue.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_C_INTERFACE_GLUE_NAME).$(MOD) \
    use_mpi_f08/mpi_c_interface_glue.lo \
    use_mpi_f08/mpi_c_interface_glue.stamp \
    use_mpi_f08/mpi_c_interface_glue.tmp

use_mpi_f08/mpi_c_interface_types.stamp: use_mpi_f08/mpi_c_interface_types.f90
	@rm -f use_mpi_f08/mpi_c_interface_types.tmp
	@touch use_mpi_f08/mpi_c_interface_types.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_c_interface_types.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_c_interface_types.f90 -o use_mpi_f08/mpi_c_interface_types.lo
	@mv use_mpi_f08/mpi_c_interface_types.tmp use_mpi_f08/mpi_c_interface_types.stamp

use_mpi_f08/$(MPI_C_INTERFACE_TYPES_NAME).$(MOD) use_mpi_f08/mpi_c_interface_types.lo : use_mpi_f08/mpi_c_interface_types.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_c_interface_types-lock use_mpi_f08/mpi_c_interface_types.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_c_interface_types-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_c_interface_types.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_c_interface_types.stamp; \
	    rmdir use_mpi_f08/mpi_c_interface_types-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_c_interface_types-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_c_interface_types.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_C_INTERFACE_TYPES_NAME).$(MOD) \
    use_mpi_f08/mpi_c_interface_types.lo \
    use_mpi_f08/mpi_c_interface_types.stamp \
    use_mpi_f08/mpi_c_interface_types.tmp

use_mpi_f08/mpi_f08_compile_constants.stamp: use_mpi_f08/mpi_f08_compile_constants.f90 use_mpi_f08/mpi_f08_types.stamp
	@rm -f use_mpi_f08/mpi_f08_compile_constants.tmp
	@touch use_mpi_f08/mpi_f08_compile_constants.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_f08_compile_constants.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_f08_compile_constants.f90 -o use_mpi_f08/mpi_f08_compile_constants.lo
	@mv use_mpi_f08/mpi_f08_compile_constants.tmp use_mpi_f08/mpi_f08_compile_constants.stamp

use_mpi_f08/$(MPI_F08_COMPILE_CONSTANTS_NAME).$(MOD) use_mpi_f08/mpi_f08_compile_constants.lo : use_mpi_f08/mpi_f08_compile_constants.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_f08_compile_constants-lock use_mpi_f08/mpi_f08_compile_constants.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_f08_compile_constants-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_f08_compile_constants.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_f08_compile_constants.stamp; \
	    rmdir use_mpi_f08/mpi_f08_compile_constants-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_f08_compile_constants-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_f08_compile_constants.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_F08_COMPILE_CONSTANTS_NAME).$(MOD) \
    use_mpi_f08/mpi_f08_compile_constants.lo \
    use_mpi_f08/mpi_f08_compile_constants.stamp \
    use_mpi_f08/mpi_f08_compile_constants.tmp

use_mpi_f08/mpi_c_interface_cdesc.stamp: use_mpi_f08/mpi_c_interface_cdesc.f90 use_mpi_f08/mpi_c_interface_nobuf.stamp
	@rm -f use_mpi_f08/mpi_c_interface_cdesc.tmp
	@touch use_mpi_f08/mpi_c_interface_cdesc.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_c_interface_cdesc.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_c_interface_cdesc.f90 -o use_mpi_f08/mpi_c_interface_cdesc.lo
	@mv use_mpi_f08/mpi_c_interface_cdesc.tmp use_mpi_f08/mpi_c_interface_cdesc.stamp

use_mpi_f08/$(MPI_C_INTERFACE_CDESC_NAME).$(MOD) use_mpi_f08/mpi_c_interface_cdesc.lo : use_mpi_f08/mpi_c_interface_cdesc.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_c_interface_cdesc-lock use_mpi_f08/mpi_c_interface_cdesc.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_c_interface_cdesc-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_c_interface_cdesc.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_c_interface_cdesc.stamp; \
	    rmdir use_mpi_f08/mpi_c_interface_cdesc-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_c_interface_cdesc-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_c_interface_cdesc.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_C_INTERFACE_CDESC_NAME).$(MOD) \
    use_mpi_f08/mpi_c_interface_cdesc.lo \
    use_mpi_f08/mpi_c_interface_cdesc.stamp \
    use_mpi_f08/mpi_c_interface_cdesc.tmp

use_mpi_f08/mpi_f08_callbacks.stamp: use_mpi_f08/mpi_f08_callbacks.f90 use_mpi_f08/mpi_f08_compile_constants.stamp
	@rm -f use_mpi_f08/mpi_f08_callbacks.tmp
	@touch use_mpi_f08/mpi_f08_callbacks.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_f08_callbacks.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_f08_callbacks.f90 -o use_mpi_f08/mpi_f08_callbacks.lo
	@mv use_mpi_f08/mpi_f08_callbacks.tmp use_mpi_f08/mpi_f08_callbacks.stamp

use_mpi_f08/$(MPI_F08_CALLBACKS_NAME).$(MOD) use_mpi_f08/mpi_f08_callbacks.lo : use_mpi_f08/mpi_f08_callbacks.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_f08_callbacks-lock use_mpi_f08/mpi_f08_callbacks.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_f08_callbacks-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_f08_callbacks.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_f08_callbacks.stamp; \
	    rmdir use_mpi_f08/mpi_f08_callbacks-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_f08_callbacks-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_f08_callbacks.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_F08_CALLBACKS_NAME).$(MOD) \
    use_mpi_f08/mpi_f08_callbacks.lo \
    use_mpi_f08/mpi_f08_callbacks.stamp \
    use_mpi_f08/mpi_f08_callbacks.tmp

use_mpi_f08/mpi_c_interface_nobuf.stamp: use_mpi_f08/mpi_c_interface_nobuf.f90 use_mpi_f08/mpi_c_interface_glue.stamp
	@rm -f use_mpi_f08/mpi_c_interface_nobuf.tmp
	@touch use_mpi_f08/mpi_c_interface_nobuf.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_c_interface_nobuf.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_c_interface_nobuf.f90 -o use_mpi_f08/mpi_c_interface_nobuf.lo
	@mv use_mpi_f08/mpi_c_interface_nobuf.tmp use_mpi_f08/mpi_c_interface_nobuf.stamp

use_mpi_f08/$(MPI_C_INTERFACE_NOBUF_NAME).$(MOD) use_mpi_f08/mpi_c_interface_nobuf.lo : use_mpi_f08/mpi_c_interface_nobuf.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_c_interface_nobuf-lock use_mpi_f08/mpi_c_interface_nobuf.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_c_interface_nobuf-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_c_interface_nobuf.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_c_interface_nobuf.stamp; \
	    rmdir use_mpi_f08/mpi_c_interface_nobuf-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_c_interface_nobuf-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_c_interface_nobuf.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_C_INTERFACE_NOBUF_NAME).$(MOD) \
    use_mpi_f08/mpi_c_interface_nobuf.lo \
    use_mpi_f08/mpi_c_interface_nobuf.stamp \
    use_mpi_f08/mpi_c_interface_nobuf.tmp

use_mpi_f08/mpi_f08_link_constants.stamp: use_mpi_f08/mpi_f08_link_constants.f90 use_mpi_f08/mpi_f08_callbacks.stamp
	@rm -f use_mpi_f08/mpi_f08_link_constants.tmp
	@touch use_mpi_f08/mpi_f08_link_constants.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_f08_link_constants.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_f08_link_constants.f90 -o use_mpi_f08/mpi_f08_link_constants.lo
	@mv use_mpi_f08/mpi_f08_link_constants.tmp use_mpi_f08/mpi_f08_link_constants.stamp

use_mpi_f08/$(MPI_F08_LINK_CONSTANTS_NAME).$(MOD) use_mpi_f08/mpi_f08_link_constants.lo : use_mpi_f08/mpi_f08_link_constants.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_f08_link_constants-lock use_mpi_f08/mpi_f08_link_constants.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_f08_link_constants-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_f08_link_constants.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_f08_link_constants.stamp; \
	    rmdir use_mpi_f08/mpi_f08_link_constants-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_f08_link_constants-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_f08_link_constants.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_F08_LINK_CONSTANTS_NAME).$(MOD) \
    use_mpi_f08/mpi_f08_link_constants.lo \
    use_mpi_f08/mpi_f08_link_constants.stamp \
    use_mpi_f08/mpi_f08_link_constants.tmp

use_mpi_f08/pmpi_f08.stamp: use_mpi_f08/pmpi_f08.f90 use_mpi_f08/mpi_f08_link_constants.stamp
	@rm -f use_mpi_f08/pmpi_f08.tmp
	@touch use_mpi_f08/pmpi_f08.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/pmpi_f08.f90' || echo '$(srcdir)/'`use_mpi_f08/pmpi_f08.f90 -o use_mpi_f08/pmpi_f08.lo
	@mv use_mpi_f08/pmpi_f08.tmp use_mpi_f08/pmpi_f08.stamp

use_mpi_f08/$(PMPI_F08_NAME).$(MOD) use_mpi_f08/pmpi_f08.lo : use_mpi_f08/pmpi_f08.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/pmpi_f08-lock use_mpi_f08/pmpi_f08.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/pmpi_f08-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/pmpi_f08.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/pmpi_f08.stamp; \
	    rmdir use_mpi_f08/pmpi_f08-lock; \
	  else \
	    while test -d use_mpi_f08/pmpi_f08-lock; do sleep 1; done; \
	    test -f use_mpi_f08/pmpi_f08.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(PMPI_F08_NAME).$(MOD) \
    use_mpi_f08/pmpi_f08.lo \
    use_mpi_f08/pmpi_f08.stamp \
    use_mpi_f08/pmpi_f08.tmp

use_mpi_f08/mpi_c_interface.stamp: use_mpi_f08/mpi_c_interface.f90 use_mpi_f08/mpi_c_interface_cdesc.stamp
	@rm -f use_mpi_f08/mpi_c_interface.tmp
	@touch use_mpi_f08/mpi_c_interface.tmp
	$(mod_verbose)$(F08_COMPILE_MODS) -c `test -f 'use_mpi_f08/mpi_c_interface.f90' || echo '$(srcdir)/'`use_mpi_f08/mpi_c_interface.f90 -o use_mpi_f08/mpi_c_interface.lo
	@mv use_mpi_f08/mpi_c_interface.tmp use_mpi_f08/mpi_c_interface.stamp

use_mpi_f08/$(MPI_C_INTERFACE_NAME).$(MOD) use_mpi_f08/mpi_c_interface.lo : use_mpi_f08/mpi_c_interface.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf use_mpi_f08/mpi_c_interface-lock use_mpi_f08/mpi_c_interface.stamp' 1 2 13 15; \
	  if mkdir use_mpi_f08/mpi_c_interface-lock 2>/dev/null; then \
	    rm -f use_mpi_f08/mpi_c_interface.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) use_mpi_f08/mpi_c_interface.stamp; \
	    rmdir use_mpi_f08/mpi_c_interface-lock; \
	  else \
	    while test -d use_mpi_f08/mpi_c_interface-lock; do sleep 1; done; \
	    test -f use_mpi_f08/mpi_c_interface.stamp; exit $$?; \
	   fi; \
	fi

CLEANFILES += use_mpi_f08/$(MPI_C_INTERFACE_NAME).$(MOD) \
    use_mpi_f08/mpi_c_interface.lo \
    use_mpi_f08/mpi_c_interface.stamp \
    use_mpi_f08/mpi_c_interface.tmp

endif BUILD_F08_BINDING
