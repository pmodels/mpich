## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# conditional includes/builds of "sock" and "ftb" controlled in the past by
# @ch3subsystems@ and @ftb_dir@ respectively

include $(top_srcdir)/src/mpid/ch3/util/sock/Makefile.mk
include $(top_srcdir)/src/mpid/ch3/util/ftb/Makefile.mk

