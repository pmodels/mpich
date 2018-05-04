AM_CPPFLAGS += -I$(top_srcdir)/src/util/decision_tree

noinst_HEADERS +=                                    \
    src/util/decision_tree/decision_tree_types.h

mpi_core_sources += src/util/decision_tree/decision_tree.c
