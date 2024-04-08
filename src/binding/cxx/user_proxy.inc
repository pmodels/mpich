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
