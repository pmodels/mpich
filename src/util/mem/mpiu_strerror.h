/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#if !defined(MPIU_STRERROR_H_INCLUDED)
#define MPIU_STRERROR_H_INCLUDED

/*
 * MPIU_Sterror()
 *
 * Thread safe implementation of strerror(), whenever possible. */
const char *MPIU_Strerror(int errnum);

#endif /* !defined(MPIU_STRERROR_H_INCLUDED) */
