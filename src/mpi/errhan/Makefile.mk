## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                                            \
    src/mpi/errhan/add_error_code.c         \
    src/mpi/errhan/add_error_class.c        \
    src/mpi/errhan/add_error_string.c       \
    src/mpi/errhan/comm_call_errhandler.c   \
    src/mpi/errhan/comm_create_errhandler.c \
    src/mpi/errhan/comm_get_errhandler.c    \
    src/mpi/errhan/comm_set_errhandler.c    \
    src/mpi/errhan/errhandler_create.c      \
    src/mpi/errhan/errhandler_free.c        \
    src/mpi/errhan/errhandler_get.c         \
    src/mpi/errhan/errhandler_set.c         \
    src/mpi/errhan/error_class.c            \
    src/mpi/errhan/error_string.c           \
    src/mpi/errhan/file_create_errhandler.c \
    src/mpi/errhan/file_get_errhandler.c    \
    src/mpi/errhan/file_set_errhandler.c    \
    src/mpi/errhan/file_call_errhandler.c   \
    src/mpi/errhan/win_create_errhandler.c  \
    src/mpi/errhan/win_call_errhandler.c    \
    src/mpi/errhan/win_get_errhandler.c     \
    src/mpi/errhan/win_set_errhandler.c

mpi_core_sources +=             \
    src/mpi/errhan/errutil.c    \
    src/mpi/errhan/dynerrutil.c

noinst_HEADERS +=             \
    src/mpi/errhan/defmsg.h   \
    src/mpi/errhan/errcodes.h

errnames_txt_files += src/mpi/errhan/errnames.txt

# FIXME DUPLICATION: this list of files can be (mostly harmlessly) different
# than the list in maint/errmsgdirs because this list will be assembled
# conditionally based on configure tests and AM_CONDITIONAL usage
dist_noinst_DATA += $(errnames_txt_files) src/mpi/errhan/baseerrnames.txt
noinst_HEADERS += $(top_srcdir)/src/mpi/errhan/defmsg.h

# Only clean/rebuild defmsg.h if maintainer mode rules are enabled (we are
# cheating slightly by looking inside the implementation of
# "AM_MAINTAINER_MODE").
if MAINTAINER_MODE
# force dependency-based rebuilds of defmsg.h to happen earlier than ".c" file
# compilations via BUILT_SOURCES
BUILT_SOURCES += $(top_srcdir)/src/mpi/errhan/defmsg.h
MAINTAINERCLEANFILES += $(top_srcdir)/src/mpi/errhan/defmsg.h

# FIXME DUPLICATION
# This code is lifted from autogen.sh.  This extra logic should just be
# rolled up into the extracterrmsgs script itself.
$(top_srcdir)/src/mpi/errhan/defmsg.h: $(top_srcdir)/maint/errmsgdirs $(errnames_txt_files) src/mpi/errhan/baseerrnames.txt
	( cd $(top_srcdir) && rm -f .err unusederr.txt ; rm -rf .tmp )
	( cd $(top_srcdir) && \
	  ./maint/extracterrmsgs -careful=unusederr.txt \
				 -skip=src/util/multichannel/mpi.c \
				 `cat maint/errmsgdirs` > .tmp 2>.err )
	( cd $(top_srcdir) && if test -s .err ; then rm -f .tmp ; cat .err ; exit 1 ; fi )
	( cd $(top_srcdir) && test -s .tmp && mv .tmp src/mpi/errhan/defmsg.h )

endif MAINTAINER_MODE


