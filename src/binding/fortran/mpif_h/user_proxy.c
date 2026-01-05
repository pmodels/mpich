/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"
#include <assert.h>

#include "mpl.h"

/* ---- user op ----------------- */
struct F77_op_state {
    F77_OpFunction *opfn;
};

static void F77_op_proxy(void *invec, void *inoutvec, MPI_Count len, MPI_Datatype datatype,
                         void *extra_state)
{
    struct F77_op_state *p = extra_state;
    MPI_Fint len_i = (MPI_Fint) len;
    MPI_Fint datatype_i = MPI_Type_c2f(datatype);

    p->opfn(invec, inoutvec, &len_i, &datatype_i);
}

static void F77_op_free(void *extra_state)
{
    MPL_free(extra_state);
}

int MPII_op_create(F77_OpFunction * opfn, MPI_Fint commute, MPI_Fint * op)
{
    struct F77_op_state *p = MPL_malloc(sizeof(struct F77_op_state), MPL_MEM_OTHER);
    p->opfn = opfn;

    MPI_Op op_i;
    int ret = MPIX_Op_create_x(F77_op_proxy, F77_op_free, commute, p, &op_i);
    if (ret == MPI_SUCCESS) {
        *op = MPI_Op_c2f(op_i);
    } else {
        MPL_free(p);
    }

    return ret;
}

/* ---- user errhandler  ----------------- */
struct F77_errhan_state {
    F77_ErrFunction *err_fn;
};

static void F77_comm_errhan_proxy(MPI_Comm comm, int error_code, void *extra_state)
{
    struct F77_errhan_state *p = extra_state;
    MPI_Fint comm_i = MPI_Comm_c2f(comm);
    MPI_Fint error_code_i = error_code;

    p->err_fn(&comm_i, &error_code_i);
}

static void F77_win_errhan_proxy(MPI_Win win, int error_code, void *extra_state)
{
    struct F77_errhan_state *p = extra_state;
    MPI_Fint win_i = MPI_Win_c2f(win);
    MPI_Fint error_code_i = error_code;

    p->err_fn(&win_i, &error_code_i);
}

static void F77_file_errhan_proxy(MPI_File file, int error_code, void *extra_state)
{
    struct F77_errhan_state *p = extra_state;
    MPI_Fint file_i = MPI_File_c2f(file);
    MPI_Fint error_code_i = error_code;

    p->err_fn(&file_i, &error_code_i);
}

static void F77_session_errhan_proxy(MPI_Session session, int error_code, void *extra_state)
{
    struct F77_errhan_state *p = extra_state;
    MPI_Fint session_i = MPI_Session_c2f(session);
    MPI_Fint error_code_i = error_code;

    p->err_fn(&session_i, &error_code_i);
}

static void F77_errhan_free(void *extra_state)
{
    MPL_free(extra_state);
}

int MPII_errhan_create(F77_ErrFunction * err_fn, MPI_Fint * errhandler, enum F77_handle_type type)
{
    struct F77_errhan_state *p = MPL_malloc(sizeof(struct F77_errhan_state), MPL_MEM_OTHER);
    p->err_fn = err_fn;

    MPI_Errhandler errhandler_i;
    int ret = MPI_SUCCESS;
    switch (type) {
        case F77_COMM:
            ret = MPIX_Comm_create_errhandler_x(F77_comm_errhan_proxy, F77_errhan_free,
                                                p, &errhandler_i);
            break;
        case F77_WIN:
            ret = MPIX_Win_create_errhandler_x(F77_win_errhan_proxy, F77_errhan_free,
                                               p, &errhandler_i);
            break;
        case F77_FILE:
            ret = MPIX_File_create_errhandler_x(F77_file_errhan_proxy, F77_errhan_free,
                                                p, &errhandler_i);
            break;
        case F77_SESSION:
            ret = MPIX_Session_create_errhandler_x(F77_session_errhan_proxy, F77_errhan_free,
                                                   p, &errhandler_i);
            break;
        default:
            assert(0);
    }
    if (ret == MPI_SUCCESS) {
        *errhandler = MPI_Errhandler_c2f(errhandler_i);
    } else {
        MPL_free(p);
    }

    return ret;
}

/* For use by MPI_F08 */
int MPII_Comm_create_errhandler(F77_ErrFunction * err_fn, MPI_Fint * errhandler)
{
    return MPII_errhan_create(err_fn, errhandler, F77_COMM);
}

int MPII_File_create_errhandler(F77_ErrFunction * err_fn, MPI_Fint * errhandler)
{
    return MPII_errhan_create(err_fn, errhandler, F77_FILE);
}

int MPII_Win_create_errhandler(F77_ErrFunction * err_fn, MPI_Fint * errhandler)
{
    return MPII_errhan_create(err_fn, errhandler, F77_WIN);
}

int MPII_Session_create_errhandler(F77_ErrFunction * err_fn, MPI_Fint * errhandler)
{
    return MPII_errhan_create(err_fn, errhandler, F77_SESSION);
}

/* ---- generalized request ----------------- */
struct F77_greq_state {
    F77_greq_cancel_function *cancel_fn;
    F77_greq_free_function *free_fn;
    F77_greq_query_function *query_fn;
    void *extra_state;
};

static int F77_greq_cancel_proxy(void *extra_state, int complete)
{
    struct F77_greq_state *p = extra_state;

    MPI_Fint ierr;
    MPI_Fint complete_i = complete;
    p->cancel_fn(p->extra_state, &complete_i, &ierr);

    return ierr;
}

static int F77_greq_free_proxy(void *extra_state)
{
    struct F77_greq_state *p = extra_state;

    MPI_Fint ierr;
    p->free_fn(p->extra_state, &ierr);

    MPL_free(p);

    return ierr;
}

static int F77_greq_query_proxy(void *extra_state, MPI_Status * status)
{
    struct F77_greq_state *p = extra_state;

    MPI_Fint ierr;
    if (status == MPI_STATUS_IGNORE) {
        p->query_fn(p->extra_state, MPI_F_STATUS_IGNORE, &ierr);
    } else {
#ifdef HAVE_FINT_IS_INT
        MPI_Fint *status_p = (void *) status;
        p->query_fn(p->extra_state, status_p, &ierr);
#else
        MPI_Fint status_i[MPI_F_STATUS_SIZE];
        p->query_fn(p->extra_state, status_i, &ierr);

        int *t = (void *) status;
        for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
            t[i] = status_i[i];
        }
#endif
    }

    return ierr;
}

int MPII_greq_start(F77_greq_query_function query_fn, F77_greq_free_function free_fn,
                    F77_greq_cancel_function cancel_fn, void *extra_state, MPI_Fint * request)
{
    struct F77_greq_state *p = MPL_malloc(sizeof(struct F77_greq_state), MPL_MEM_OTHER);
    p->query_fn = query_fn;
    p->free_fn = free_fn;
    p->cancel_fn = cancel_fn;
    p->extra_state = extra_state;

    int mpi_errno;
    MPI_Request req_i;
    mpi_errno = MPI_Grequest_start(F77_greq_query_proxy, F77_greq_free_proxy, F77_greq_cancel_proxy,
                                   p, &req_i);
    if (mpi_errno == MPI_SUCCESS) {
        *request = MPI_Request_toint(req_i);
    }

    return mpi_errno;
}
