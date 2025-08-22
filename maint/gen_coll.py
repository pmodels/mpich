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

    # dump impl functions
    for a in coll_names:
        dump_coll_impl(a, "blocking")
        dump_coll_impl(a, "nonblocking")
        dump_coll_impl(a, "persistent")
    # dump the container version of the algorithms
    dump_algo_cnt_fns()
    add_algo_prototypes()
    for a in coll_names:
        add_sched_auto_prototypes(a)

    dump_c_file("src/mpi/coll/mpir_coll.c", G.out)
    dump_prototypes("src/mpi/coll/include/coll_algos.h", G.prototypes)

def dump_algo_cnt_fns():
    def get_coll_args(func, func_name):
        args = []
        for p in func['parameters']:
            if p['name'] == 'comm':
                args.append("coll_sig->comm_ptr")
            else:
                args.append("coll_sig->u.%s.%s" % (func_name, p['name']))
        return ', '.join(args)

    def get_algo_args(func, func_name, algo):
        args = get_coll_args(func, func_name)
        if 'extra_params' in algo:
            args += ", " + get_algo_extra_args(algo, "csel")

        if func_name.startswith('i'):
            args += ", coll_sig->sched"
        elif func_name.startswith('neighbor_'):
            pass
        else:
            args += ", 0" # coll_attr

        return args

    def dump_algo_prep(func_name, algo):
        if func_name.startswith('i'):
            if algo['name'].startswith('tsp_'):
                G.out.append("MPII_CSEL_CREATE_TSP_SCHED(coll_sig);")
            else:
                G.out.append("MPII_CSEL_CREATE_SCHED(coll_sig);")

    algo_funcname_hash = {}
    for func_commkind in sorted(G.algos):
        func_name, commkind = func_commkind.split("-")
        if func_name.startswith('i'):
            # use blocking func for base parameters
            func = G.FUNCS["mpi_" + func_name[1:]]
        else:
            func = G.FUNCS["mpi_" + func_name]
        for algo in G.algos[func_commkind]:
            if "allcomm" in algo and commkind == "inter":
                continue
            algo_funcname = get_algo_funcname(func_name, commkind, algo)
            if algo_funcname in algo_funcname_hash:
                # skip alias algorithms
                continue
            else:
                algo_funcname_hash[algo_funcname] = 1
            algo_args = get_algo_args(func, func_name, algo)
            decl = "int %s_cnt(%s)" % (algo_funcname, "MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt")
            add_prototype(decl)
            dump_split(0, decl)
            dump_open('{')
            G.out.append("int mpi_errno = MPI_SUCCESS;")
            G.out.append("")
            dump_algo_prep(func_name, algo)
            dump_split(1, "mpi_errno = %s(%s);" % (algo_funcname, algo_args))
            G.out.append("MPIR_ERR_CHECK(mpi_errno);")
            G.out.append("")
            G.out.append("fn_exit:")
            G.out.append("return mpi_errno;")
            G.out.append("fn_fail:")
            G.out.append("goto fn_exit;")
            dump_close('}')
            G.out.append("")

def add_algo_prototypes():
    def get_coll_params(func):
        mapping = G.MAPS['SMALL_C_KIND_MAP']
        params = []
        for p in func['parameters']:
            if p['name'] == 'comm':
                params.append("MPIR_Comm * comm_ptr")
            else:
                s = get_C_param(p, func, mapping)
                if p['kind'].startswith('POLY'):
                    s = re.sub(r'\bint ', 'MPI_Aint ', s)
                params.append(s)
        return ', '.join(params)

    def get_algo_params(func, func_name, algo):
        params = get_coll_params(func)
        if 'extra_params' in algo:
            params += ", " + get_algo_extra_params(algo)

        if func_name.startswith('i'):
            if algo['name'].startswith('tsp_'):
                params += ", MPIR_TSP_sched_t s"
            else:
                params += ", MPIR_Sched_t s"
        elif func_name.startswith('neighbor_'):
            pass
        else:
            params += ", int coll_attr" # coll_attr

        return params

    for func_commkind in sorted(G.algos):
        func_name, commkind = func_commkind.split("-")
        if func_name.startswith('i'):
            # use blocking func for base parameters
            func = G.FUNCS["mpi_" + func_name[1:]]
        else:
            func = G.FUNCS["mpi_" + func_name]

        algo_funcname_hash = {}
        for algo in G.algos[func_commkind]:
            if "allcomm" in algo and commkind == "inter":
                continue
            algo_funcname = get_algo_funcname(func_name, commkind, algo)
            if algo_funcname in algo_funcname_hash:
                # skip alias algorithms
                continue
            else:
                algo_funcname_hash[algo_funcname] = 1
            algo_params = get_algo_params(func, func_name, algo)
            decl = "int %s(%s)" % (algo_funcname, algo_params)
            add_prototype(decl)

def add_sched_auto_prototypes(name):
    def get_coll_params(func):
        mapping = G.MAPS['SMALL_C_KIND_MAP']
        params = []
        for p in func['parameters']:
            if p['name'] == 'comm':
                params.append("MPIR_Comm * comm_ptr")
            else:
                s = get_C_param(p, func, mapping)
                if p['kind'].startswith('POLY'):
                    s = re.sub(r'\bint ', 'MPI_Aint ', s)
                params.append(s)
        return ', '.join(params)

    func = G.FUNCS["mpi_" + name]
    params = get_coll_params(func)
    params += ", MPIR_Sched_t s"
    add_prototype("int MPIR_I%s_intra_sched_auto(%s)" % (name, params))
    if not re.match(r'(scan|exscan|neighbor_)', name):
        add_prototype("int MPIR_I%s_inter_sched_auto(%s)" % (name, params))

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

def dump_coll_impl(name, blocking_type):
    func = G.FUNCS["mpi_" + name]
    func_params = get_func_params(func, name, blocking_type)

    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()

    G.out.append("")
    add_prototype("int MPIR_%s_impl(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s_impl(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("")

    # Initialize coll_sig
    G.out.append("MPIR_Csel_coll_sig_s coll_sig;")
    if blocking_type == "blocking":
        G.out.append("coll_sig.coll_type = MPIR_CSEL_COLL_TYPE__%s;" % name.upper())
    else:
        # nonblocking and persistent
        G.out.append("coll_sig.coll_type = MPIR_CSEL_COLL_TYPE__I%s;" % name.upper())
    G.out.append("coll_sig.comm_ptr = comm_ptr;")
    if blocking_type == "persistent":
        G.out.append("coll_sig.is_persistent = true;")
    else:
        G.out.append("coll_sig.is_persistent = false;")
    G.out.append("coll_sig.sched = NULL;")

    for p in func['parameters']:
        if p['name'] == 'comm':
            pass
        else:
            G.out.append("coll_sig.u.%s.%s = %s;" % (name, p['name'], p['name']))

    # Call csel
    G.out.append("")
    G.out.append("mpi_errno = MPIR_Coll_composition_auto(&coll_sig);")
    G.out.append("MPIR_ERR_CHECK(mpi_errno);")
    G.out.append("")

    # Set request if nonblocking or persistent
    if blocking_type == "blocking":
        pass
    elif blocking_type == "nonblocking":
        G.out.append("MPII_SCHED_START(coll_sig.sched_type, coll_sig.sched, comm_ptr, request);")
        G.out.append("")
    elif blocking_type == "persistent":
        G.out.append("MPIR_Request *req = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_COLL);")
        G.out.append("MPIR_ERR_CHKANDJUMP(!req, mpi_errno, MPI_ERR_OTHER, \"**nomem\");")
        G.out.append("MPIR_Comm_add_ref(comm_ptr);")
        G.out.append("MPIR_Comm_save_inactive_request(comm_ptr, req);")
        G.out.append("req->u.persist_coll.sched_type = coll_sig.sched_type;")
        G.out.append("req->u.persist_coll.sched = coll_sig.sched;")
        G.out.append("*request = req;")
        G.out.append("")
    else:
        raise Exception("Wrong blocking_type")

    G.out.append("fn_exit:")
    G.out.append("return mpi_errno;")
    G.out.append("fn_fail:")
    G.out.append("goto fn_exit;")
    dump_close('}')

# ----
def get_func_name(name, blocking_type):
    if blocking_type == "blocking":
        return name
    elif blocking_type == "nonblocking":
        return 'i' + name
    elif blocking_type == "persistent":
        return name + "_init"

def get_algo_funcname(func_name, commkind, algo):
    if 'allcomm' in algo:
        commkind = 'allcomm'
    Name = func_name.capitalize()
    if func_name.startswith('i'):
        if algo['name'].startswith('tsp_'):
            return "MPIR_TSP_%s_sched_%s_%s" % (Name, commkind, get_algo_name(algo))
        else:
            return "MPIR_%s_%s_%s" % (Name, commkind, get_algo_name(algo))
    else:
        return "MPIR_%s_%s_%s" % (Name, commkind, get_algo_name(algo))

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
def get_algo_name(algo):
    # the name used in algo function name
    if "func_name" in algo:
        return algo['func_name']
    elif algo['name'].startswith('tsp_'):
        return algo['name'][4:]
    else:
        return algo['name']

def get_func_params(func, name, blocking_type):
    mapping = G.MAPS['SMALL_C_KIND_MAP']

    params = []
    for p in func['parameters']:
        if p['name'] == 'comm':
            params.append("MPIR_Comm * comm_ptr")
        else:
            s = get_C_param(p, func, mapping)
            if p['kind'].startswith('POLY'):
                s = re.sub(r'\bint ', 'MPI_Aint ', s)
            params.append(s)

    if blocking_type == "blocking":
        pass
    elif blocking_type == "nonblocking":
        params.append("MPIR_Request ** request")
    elif blocking_type == "persistent":
        params.append("MPIR_Info * info_ptr")
        params.append("MPIR_Request ** request")
    else:
        raise Exception("get_func_params - unexpected blocking_type = %s" % blocking_type)

    return ', '.join(params)

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
