/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Unit test stand-in for src/include/mpiimpl.h. The real header pulls in
 * generated and configured headers; this one defines only what the units use.
 * Add to it as needed, keeping the real names and argument order. */

#ifndef MPIIMPL_H_INCLUDED
#define MPIIMPL_H_INCLUDED

#include "../mpl/mpl.h"

#define MPI_SUCCESS          0
#define MPI_ERR_OTHER       15
#define MPI_ERR_INTERN      16

typedef intptr_t MPI_Aint;
typedef int MPI_Datatype;
typedef int MPI_Op;

/* Types that headers under test only mention. Pointers to them are fine as
 * incomplete types; the ones used by value are placeholders whose layout is
 * NOT the real one, so units must not depend on their contents. */
typedef struct MPIR_Comm MPIR_Comm;
typedef struct MPIR_Request MPIR_Request;
typedef struct {
    int placeholder;
} MPIR_gpu_req;

/* Errors: instead of creating an error code, the MPIR_ERR_ macros format the
 * generic message, or the specific message with its arguments, into
 * unit_last_error, so a driver can print it. */
extern char unit_last_error[1024];
void unit_set_error(const char *fmt, ...);

#define MPIR_ERR_SETANDJUMP(err_, class_, gmsg_) \
    do { unit_set_error("%s", gmsg_); err_ = class_; goto fn_fail; } while (0)
#define MPIR_ERR_SETANDJUMP_N(err_, class_, gmsg_, smsg_, ...) \
    do { unit_set_error(smsg_, __VA_ARGS__); err_ = class_; goto fn_fail; } while (0)
#define MPIR_ERR_SETANDJUMP1 MPIR_ERR_SETANDJUMP_N
#define MPIR_ERR_SETANDJUMP2 MPIR_ERR_SETANDJUMP_N
#define MPIR_ERR_SETANDJUMP3 MPIR_ERR_SETANDJUMP_N
#define MPIR_ERR_SETANDJUMP4 MPIR_ERR_SETANDJUMP_N

#define MPIR_ERR_CHKANDJUMP(cond_, err_, class_, gmsg_) \
    do { if (cond_) MPIR_ERR_SETANDJUMP(err_, class_, gmsg_); } while (0)
#define MPIR_ERR_CHKANDJUMP_N(cond_, err_, class_, gmsg_, smsg_, ...) \
    do { if (cond_) MPIR_ERR_SETANDJUMP_N(err_, class_, gmsg_, smsg_, __VA_ARGS__); } while (0)
#define MPIR_ERR_CHKANDJUMP1 MPIR_ERR_CHKANDJUMP_N
#define MPIR_ERR_CHKANDJUMP2 MPIR_ERR_CHKANDJUMP_N
#define MPIR_ERR_CHKANDJUMP3 MPIR_ERR_CHKANDJUMP_N
#define MPIR_ERR_CHKANDJUMP4 MPIR_ERR_CHKANDJUMP_N

#define MPIR_ERR_CHECK(err_) do { if (err_) goto fn_fail; } while (0)
#define MPIR_ERR_POP(err_) do { goto fn_fail; } while (0)

/* Assertions are always on and abort, so a failed one fails the test even if
 * it expects an error. */
#define MPIR_Assert(a_) \
    do { \
        if (!(a_)) { \
            fprintf(stderr, "Assertion failed in file %s at line %d: %s\n", \
                    __FILE__, __LINE__, #a_); \
            abort(); \
        } \
    } while (0)
#define MPIR_Assertp(a_) MPIR_Assert(a_)

#endif /* MPIIMPL_H_INCLUDED */
