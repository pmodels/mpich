/* This file is to be included in initcxx.cxx */

/* ---- user op ----------------- */
struct CXX_op_state {
    User_function *opfn;
};

static void CXX_op_proxy(void *invec, void *inoutvec, MPI_Count len, MPI_Datatype datatype,
                         void *extra_state)
{
    CXX_op_state *p = (CXX_op_state *) extra_state;
    MPI::Datatype cxxdtype = datatype;

    p->opfn(invec, inoutvec, len, cxxdtype);
}

static void CXX_op_free(void *extra_state)
{
    delete((CXX_op_state *) extra_state);
}

void Op::Init(User_function *opfn, bool commute)
{
    CXX_op_state *p = new(CXX_op_state);
    p->opfn = opfn;

    MPIX_CALLWORLD( MPIX_Op_create_x(CXX_op_proxy, CXX_op_free, (int) commute, p, &the_real_op) );
}

/* ---- user errhandler  ----------------- */
struct CXX_errhan_state {
    union {
        MPI::Comm::Errhandler_function * comm_err_fn;
        MPI::Win::Errhandler_function * win_err_fn;
        MPI::File::Errhandler_function * file_err_fn;
        // MPI::Session::Errhandler_function * session_err_fn;
    } u;
};

static void CXX_comm_errhan_proxy(MPI_Comm comm, int error_code, void *extra_state)
{
    CXX_errhan_state *p = (CXX_errhan_state *) extra_state;
    int is_intercomm;
    MPI_Comm_test_inter(comm, &is_intercomm);
    if (is_intercomm) {
        MPI::Intercomm ic(comm);
        p->u.comm_err_fn(ic, &error_code);
    } else {
        MPI::Intracomm ic(comm);
        p->u.comm_err_fn(ic, &error_code);
    }
}

static void CXX_win_errhan_proxy(MPI_Win win, int error_code, void *extra_state)
{
    CXX_errhan_state *p = (CXX_errhan_state *) extra_state;
    MPI::Win h = win;
    p->u.win_err_fn(h, &error_code);
}

static void CXX_file_errhan_proxy(MPI_File file, int error_code, void *extra_state)
{
    CXX_errhan_state *p = (CXX_errhan_state *) extra_state;
    MPI::File h = file;
    p->u.file_err_fn(h, &error_code);
}

static void CXX_errhan_free(void *extra_state)
{
    delete((CXX_errhan_state *) extra_state);
}

Errhandler Comm::Create_errhandler(MPI::Comm::Errhandler_function *f)
{
    CXX_errhan_state *p = new(CXX_errhan_state);
    p->u.comm_err_fn = f;

    MPI_Errhandler eh;
    MPIX_Comm_create_errhandler_x(CXX_comm_errhan_proxy, CXX_errhan_free, p, &eh);

    MPI::Errhandler e1;
    e1.the_real_errhandler = eh;
    return e1;
}

Errhandler Win::Create_errhandler(MPI::Win::Errhandler_function *f)
{
    CXX_errhan_state *p = new(CXX_errhan_state);
    p->u.win_err_fn = f;

    MPI_Errhandler eh;
    MPIX_Win_create_errhandler_x(CXX_win_errhan_proxy, CXX_errhan_free, p, &eh);

    MPI::Errhandler e1;
    e1.the_real_errhandler = eh;
    return e1;
}

Errhandler File::Create_errhandler(MPI::File::Errhandler_function *f)
{
    CXX_errhan_state *p = new(CXX_errhan_state);
    p->u.file_err_fn = f;

    MPI_Errhandler eh;
    MPIX_File_create_errhandler_x(CXX_file_errhan_proxy, CXX_errhan_free, p, &eh);

    MPI::Errhandler e1;
    e1.the_real_errhandler = eh;
    return e1;
}
