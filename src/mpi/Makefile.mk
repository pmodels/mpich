##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/src/mpi/attr/Makefile.mk
include $(top_srcdir)/src/mpi/coll/Makefile.mk
include $(top_srcdir)/src/mpi/comm/Makefile.mk
include $(top_srcdir)/src/mpi/datatype/Makefile.mk
include $(top_srcdir)/src/mpi/debugger/Makefile.mk
include $(top_srcdir)/src/mpi/errhan/Makefile.mk
include $(top_srcdir)/src/mpi/group/Makefile.mk
include $(top_srcdir)/src/mpi/info/Makefile.mk
include $(top_srcdir)/src/mpi/init/Makefile.mk
include $(top_srcdir)/src/mpi/misc/Makefile.mk
include $(top_srcdir)/src/mpi/pt2pt/Makefile.mk
include $(top_srcdir)/src/mpi/request/Makefile.mk
include $(top_srcdir)/src/mpi/rma/Makefile.mk
include $(top_srcdir)/src/mpi/session/Makefile.mk
include $(top_srcdir)/src/mpi/spawn/Makefile.mk
include $(top_srcdir)/src/mpi/topo/Makefile.mk
include $(top_srcdir)/src/mpi/stream/Makefile.mk
include $(top_srcdir)/src/mpi/threadcomm/Makefile.mk

# dir is present but currently intentionally unbuilt
#include $(top_srcdir)/src/mpi/io/Makefile.mk
