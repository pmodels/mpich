##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/pm/utils -I$(top_srcdir)/pm/pmiserv

libpm_la_SOURCES += pm/pmiserv/pmiserv_pmi.c \
    pm/pmiserv/pmiserv_pmi_v1.c \
    pm/pmiserv/pmiserv_pmi_v2.c \
    pm/pmiserv/pmiserv_pmci.c \
    pm/pmiserv/pmiserv_cb.c \
    pm/pmiserv/pmiserv_utils.c \
    pm/pmiserv/common.c \
    pm/pmiserv/pmi_v2_common.c
