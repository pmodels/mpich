##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

EXTRA_LTLIBRARIES = test/dtpools/libdtpools.la

test_dtpools_libdtpools_la_CPPFLAGS = $(test_cppflags)

test_dtpools_libdtpools_la_SOURCES = \
	test/dtpools/src/dtpools.c \
	test/dtpools/src/dtpools_custom.c \
	test/dtpools/src/dtpools_attr.c \
	test/dtpools/src/dtpools_desc.c \
	test/dtpools/src/dtpools_init_verify.c \
	test/dtpools/src/dtpools_misc.c

noinst_HEADERS += \
	test/dtpools/src/dtpools.h \
	test/dtpools/src/dtpools_internal.h
