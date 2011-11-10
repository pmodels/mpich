## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_CH3_UTIL_FTB

lib_lib@MPILIBNAME@_la_SOURCES +=    \
    src/mpid/ch3/util/ftb/ftb.c

errnames_txt_files += src/mpid/ch3/util/ftb/errnames.txt

endif BUILD_CH3_UTIL_FTB

