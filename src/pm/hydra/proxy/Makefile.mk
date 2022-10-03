##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

bin_PROGRAMS += hydra_pmi_proxy

hydra_pmi_proxy_SOURCES = \
    proxy/pmip.c \
    proxy/pmip_pg.c \
    proxy/pmip_cb.c \
    proxy/pmip_utils.c \
    proxy/pmip_pmi.c

hydra_pmi_proxy_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/proxy
hydra_pmi_proxy_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_pmi_proxy_LDADD = -lhydra $(external_libs)
hydra_pmi_proxy_DEPENDENCIES = libhydra.la

if HYDRA_HAVE_HWLOC
hydra_pmi_proxy_CPPFLAGS += -I$(top_srcdir)/lib/tools/topo/hwloc
endif
