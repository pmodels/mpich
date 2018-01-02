/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIR_STRERROR_H_INCLUDED
#define MPIR_STRERROR_H_INCLUDED

/*
 * MPIR_Sterror()
 *
 * Thread safe implementation of strerror(), whenever possible. */
const char *MPIR_Strerror(int errnum);

#endif /* MPIR_STRERROR_H_INCLUDED */
