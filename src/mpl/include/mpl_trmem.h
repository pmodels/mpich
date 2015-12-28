/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPL_TRMEM_H_INCLUDED)
#define MPL_TRMEM_H_INCLUDED

/* FIXME: Consider an option of specifying __attribute__((malloc)) for
   gcc - this lets gcc-style compilers know that the returned pointer
   does not alias any pointer prior to the call.
 */
void MPL_trinit(int);
void *MPL_trmalloc(size_t, int, const char[]);
void MPL_trfree(void *, int, const char[]);
int MPL_trvalid(const char[]);
int MPL_trvalid2(const char[],int,const char[]);
void *MPL_trcalloc(size_t, size_t, int, const char[]);
void *MPL_trrealloc(void *, size_t, int, const char[]);
void *MPL_trstrdup(const char *, int, const char[]);

/* Make sure that FILE is defined */
#include <stdio.h>
void MPL_trdump(FILE *, int);

#endif /* !defined(MPL_TRMEM_H_INCLUDED) */
