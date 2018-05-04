AM_CPPFLAGS += -I$(top_srcdir)/src/util/coll_selection

noinst_HEADERS +=                                           \
    src/util/coll_selection/coll_tree_bin_pre.h             \
    src/util/coll_selection/coll_tree_bin_types.h

mpi_core_sources += src/util/coll_selection/coll_tree_bin.c
