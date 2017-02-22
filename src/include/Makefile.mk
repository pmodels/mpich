## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# nodist_ b/c these are created by config.status and should not be distributed
nodist_include_HEADERS += src/include/mpi.h

## Internal headers that are created by config.status from a corresponding
## ".h.in" file.  This ensures that these files are _not_ distributed, which is
## important because they contain lots of info that is computed by configure.
nodist_noinst_HEADERS +=     \
    src/include/mpichinfo.h \
    src/include/mpichconf.h

## listed here in BUILT_SOURCES to ensure that if mpir_ext.h is out of date
## that it will be rebuilt before make recurses into src/mpi/romio and runs
## make.  This isn't normally necessary when automake is tracking all the
## dependencies, but that's not true for SUBDIRS packages
BUILT_SOURCES += src/include/mpir_ext.h

noinst_HEADERS +=                   \
    src/include/bsocket.h           \
    src/include/mpir_refcount.h     \
    src/include/mpir_refcount_global.h \
    src/include/mpir_refcount_pobj.h \
    src/include/mpir_refcount_single.h \
    src/include/mpir_refcount.h     \
    src/include/mpir_assert.h       \
    src/include/mpir_misc_post.h    \
    src/include/mpir_type_defs.h    \
    src/include/mpir_dbg.h          \
    src/include/mpir_attr_generic.h \
    src/include/mpir_attr.h         \
    src/include/mpii_f77interface.h \
    src/include/mpii_cxxinterface.h \
    src/include/mpii_fortlogical.h   \
    src/include/mpiallstates.h      \
    src/include/mpii_bsend.h          \
    src/include/mpir_cvars.h        \
    src/include/mpichconfconst.h    \
    src/include/mpir_err.h          \
    src/include/mpir_ext.h            \
    src/include/mpir_func.h         \
    src/include/mpir_coll.h         \
    src/include/mpir_comm.h         \
    src/include/mpir_debugger.h     \
    src/include/mpir_request.h      \
    src/include/mpir_status.h       \
    src/include/mpir_contextid.h    \
    src/include/mpir_objects.h      \
    src/include/mpir_pointers.h     \
    src/include/mpir_topo.h         \
    src/include/mpir_group.h        \
    src/include/mpir_errhandler.h   \
    src/include/mpir_info.h         \
    src/include/mpiimpl.h           \
    src/include/mpir_mem.h          \
    src/include/mpir_thread.h       \
    src/include/mpir_nbc.h          \
    src/include/mpir_op.h           \
    src/include/mpir_process.h      \
    src/include/mpir_utarray.h      \
    src/include/mpir_misc.h         \
    src/include/mpir_tags.h         \
    src/include/mpir_datatype.h     \
    src/include/mpir_win.h          \
    src/include/mpir_pt2pt.h        \
    src/include/nopackage.h         \
    src/include/rlog.h              \
    src/include/rlog_macros.h       \
    src/include/mpir_op_util.h

src/include/mpir_cvars.h:
	$(top_srcdir)/maint/extractcvars --dirs="`cat $(top_srcdir)/maint/cvardirs`"
