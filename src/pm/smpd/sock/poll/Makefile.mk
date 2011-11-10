## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_PM_SMPD
if SMPD_SOCK_IS_POLL
# the .i files (a terrible design) are effectively header files, even though
# they contain code that should really only live in a single .c file
smpd_sock_hdrs = \
    src/pm/smpd/sock/poll/smpd_sock_immed.i \
    src/pm/smpd/sock/poll/smpd_sock_init.i \
    src/pm/smpd/sock/poll/smpd_sock_misc.i \
    src/pm/smpd/sock/poll/smpd_sock_post.i \
    src/pm/smpd/sock/poll/smpd_sock_set.i \
    src/pm/smpd/sock/poll/smpd_sock_wait.i \
    src/pm/smpd/sock/poll/smpd_socki_util.i \
    src/pm/smpd/sock/poll/smpd_util_socki.h

smpd_sock_src = src/pm/smpd/sock/poll/smpd_util_sock.c
smpd_sock_cppflags = -I$(top_srcdir)/src/pm/smpd/sock/poll

endif SMPD_SOCK_IS_POLL
endif BUILD_PM_SMPD

