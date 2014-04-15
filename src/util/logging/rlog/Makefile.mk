## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##


if BUILD_LOGGING_RLOG

lib_LTLIBRARIES += lib/librlogutil.la
pmpi_convenience_libs += lib/librlogutil.la
lib_librlogutil_la_SOURCES =          \
    src/util/logging/rlog/rlog.c      \
    src/util/logging/rlog/rlogutil.c  \
    src/util/logging/rlog/rlogtime.c  \
    src/util/logging/rlog/irlogutil.c

# was "minalignrlog" intentionally omitted from the bin_PROGRAMS (install_BIN in
# simplemake terminology) or was that an oversight?
bin_PROGRAMS +=                      \
    src/util/logging/rlog/printrlog  \
    src/util/logging/rlog/printirlog \
    src/util/logging/rlog/irlog2rlog

src_util_logging_rlog_irlog2rlog_SOURCES   = src/util/logging/rlog/irlog2rlog.c
src_util_logging_rlog_irlog2rlog_LDADD     = lib/librlogutil.la
src_util_logging_rlog_printirlog_SOURCES   = src/util/logging/rlog/printirlog.c
src_util_logging_rlog_printirlog_LDADD     = lib/librlogutil.la
src_util_logging_rlog_printrlog_SOURCES    = src/util/logging/rlog/printrlog.c
src_util_logging_rlog_printrlog_LDADD      = lib/librlogutil.la
#src_util_logging_rlog_minalignrlog_SOURCES = src/util/logging/rlog/minalignrlog.c $(irlog_sources)

endif BUILD_LOGGING_RLOG

