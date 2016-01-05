## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/util/timers -I$(top_builddir)/src/util/timers

nodist_noinst_HEADERS += \
	src/util/timers/mpiu_timer.h \
	src/util/timers/mpiu_timer_clock_gettime.h \
	src/util/timers/mpiu_timer_gethrtime.h \
	src/util/timers/mpiu_timer_mach_absolute_time.h \
	src/util/timers/mpiu_timer_device.h \
	src/util/timers/mpiu_timer_gettimeofday.h \
	src/util/timers/mpiu_timer_query_performance_counter.h \
	src/util/timers/mpiu_timer_gcc_ia64_cycle.h \
	src/util/timers/mpiu_timer_linux86_cycle.h \
	src/util/timers/mpiu_timer_win86_cycle.h

mpi_core_sources += \
	src/util/timers/mpiu_timer_clock_gettime.c \
	src/util/timers/mpiu_timer_gcc_ia64_cycle.c \
	src/util/timers/mpiu_timer_gethrtime.c \
	src/util/timers/mpiu_timer_linux86_cycle.c \
	src/util/timers/mpiu_timer_mach_absolute_time.c \
	src/util/timers/mpiu_timer_query_performance_counter.c \
	src/util/timers/mpiu_timer_win86_cycle.c \
	src/util/timers/mpiu_timer_device.c
