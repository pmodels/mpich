# -*- Mode: Makefile; -*-
#
# (C) 2018 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#
AM_CPPFLAGS = $(MPI_H_INCLUDE)
AM_CPPFLAGS += -DDTP_MPI_DATATYPE=@DTP_DATATYPES@   # Coma separated list of MPI datatypes

pkglib_LTLIBRARIES = libdtpools.la
libdtpools_la_SOURCES = dtpools.c dtpools_internal.c
libdtpools_la_LDFLAGS = -version-info 0:0:0
libdtpools_la_CPPFLAGS = -I$(top_srcdir)/include -I../include $(AM_CPPFLAGS)
