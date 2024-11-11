/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"
#include <assert.h>

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
    free(extra_state);
}

int MPII_op_create(F77_OpFunction * opfn, MPI_Fint commute, MPI_Fint * op)
{
    struct F77_op_state *p = malloc(sizeof(struct F77_op_state));
    p->opfn = opfn;

    MPI_Op op_i;
    int ret = MPIX_Op_create_x(F77_op_proxy, F77_op_free, commute, p, &op_i);
    if (ret == MPI_SUCCESS) {
        *op = MPI_Op_c2f(op_i);
    } else {
        free(p);
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
    free(extra_state);
}

int MPII_errhan_create(F77_ErrFunction * err_fn, MPI_Fint * errhandler, enum F77_handle_type type)
{
    struct F77_errhan_state *p = malloc(sizeof(struct F77_errhan_state));
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
        free(p);
    }

    return ret;
}

/* For use by MPI_F08 */
int MPII_Comm_create_errhandler(F77_ErrFunction * err_fn, MPI_Fint * errhandler)
{
    struct F77_errhan_state *p = malloc(sizeof(struct F77_errhan_state));
    p->err_fn = err_fn;

    MPI_Errhandler errhandler_i;
    int ret = MPI_SUCCESS;

    ret = MPIX_Comm_create_errhandler_x(F77_comm_errhan_proxy, F77_errhan_free, p, &errhandler_i);
    if (ret == MPI_SUCCESS) {
        *errhandler = MPI_Errhandler_c2f(errhandler_i);
    } else {
        free(p);
    }

    return ret;
}

int MPII_File_create_errhandler(F77_ErrFunction * err_fn, MPI_Fint * errhandler)
{
    struct F77_errhan_state *p = malloc(sizeof(struct F77_errhan_state));
    p->err_fn = err_fn;

    MPI_Errhandler errhandler_i;
    int ret = MPI_SUCCESS;

    ret = MPIX_File_create_errhandler_x(F77_file_errhan_proxy, F77_errhan_free, p, &errhandler_i);
    if (ret == MPI_SUCCESS) {
        *errhandler = MPI_Errhandler_c2f(errhandler_i);
    } else {
        free(p);
    }

    return ret;
}

int MPII_Win_create_errhandler(F77_ErrFunction * err_fn, MPI_Fint * errhandler)
{
    struct F77_errhan_state *p = malloc(sizeof(struct F77_errhan_state));
    p->err_fn = err_fn;

    MPI_Errhandler errhandler_i;
    int ret = MPI_SUCCESS;

    ret = MPIX_Win_create_errhandler_x(F77_win_errhan_proxy, F77_errhan_free, p, &errhandler_i);
    if (ret == MPI_SUCCESS) {
        *errhandler = MPI_Errhandler_c2f(errhandler_i);
    } else {
        free(p);
    }

    return ret;
}

int MPII_Session_create_errhandler(F77_ErrFunction * err_fn, MPI_Fint * errhandler)
{
    struct F77_errhan_state *p = malloc(sizeof(struct F77_errhan_state));
    p->err_fn = err_fn;

    MPI_Errhandler errhandler_i;
    int ret = MPI_SUCCESS;

    ret = MPIX_Session_create_errhandler_x(F77_session_errhan_proxy, F77_errhan_free,
                                           p, &errhandler_i);
    if (ret == MPI_SUCCESS) {
        *errhandler = MPI_Errhandler_c2f(errhandler_i);
    } else {
        free(p);
    }

    return ret;
}
