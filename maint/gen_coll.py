##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python import RE
from local_python.mpi_api import *
from local_python.binding_common import *

def main():
    binding_dir = G.get_srcdir_path("src/binding")
    c_dir = "src/binding/c"
    func_list = load_C_func_list(binding_dir, silent=True)
    G.algos = load_coll_algos("src/mpi/coll/coll_algorithms.txt")

    coll_names = ["barrier", "bcast", "gather", "gatherv", "scatter", "scatterv", "allgather", "allgatherv", "alltoall", "alltoallv", "alltoallw", "reduce", "allreduce", "reduce_scatter", "reduce_scatter_block", "scan", "exscan", "neighbor_allgather", "neighbor_allgatherv", "neighbor_alltoall", "neighbor_alltoallv", "neighbor_alltoallw"]

    G.out = []
    G.prototypes_hash = {}
    G.prototypes = []
    G.out.append("#include \"mpiimpl.h\"")
    G.out.append("#include \"iallgatherv/iallgatherv.h\"")
    for a in coll_names:
        dump_coll(a, "blocking")
        dump_coll(a, "nonblocking")
        dump_coll(a, "persistent")
    dump_c_file("src/mpi/coll/mpir_coll.c", G.out)
    dump_prototypes("src/mpi/coll/include/coll_algos.h", G.prototypes)

def add_prototype(l):
    if RE.match(r'int\s+(\w+)\(', l):
        func_name = RE.m.group(1)
        if func_name not in G.prototypes_hash:
            G.prototypes_hash[func_name] = 1
            G.prototypes.append(l)
        else:
            pass

def load_coll_algos(algo_txt):
    All = {}
    with open(algo_txt) as In:
        (func_commkind, algo_list, algo) = (None, None, None)
        for line in In:
            if RE.match(r'(\w+-(intra|inter)):', line):
                func_commkind = RE.m.group(1)
                algo_list = []
                All[func_commkind] = algo_list
            elif RE.match(r'\s+(\w+)\s*$', line):
                algo = {"name": RE.m.group(1), "func-commkind": func_commkind}
                algo_list.append(algo)
            elif RE.match(r'\s+(\w+):\s*(.+)', line):
                (key, value) = RE.m.group(1,2)
                algo[key] = value
    return All

def dump_coll(name, blocking_type):
    if blocking_type == "blocking":
        dump_allcomm_auto_blocking(name)
        dump_mpir_impl_blocking(name)
    elif blocking_type == "nonblocking":
        dump_allcomm_sched_auto(name)
        dump_sched_impl(name)
        dump_mpir_impl_nonblocking(name)
    elif blocking_type == "persistent":
        dump_mpir_impl_persistent(name)
    else:
        raise Exception("Wrong blocking_type")
    dump_mpir(name, blocking_type)

def dump_allcomm_auto_blocking(name):
    """ MPIR_Xxx_allcomm_auto - use Csel selections """
    blocking_type = "blocking"
    func = G.FUNCS["mpi_" + name]
    params, args = get_params_and_args(func)
    func_params = get_func_params(params, name, "blocking")
    func_args = get_func_args(args, name, "blocking")

    # e.g. ibcast, Ibcast, IBCAST
    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()
    NAME = func_name.upper()

    G.out.append("")
    G.out.append("/* ---- %s ---- */" % func_name)
    G.out.append("")
    add_prototype("int MPIR_%s_allcomm_auto(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s_allcomm_auto(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("")

    # -- Csel_search
    dump_open("MPIR_Csel_coll_sig_s coll_sig = {")
    G.out.append(".coll_type = MPIR_CSEL_COLL_TYPE__%s," % NAME)
    G.out.append(".comm_ptr = comm_ptr,")
    for p in func['parameters']:
        if not re.match(r'comm$', p['name']):
            G.out.append(".u.%s.%s = %s," % (func_name, p['name'], p['name']))
    dump_close("};")
    G.out.append("")
    G.out.append("MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);")
    G.out.append("MPIR_Assert(cnt);")
    G.out.append("")

    # -- switch
    def dump_cnt_algo_blocking(algo, commkind):
        if "allcomm" in algo:
            commkind = "allcomm"
        algo_name = get_algo_name(algo)
        algo_args = get_algo_args(args, algo, "csel")
        algo_params = get_algo_params(params, algo)
        add_prototype("int MPIR_%s_%s_%s(%s)" % (Name, commkind, algo_name, algo_params))
        dump_split(3, "mpi_errno = MPIR_%s_%s_%s(%s);" % (Name, commkind, algo_name, algo_args))

    dump_open("switch (cnt->id) {")
    for commkind in ("intra", "inter"):
        if commkind == "inter" and re.match(r'(scan|exscan|neighbor_)', name):
            continue
        for algo in G.algos[func_name + "-" + commkind]:
            if "allcomm" in algo:
                if commkind == "intra":
                    commkind = "allcomm"
                else:
                    # skip inter since it is covered already
                    continue
            G.out.append("case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_%s_%s_%s:" % (Name, commkind, algo['name']))
            G.out.append("INDENT")
            dump_cnt_algo_blocking(algo, commkind)
            G.out.append("break;");
            G.out.append("DEDENT")
            G.out.append("")
    G.out.append("case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_%s_allcomm_nb:" % Name)
    add_prototype("int MPIR_%s_allcomm_nb(%s);" % (Name, func_params))
    dump_split(2, "   mpi_errno = MPIR_%s_allcomm_nb(%s);" % (Name, func_args))
    G.out.append("   break;");
    G.out.append("")
    G.out.append("default:")
    G.out.append("    MPIR_Assert(0);")
    dump_close("}")

    # -- return
    G.out.append("MPIR_ERR_CHECK(mpi_errno);")
    dump_fn_exit()
    dump_close("}")

def dump_allcomm_sched_auto(name):
    """ MPIR_Xxx_allcomm_sched_auto - use Csel selections """
    blocking_type = "nonblocking"
    func = G.FUNCS["mpi_" + name]
    params, args = get_params_and_args(func)
    func_params = get_func_params(params, name, "allcomm_sched_auto")

    # e.g. ibcast, Ibcast, IBCAST
    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()
    NAME = func_name.upper()

    G.out.append("")
    G.out.append("/* ---- %s ---- */" % func_name)
    G.out.append("")
    add_prototype("int MPIR_%s_allcomm_sched_auto(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s_allcomm_sched_auto(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("")

    # -- Csel_search
    dump_open("MPIR_Csel_coll_sig_s coll_sig = {")
    G.out.append(".coll_type = MPIR_CSEL_COLL_TYPE__%s," % NAME)
    G.out.append(".comm_ptr = comm_ptr,")
    for p in func['parameters']:
        if not re.match(r'comm$', p['name']):
            G.out.append(".u.%s.%s = %s," % (func_name, p['name'], p['name']))
    dump_close("};")
    G.out.append("")
    G.out.append("MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);")
    G.out.append("MPIR_Assert(cnt);")
    G.out.append("")

    # -- add shced_auto prototypes
    sched_auto_params = get_func_params(params, name, "sched_auto")
    add_prototype("int MPIR_%s_intra_sched_auto(%s)" % (Name, sched_auto_params))
    if not re.match(r'(scan|exscan|neighbor_)', name):
        add_prototype("int MPIR_%s_inter_sched_auto(%s)" % (Name, sched_auto_params))

    # -- switch
    def dump_cnt_algo_tsp(algo, commkind):
        G.out.append("MPII_GENTRAN_CREATE_SCHED_P();")
        algo_name = get_algo_name(algo)
        algo_args = get_algo_args(args, algo, "csel")
        algo_params = get_algo_params(params, algo)
        add_prototype("int MPIR_TSP_%s_sched_%s_%s(%s)" % (Name, commkind, algo_name, algo_params))
        dump_split(3, "mpi_errno = MPIR_TSP_%s_sched_%s_%s(%s);" % (Name, commkind, algo_name, algo_args))

    def dump_cnt_algo_sched(algo, commkind):
        G.out.append("MPII_SCHED_CREATE_SCHED_P();")
        algo_name = get_algo_name(algo)
        algo_args = get_algo_args(args, algo, "csel")
        algo_params = get_algo_params(params, algo)
        add_prototype("int MPIR_%s_%s_%s(%s)" % (Name, commkind, algo_name, algo_params))
        dump_split(3, "mpi_errno = MPIR_%s_%s_%s(%s);" % (Name, commkind, algo_name, algo_args))

    dump_open("switch (cnt->id) {")
    for commkind in ("intra", "inter"):
        if commkind == "inter" and re.match(r'(scan|exscan|neighbor_)', name):
            continue
        for algo in G.algos[func_name + "-" + commkind]:
            use_commkind = commkind
            if "allcomm" in algo:
                if commkind == "intra":
                    use_commkind = "allcomm"
                else:
                    # skip inter since it is covered already
                    continue
            G.out.append("case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_%s_%s_%s:" % (Name, use_commkind, algo['name']))
            G.out.append("INDENT")
            if algo['name'].startswith('tsp_'):
                dump_cnt_algo_tsp(algo, use_commkind)
            else:
                dump_cnt_algo_sched(algo, use_commkind)
            G.out.append("break;");
            G.out.append("DEDENT")
            G.out.append("")
    G.out.append("default:")
    G.out.append("    MPIR_Assert(0);")
    dump_close("}")

    # -- return
    G.out.append("MPIR_ERR_CHECK(mpi_errno);")
    dump_fn_exit()
    dump_close("}")

def dump_mpir_impl_blocking(name):
    """ MPIR_Xxx_impl - """
    blocking_type = "blocking"
    func = G.FUNCS["mpi_" + name]
    params, args = get_params_and_args(func)
    func_params = get_func_params(params, name, "blocking")
    func_args = get_func_args(args, name, "blocking")

    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()
    NAME = func_name.upper()

    need_fallback = False

    def dump_algo(algo, commkind):
        if "allcomm" in algo:
            commkind = "allcomm"
        algo_name = get_algo_name(algo)
        algo_args = get_algo_args(args, algo, "cvar")
        dump_split(3, "mpi_errno = MPIR_%s_%s_%s(%s);" % (Name, commkind, algo_name, algo_args))

    def dump_cases(commkind):
        nonlocal need_fallback
        CVAR_PREFIX = "MPIR_CVAR_%s_%s_ALGORITHM" % (NAME, commkind.upper())
        for algo in G.algos[func_name + '-' + commkind]:
            if algo['name'] != "auto" and algo['name'] != "nb":
                G.out.append("case %s_%s:" % (CVAR_PREFIX, algo['name']))
                G.out.append("INDENT")
                if 'restrictions' in algo:
                    dump_fallback(algo)
                    need_fallback = True
                dump_algo(algo, commkind)
                G.out.append("break;");
                G.out.append("DEDENT")
        G.out.append("case %s_nb:" % CVAR_PREFIX)
        dump_split(3, "    mpi_errno = MPIR_%s_allcomm_nb(%s);" % (Name, func_args))
        G.out.append("     break;");
        G.out.append("case %s_auto:" % CVAR_PREFIX)
        dump_split(3, "    mpi_errno = MPIR_%s_allcomm_auto(%s);" % (Name, func_args))
        G.out.append("    break;");
        G.out.append("default:")
        G.out.append("    MPIR_Assert(0);")

    # ----------------            
    G.out.append("")
    add_prototype("int MPIR_%s_impl(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s_impl(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("")

    dump_open("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {")
    dump_open("switch (MPIR_CVAR_%s_INTRA_ALGORITHM) {" % NAME)
    dump_cases("intra")
    dump_close("}")
    dump_else()
    if re.match(r'(scan|exscan|neighbor_)', name):
        G.out.append("MPIR_Assert_error(\"Only intra-communicator allowed\");")
    else:
        dump_open("switch (MPIR_CVAR_%s_INTER_ALGORITHM) {" % NAME)
        dump_cases("inter")
        dump_close("}")
    dump_close("}")

    G.out.append("MPIR_ERR_CHECK(mpi_errno);")
    if need_fallback:
        G.out.append("goto fn_exit;")
        G.out.append("")
        G.out.append("fallback:")
        dump_split(1, "mpi_errno = MPIR_%s_allcomm_auto(%s);" % (Name, func_args))
    G.out.append("")
    dump_fn_exit()
    dump_close("}")

def dump_sched_impl(name):
    """ MPIR_Xxx_impl - """
    blocking_type = "nonblocking"
    func = G.FUNCS["mpi_" + name]
    params, args = get_params_and_args(func)
    func_params = get_func_params(params, name, "sched_impl")

    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()
    NAME = func_name.upper()

    need_fallback = False

    def dump_algo_tsp(algo, commkind):
        G.out.append("MPII_GENTRAN_CREATE_SCHED_P();")
        algo_name = get_algo_name(algo)
        algo_args = get_algo_args(args, algo, "cvar")
        if "allcomm" in algo:
            commkind = "allcomm"
        dump_split(3, "mpi_errno = MPIR_TSP_%s_sched_%s_%s(%s);" % (Name, commkind, algo_name, algo_args))

    def dump_algo_sched(algo, commkind):
        G.out.append("MPII_SCHED_CREATE_SCHED_P();")
        algo_name = get_algo_name(algo)
        algo_args = get_algo_args(args, algo, "cvar")
        if "allcomm" in algo:
            commkind = "allcomm"
        dump_split(3, "mpi_errno = MPIR_%s_%s_%s(%s);" % (Name, commkind, algo_name, algo_args))

    def dump_cases(commkind):
        nonlocal need_fallback
        CVAR_PREFIX = "MPIR_CVAR_%s_%s_ALGORITHM" % (NAME, commkind.upper())
        for algo in G.algos[func_name + '-' + commkind]:
            if algo['name'] != "auto" and algo['name'] != "nb":
                G.out.append("case %s_%s:" % (CVAR_PREFIX, algo['name']))
                G.out.append("INDENT")
                if 'restrictions' in algo:
                    dump_fallback(algo)
                    need_fallback = True
                if algo['name'].startswith('tsp_'):
                    dump_algo_tsp(algo, commkind)
                else:
                    dump_algo_sched(algo, commkind)
                G.out.append("break;");
                G.out.append("DEDENT")
        G.out.append("case %s_auto:" % CVAR_PREFIX)
        func_args = get_func_args(args, name, "allcomm_sched_auto")
        dump_split(3, "    mpi_errno = MPIR_%s_allcomm_sched_auto(%s);" % (Name, func_args))
        G.out.append("    break;");
        G.out.append("default:")
        G.out.append("    MPIR_Assert(0);")

    # ----------------            
    G.out.append("")
    add_prototype("int MPIR_%s_sched_impl(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s_sched_impl(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("")

    dump_open("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {")
    dump_open("switch (MPIR_CVAR_%s_INTRA_ALGORITHM) {" % NAME)
    dump_cases("intra")
    dump_close("}")
    dump_else()
    if re.match(r'(scan|exscan|neighbor_)', name):
        G.out.append("MPIR_Assert_error(\"Only intra-communicator allowed\");")
    else:
        dump_open("switch (MPIR_CVAR_%s_INTER_ALGORITHM) {" % NAME)
        dump_cases("inter")
        dump_close("}")
    dump_close("}")

    G.out.append("MPIR_ERR_CHECK(mpi_errno);")
    if need_fallback:
        G.out.append("goto fn_exit;")
        G.out.append("")
        G.out.append("fallback:")
        func_args = get_func_args(args, name, "allcomm_sched_auto")
        dump_split(1, "mpi_errno = MPIR_%s_allcomm_sched_auto(%s);" % (Name, func_args))
    G.out.append("")
    dump_fn_exit()
    dump_close("}")

def dump_mpir_impl_nonblocking(name):
    blocking_type = "nonblocking"
    func = G.FUNCS["mpi_" + name]
    params, args = get_params_and_args(func)
    func_params = get_func_params(params, name, "nonblocking")

    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()
    NAME = func_name.upper()

    G.out.append("")
    add_prototype("int MPIR_%s_impl(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s_impl(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("enum MPIR_sched_type sched_type;")
    G.out.append("void *sched;")
    G.out.append("")
    G.out.append("*request = NULL;")
    func_args = get_func_args(args, name, "mpir_impl_nonblocking")
    dump_split(1, "mpi_errno = MPIR_%s_sched_impl(%s);" % (Name, func_args))
    G.out.append("MPIR_ERR_CHECK(mpi_errno);")
    G.out.append("MPII_SCHED_START(sched_type, sched, comm_ptr, request);")
    G.out.append("")
    G.out.append("fn_exit:")
    G.out.append("return mpi_errno;")
    G.out.append("fn_fail:")
    G.out.append("goto fn_exit;")
    dump_close("}")

def dump_mpir_impl_persistent(name):
    blocking_type = "persistent"
    func = G.FUNCS["mpi_" + name]
    params, args = get_params_and_args(func)
    func_params = get_func_params(params, name, "persistent")

    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()
    NAME = func_name.upper()

    G.out.append("")
    add_prototype("int MPIR_%s_impl(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s_impl(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("")
    G.out.append("MPIR_Request *req = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_COLL);")
    G.out.append("MPIR_ERR_CHKANDJUMP(!req, mpi_errno, MPI_ERR_OTHER, \"**nomem\");")
    G.out.append("MPIR_Comm_add_ref(comm_ptr);")
    G.out.append("req->comm = comm_ptr;")
    G.out.append("MPIR_Comm_save_inactive_request(comm_ptr, req);")
    G.out.append("req->u.persist_coll.sched_type = MPIR_SCHED_INVALID;")
    G.out.append("req->u.persist_coll.real_request = NULL;")

    func_args = get_func_args(args, name, "mpir_impl_persistent")
    dump_split(1, "mpi_errno = MPIR_I%s_sched_impl(%s);" % (name, func_args))
    G.out.append("MPIR_ERR_CHECK(mpi_errno);")
    G.out.append("")
    G.out.append("*request = req;")
    G.out.append("")
    G.out.append("fn_exit:")
    G.out.append("return mpi_errno;")
    G.out.append("fn_fail:")
    G.out.append("goto fn_exit;")
    dump_close("}")

def dump_mpir(name, blocking_type):
    """ MPIR_Xxx - """
    func = G.FUNCS["mpi_" + name]
    params, args = get_params_and_args(func)
    func_params = get_func_params(params, name, blocking_type)
    func_args = get_func_args(args, name, blocking_type)

    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()
    NAME = func_name.upper()

    def dump_buffer_swap_pre():
        G.out.append("void *in_recvbuf = recvbuf;")
        G.out.append("void *host_sendbuf = NULL;")
        G.out.append("void *host_recvbuf = NULL;")
        G.out.append("")
        if name == "reduce_scatter":
            G.out.append("MPI_Aint count = 0;")
            G.out.append("for (int i = 0; i < MPIR_Comm_size(comm_ptr); i++) {")
            G.out.append("    count += recvcounts[i];")
            G.out.append("}")
            G.out.append("")
        elif name == "reduce_scatter_block":
            G.out.append("MPI_Aint count = MPIR_Comm_size(comm_ptr) * recvcount;")

        if name == "reduce":
            use_recvbuf = "(comm_ptr->rank == root || root == MPI_ROOT) ? recvbuf : NULL"
        else:
            use_recvbuf = "recvbuf"

        G.out.append("if(!MPIR_Typerep_reduce_is_supported(op, datatype))") 
        G.out.append("  MPIR_Coll_host_buffer_alloc(sendbuf, %s, count, datatype, &host_sendbuf, &host_recvbuf);" % use_recvbuf)
        G.out.append("")

        for buf in ("sendbuf", "recvbuf"):
            G.out.append("if (host_%s) {" % buf);
            G.out.append("    %s = host_%s;" % (buf, buf));
            G.out.append("}")
        G.out.append("")

    def dump_buffer_swap_post():
        count = "count"
        if name == "reduce_scatter":
            count = "recvcounts[comm_ptr->rank]"
        elif name == "reduce_scatter_block":
            count = "recvcount"

        if blocking_type == "blocking":
            G.out.append("if (host_recvbuf) {")
            G.out.append("    recvbuf = in_recvbuf;")
            G.out.append("    MPIR_Localcopy(host_recvbuf, count, datatype, recvbuf, count, datatype);")
            G.out.append("}")
            G.out.append("MPIR_Coll_host_buffer_free(host_sendbuf, host_recvbuf);")
        elif blocking_type == "nonblocking":
            G.out.append("MPIR_Coll_host_buffer_swap_back(host_sendbuf, host_recvbuf, in_recvbuf, %s, datatype, *request);" % count)
        elif blocking_type == "persistent":
            G.out.append("MPIR_Coll_host_buffer_persist_set(host_sendbuf, host_recvbuf, in_recvbuf, %s, datatype, *request);" % count)

    G.out.append("")
    add_prototype("int MPIR_%s(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("")

    need_buffer_swap = False
    if re.match(r'(reduce|allreduce|scan|exscan|reduce_scatter)', name):
        need_buffer_swap = True
    if need_buffer_swap:
        dump_buffer_swap_pre()

    cond1 = "MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all"
    cond2 = "MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll"
    cond3 = "MPIR_CVAR_%s_DEVICE_COLLECTIVE" % NAME
    G.out.append("if ((%s) ||" % cond1)
    G.out.append("    ((%s) &&" % cond2)
    G.out.append("     %s)) {" % cond3)
    G.out.append("INDENT")
    dump_split(2, "mpi_errno = MPID_%s(%s);" % (Name, func_args))
    dump_else()
    dump_split(2, "mpi_errno = MPIR_%s_impl(%s);" % (Name, func_args))
    dump_close("}")
    if need_buffer_swap:
        dump_buffer_swap_post()
    G.out.append("")
    G.out.append("return mpi_errno;")
    dump_close("}")

# ----
def dump_fallback(algo):
    cond_list = []
    for a in algo['restrictions'].replace(" ","").split(','):
        if a == "inplace":
            cond_list.append("sendbuf == MPI_IN_PLACE")
        elif a == "noinplace":
            cond_list.append("sendbuf != MPI_IN_PLACE")
        elif a == "power-of-two":
            cond_list.append("comm_ptr->local_size == comm_ptr->coll.pof2")
        elif a == "size-ge-pof2":
            cond_list.append("count >= comm_ptr->coll.pof2")
        elif a == "commutative":
            cond_list.append("MPIR_Op_is_commutative(op)")
        elif a== "builtin-op":
            cond_list.append("HANDLE_IS_BUILTIN(op)")
        elif a == "parent-comm":
            cond_list.append("MPIR_Comm_is_parent_comm(comm_ptr)")
        elif a == "node-consecutive":
            cond_list.append("MPII_Comm_is_node_consecutive(comm_ptr)")
        elif a == "displs-ordered":
            # assume it's allgatherv
            cond_list.append("MPII_Iallgatherv_is_displs_ordered(comm_ptr->local_size, recvcounts, displs)")
        else:
            raise Exception("Unsupported restrictions - %s" % a)
    (func_name, commkind) = algo['func-commkind'].split('-')
    G.out.append("MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, %s, mpi_errno," % ' && '.join(cond_list))
    G.out.append("                               \"%s %s cannot be applied.\\n\");" % (func_name.capitalize(), algo['name']))

# ----
def get_func_name(name, blocking_type):
    if blocking_type == "blocking":
        return name
    elif blocking_type == "nonblocking":
        return 'i' + name
    elif blocking_type == "persistent":
        return name + "_init"

def get_params_and_args(func):
    mapping = G.MAPS['SMALL_C_KIND_MAP']

    params = []
    args = []
    for p in func['parameters']:
        if p['name'] == 'comm':
            params.append("MPIR_Comm * comm_ptr")
            args.append("comm_ptr")
        else:
            s = get_C_param(p, func, mapping)
            if p['kind'].startswith('POLY'):
                s = re.sub(r'\bint ', 'MPI_Aint ', s)
            params.append(s)
            args.append(p['name'])

    return (', '.join(params), ', '.join(args))

def get_algo_extra_args(algo, kind):
    (func_name, commkind) = algo['func-commkind'].split('-')
    extra_params = algo['extra_params'].replace(' ', '').split(',')
    cvar_params = algo['cvar_params'].replace(' ', '').split(',')
    if len(extra_params) != len(cvar_params):
        raise Exception("algorithm %s-%s-%s: extra_params and cvar_params sizes mismatch!" % (func_name, commkind, algo['name']))

    out_list = []
    for i in range(len(extra_params)):
        if RE.match(r'\w+=(.+)', extra_params[i]):
            # constant parameter
            out_list.append(RE.m.group(1))
        else:
            if kind == "csel":
                prefix = "cnt->u.%s.%s_%s." % (func_name, commkind, algo['name'])
                out_list.append(prefix + extra_params[i])
            elif kind == "cvar":
                prefix = "MPIR_CVAR_%s_" % func_name.upper() 
                tmp = prefix + cvar_params[i]
                if re.match(r"%sTREE_TYPE" % prefix, tmp):
                    newname = "MPIR_%s_tree_type" % func_name.capitalize()
                    tmp = re.sub(r"%sTREE_TYPE" % prefix, newname, tmp)
                elif re.match(r"%sTHROTTLE" % prefix, tmp):
                    newname = "MPIR_CVAR_ALLTOALL_THROTTLE"
                    tmp = re.sub(r"%sTHROTTLE" % prefix, newname, tmp)
                out_list.append(tmp)
            else:
                raise Exception("Wrong kind!")

    return ', '.join(out_list)

def get_algo_extra_params(algo):
    extra_params = algo['extra_params'].replace(' ', '').split(',')
    out_list = []
    for a in extra_params:
        if RE.match(r'(\w+)=.+', a):
            # constant parameter
            out_list.append("int " + RE.m.group(1))
        else:
            out_list.append("int " + a)
    return ', '.join(out_list)

# additional wrappers
def get_algo_args(args, algo, kind):
    algo_args = args
    if 'extra_params' in algo:
        algo_args += ", " + get_algo_extra_args(algo, kind)

    if algo['name'].startswith('tsp_'):
        algo_args += ", *sched_p"
    elif algo['func-commkind'].startswith('i'):
        algo_args += ", *sched_p"
    elif not algo['func-commkind'].startswith('neighbor_'):
        algo_args += ", errflag"

    return algo_args

def get_algo_params(params, algo):
    algo_params = params
    if 'extra_params' in algo:
        algo_params += ", " + get_algo_extra_params(algo)

    if algo['name'].startswith('tsp_'):
        algo_params += ", MPIR_TSP_sched_t sched"
    elif algo['func-commkind'].startswith('i'):
        algo_params += ", MPIR_Sched_t s"
    elif not algo['func-commkind'].startswith('neighbor_'):
        algo_params += ", MPIR_Errflag_t errflag"

    return algo_params

def get_algo_name(algo):
    # the name used in algo function name
    if "func_name" in algo:
        return algo['func_name']
    elif algo['name'].startswith('tsp_'):
        return algo['name'][4:]
    else:
        return algo['name']

def get_func_params(params, name, kind):
    func_params = params
    if kind == "blocking":
        if not name.startswith('neighbor_'):
            func_params += ", MPIR_Errflag_t errflag"
    elif kind == "nonblocking":
        func_params += ", MPIR_Request ** request"
    elif kind == "persistent":
        func_params += ", MPIR_Info * info_ptr, MPIR_Request ** request"
    elif kind == "sched_auto":
        func_params += ", MPIR_Sched_t s"
    elif kind == "allcomm_sched_auto":
        func_params += ", bool is_persistent, void **sched_p, enum MPIR_sched_type *sched_type_p"
    elif kind == "sched_impl":
        func_params += ", bool is_persistent, void **sched_p, enum MPIR_sched_type *sched_type_p"
    else:
        raise Exception("get_func_params - unexpected kind = %s" % kind)

    return func_params

def get_func_args(args, name, kind):
    func_args = args
    if kind == "blocking":
        if not name.startswith('neighbor_'):
            func_args += ", errflag"
    elif kind == "nonblocking":
        func_args += ", request"
    elif kind == "persistent":
        func_args += ", info_ptr, request"
    elif kind == "allcomm_sched_auto":
        func_args += ", is_persistent, sched_p, sched_type_p"
    elif kind == "mpir_impl_nonblocking":
        func_args += ", false, &sched, &sched_type"
    elif kind == "mpir_impl_persistent":
        func_args += ", true, &req->u.persist_coll.sched, &req->u.persist_coll.sched_type"
    else:
        raise Exception("get_func_args - unexpected kind = %s" % kind)

    return func_args

# ----------------------
def dump_c_file(f, lines):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        indent = 0
        for l in G.copyright_c:
            print(l, file=Out)
        for l in lines:
            if RE.match(r'(INDENT|DEDENT)', l):
                # indentations
                a = RE.m.group(1)
                if a == "INDENT":
                    indent += 1
                else:
                    indent -= 1
            elif RE.match(r'\s*(fn_exit|fn_fail|fallback):', l):
                # labels
                print("  %s:" % RE.m.group(1), file=Out)
            else:
                # print the line with correct indentations
                if indent > 0 and not RE.match(r'#(if|endif)', l):
                    print("    " * indent, end='', file=Out)
                print(l, file=Out)

def dump_prototypes(f, prototypes):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)
        print("#ifndef COLL_ALGOS_H_INCLUDED", file=Out)
        print("#define COLL_ALGOS_H_INCLUDED", file=Out)
        print("", file=Out)
        for l in prototypes:
            lines = split_line_with_break(l + ';', '', 80)
            for l2 in lines:
                print(l2, file=Out)
        print("#endif /* COLL_ALGOS_H_INCLUDED */", file=Out)

def dump_open(line):
    G.out.append(line)
    G.out.append("INDENT")

def dump_close(line):
    G.out.append("DEDENT")
    G.out.append(line)

def dump_else():
    G.out.append("DEDENT")
    G.out.append("} else {")
    G.out.append("INDENT")

def dump_fn_exit():
    G.out.append("")
    G.out.append("fn_exit:")
    G.out.append("return mpi_errno;")
    G.out.append("fn_fail:")
    G.out.append("goto fn_exit;")

def dump_split(indent, l):
    tlist = split_line_with_break(l, "", 100 - indent * 4)
    G.out.extend(tlist)

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
