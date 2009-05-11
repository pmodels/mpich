/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIEXEC_H
#define MPIEXEC_H

#include "smpd.h"
/* #include "mpidu_sock.h" */
#include <stdio.h>
#if !defined(SIGALRM) && defined (HAVE_PTHREAD_H)
#include <pthread.h>
#endif

int mp_parse_command_args(int *argcp, char **argvp[]);
int mp_parse_mpich1_configfile(char *filename, char *configfilename, int length);
void mp_print_options(void);
int mpiexec_rsh(void);
#ifdef HAVE_WINDOWS_H
void timeout_thread(void *p);
#else
#ifdef SIGALRM
void timeout_function(int signo);
#else
#ifdef HAVE_PTHREAD_H
void *timeout_thread(void *p);
#endif
#endif
#endif

#endif
