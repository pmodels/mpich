##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS = $(MPI_H_INCLUDE)
AM_CPPFLAGS += -DDTP_MPI_DATATYPE=@DTP_DATATYPES@   # Coma separated list of MPI datatypes

noinst_LTLIBRARIES = libdtpools.la
libdtpools_la_SOURCES = dtpools.c dtpools_misc.c dtpools_init_verify.c dtpools_desc.c dtpools_attr.c dtpools_custom.c
libdtpools_la_CPPFLAGS = -I$(top_srcdir)/include -I../include $(AM_CPPFLAGS)
