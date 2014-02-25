## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

libhydra_la_SOURCES += tools/bootstrap/src/bsci_init.c \
	tools/bootstrap/src/bsci_finalize.c \
	tools/bootstrap/src/bsci_launch.c \
	tools/bootstrap/src/bsci_query_node_list.c \
	tools/bootstrap/src/bsci_query_proxy_id.c \
	tools/bootstrap/src/bsci_query_native_int.c \
	tools/bootstrap/src/bsci_wait.c \
	tools/bootstrap/src/bsci_env.c
