##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python import RE
from local_python.mpi_api import *
from local_python.binding_common import *

def main():
    G.coll_names = ["barrier", "bcast", "gather", "gatherv", "scatter", "scatterv", "allgather", "allgatherv", "alltoall", "alltoallv", "alltoallw", "reduce", "allreduce", "reduce_scatter", "reduce_scatter_block", "scan", "exscan", "neighbor_allgather", "neighbor_allgatherv", "neighbor_alltoall", "neighbor_alltoallv", "neighbor_alltoallw"]

    binding_dir = G.get_srcdir_path("src/binding")
    c_dir = "src/binding/c"
    func_list = load_C_func_list(binding_dir, silent=True)
    # G.algos is a two level array: [func-commkind][algo]
    G.algos = load_coll_algos("src/mpi/coll/coll_algorithms.txt")
    # G.algo_list is a one level array [algo] 
    G.algo_list = collect_algo_list()

    G.out = []  # output to C file
    G.out2 = [] # output to header
    G.prototypes = []
    G.out.append("#include \"mpiimpl.h\"")
    G.out.append("#include \"iallgatherv/iallgatherv.h\"")

    # dump impl functions
    for a in G.coll_names:
        dump_coll_impl(a, "blocking")
        dump_coll_impl(a, "nonblocking")
        dump_coll_impl(a, "persistent")

    # TEMP: dump mpir functions.
    #       Current code base call MPIR_ functions in copositinal algorithms. Create a wrapper that call _impl
    #       for now. We will refactor the compositional algorithms later.
    for a in G.coll_names:
        dump_coll_mpir(a, "blocking")
        dump_coll_mpir(a, "nonblocking")

    # dump the container version of the algorithms
    dump_algo_cnt_fns()
    add_algo_prototypes()
    for a in G.coll_names:
        add_sched_auto_prototypes(a)

    # initialize MPIR_Coll_algo_table[algo_id] -> algo_fn
    dump_MPII_Coll_algo_init()
    # initialize MPIR_Coll_cvar_table
    dump_MPII_Coll_cvar_init()
    # create csel container from parsing json
    dump_MPII_Create_container()
    # routines for checking algorithm CVARs
    dump_MPIR_Coll_cvar_to_algo_id()
    dump_MPIR_Coll_init_algo_container()
    dump_MPIR_Coll_check_algo_restriction()

    # enum for coll_type
    dump_MPIR_Csel_coll_type_e()
    # enum for algorithm id
    dump_MPIR_Csel_container_type_e()
    # algorithm container struct
    dump_MPII_Csel_container()

    dump_c_file("src/mpi/coll/mpir_coll.c", G.out)
    dump_coll_algos_h("src/mpi/coll/include/coll_algos.h", G.prototypes, G.out2)

def collect_algo_list():
    algo_list = []
    for coll in G.coll_names:
        for commkind in ("intra", "inter"):
            func_commkind = coll + '-' + commkind
            if func_commkind in G.algos:
                for algo in G.algos[func_commkind]:
                    if "allcomm" in algo and func_commkind.endswith("inter"):
                        continue
                    algo_list.append(algo)
    for algo in G.algos['general']:
        algo_list.append(algo)
    return algo_list

def dump_algo_cnt_fns():
    def get_algo_args(coll_name, algo):
        args = get_coll_args(coll_name, "csel")
        if 'extra_params' in algo:
            args += ", " + get_algo_extra_args(algo, "csel")

        if algo['func-commkind'].startswith('i'):
            args += ", coll_sig->sched"
        elif algo['func-commkind'].startswith('neighbor_'):
            pass
        else:
            args += ", 0" # coll_attr

        return args

    def dump_algo_prep(algo):
        if algo['func-commkind'].startswith('i'):
            if algo['name'].startswith('tsp_'):
                G.out.append("MPII_CSEL_CREATE_TSP_SCHED(coll_sig);")
            else:
                G.out.append("MPII_CSEL_CREATE_SCHED(coll_sig);")

    for algo in G.algo_list:
        if algo["func-commkind"] != 'general':
            coll_name = get_algo_coll_name(algo)
            algo_funcname = get_algo_funcname(algo)
            algo_args = get_algo_args(coll_name, algo)
            decl = "int %s_cnt(%s)" % (algo_funcname, "MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt")
            add_prototype(decl)
            dump_split(0, decl)
            dump_open('{')
            G.out.append("int mpi_errno = MPI_SUCCESS;")
            G.out.append("")
            dump_algo_prep(algo)
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
    def get_algo_params(algo):
        coll_name = get_algo_coll_name(algo)
        params = get_coll_params(coll_name)
        if 'extra_params' in algo:
            params += ", " + get_algo_extra_params(algo)

        if algo['func-commkind'].startswith('i'):
            if algo['name'].startswith('tsp_'):
                params += ", MPIR_TSP_sched_t s"
            else:
                params += ", MPIR_Sched_t s"
        elif algo['func-commkind'].startswith('neighbor_'):
            pass
        else:
            params += ", int coll_attr" # coll_attr

        return params

    for algo in G.algo_list:
        if algo['func-commkind'] == 'general':
            decl = "int %s(%s)" % (algo['name'], "MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt")
            add_prototype(decl)
        else:
            algo_funcname = get_algo_funcname(algo)
            algo_params = get_algo_params(algo)
            decl = "int %s(%s)" % (algo_funcname, algo_params)
            add_prototype(decl)

def add_sched_auto_prototypes(coll_name):
    params = get_coll_params(coll_name)
    params += ", MPIR_Sched_t s"
    add_prototype("int MPIR_I%s_intra_sched_auto(%s)" % (coll_name, params))
    if not re.match(r'(scan|exscan|neighbor_)', coll_name):
        add_prototype("int MPIR_I%s_inter_sched_auto(%s)" % (coll_name, params))

def dump_MPII_Coll_cvar_init():
    G.out.append("")
    decl = "void MPII_Coll_cvar_init(void)"
    add_prototype(decl)
    G.out.append(decl)
    dump_open('{')
    for a in G.coll_names:
        for is_blocking in (True, False):
            G.out.append("MPIR_Coll_cvar_table[%s * 2] = %s;" % (coll_type(a, is_blocking), cvar_name(a, is_blocking, "intra")))
            if not re.match(r'(scan|exscan|neighbor_)', a):
                G.out.append("MPIR_Coll_cvar_table[%s * 2 + 1] = %s;" % (coll_type(a, is_blocking), cvar_name(a, is_blocking, "inter")))
            else:
                G.out.append("MPIR_Coll_cvar_table[%s * 2 + 1] = 0;" % (coll_type(a, is_blocking)))
    dump_close('}')

def dump_MPII_Coll_algo_init():
    G.out.append("")
    decl = "void MPIR_Coll_algo_init(void)"
    add_prototype(decl)
    G.out.append(decl)
    dump_open('{')
    for a in G.algo_list:
        algo_funcname = get_algo_funcname(a)
        idx = algo_id(algo_funcname)
        if a['func-commkind'] != 'general':
            algo_funcname += "_cnt"
        G.out.append("MPIR_Coll_algo_table[%s] = %s;" % (idx, algo_funcname))
    dump_close('}')


def dump_MPII_Create_container():
    G.out.append("")
    def dump_json_foreach_open():
        dump_open("json_object_object_foreach(obj, key, val) {")
        G.out.append("char *ckey = MPL_strdup_no_spaces(key);")

    def dump_json_foreach_close():
        G.out.append("MPL_free(ckey);")
        dump_close("}")

    def dump_parse_id():
        dump_json_foreach_open()
        if_clause = "if"
        for a in G.algo_list:
            algo_funcname = get_algo_funcname(a)
            G.out.append("%s(!strcmp(ckey, \"algorithm=%s\")) {" % (if_clause, algo_funcname))
            G.out.append("    cnt->id = %s;" % algo_id(algo_funcname))
            if_clause = "} else if"
        G.out.append("} else {")
        G.out.append("    fprintf(stderr, \"unrecognized key \%s\\n\", key);")
        G.out.append("}")
        dump_json_foreach_close()

    def dump_parse_params():
        dump_open("switch (cnt->id) {")
        for algo in G.algo_list:
            if 'extra_params' in algo:
                struct_name = algo_struct_name(algo)
                extra_params = algo['extra_params'].replace(' ', '').split(',')
                G.out.append("case %s:" % algo_id(get_algo_funcname(algo)))
                dump_open('{') # protect json_object_object_foreach
                dump_json_foreach_open()
                ifstr = "if"
                for a in extra_params:
                    if re.match(r'\w+=(.+)', a):
                        # skip constant parameter
                        continue
                    else:
                        n = len(a) + 1
                        atoi = "atoi"
                        if a == "tree_type":
                            atoi = "get_tree_type_from_string"
                        G.out.append("%s (!strncmp(ckey, \"%s=\", %d)) {" % (ifstr, a, n))
                        G.out.append("    cnt->u.%s.%s = %s(ckey + %d);" % (struct_name, a, atoi, n))
                        ifstr = "} else if"
                G.out.append("}")
                dump_json_foreach_close();
                dump_close('}')
                G.out.append("    break;")
        dump_close('}') # switch

    decl = "void *MPII_Create_container(struct json_object *obj)"
    add_prototype(decl)
    G.out.append(decl)
    dump_open('{')
    G.out.append("MPII_Csel_container_s *cnt = MPL_malloc(sizeof(MPII_Csel_container_s), MPL_MEM_COLL);")
    # FIXME: check cnt != NULL
    dump_parse_id()
    G.out.append("obj = json_object_object_get(obj, key);")
    dump_parse_params()
    G.out.append("")
    G.out.append("return (void *) cnt;")
    dump_close('}')

def dump_MPIR_Coll_cvar_to_algo_id():
    G.out.append("")
    def dump_cvar_cases(name, commkind):
        algo_id_prefix = "MPII_CSEL_CONTAINER_TYPE__ALGORITHM"

        dump_open("switch (cvar_val) {")
        G.out.append("case MPIR_CVAR_%s_%s_ALGORITHM_auto:" % (name.upper(), commkind.upper()))
        G.out.append("    return %s__MPIR_Coll_auto;" % (algo_id_prefix))
        if not name.startswith("i"): # blocking
            G.out.append("case MPIR_CVAR_%s_%s_ALGORITHM_nb:" % (name.upper(), commkind.upper()))
            G.out.append("    return %s__MPIR_Coll_nb;" % (algo_id_prefix))

        func_commkind = name + '-' + commkind
        for algo in G.algos[func_commkind]:
            G.out.append("case MPIR_CVAR_%s_%s_ALGORITHM_%s:" % (name.upper(), commkind.upper(), algo['name']))
            G.out.append("    return %s__%s;" % (algo_id_prefix, get_algo_funcname(algo)))
        dump_close("}")

    decl = "int MPIR_Coll_cvar_to_algo_id(int cvar_idx, int cvar_val)"
    add_prototype(decl)
    G.out.append(decl)
    dump_open("{")
    dump_open("switch (cvar_idx) {")
    coll_type = 0
    for coll in G.coll_names:
        for is_blocking in (True, False):
            if is_blocking:
                name = coll
            else:
                name = 'i' + coll
            for commkind in ("intra", "inter"):
                if commkind == "inter" and re.match(r'(scan|exscan|neighbor_)', coll):
                    continue
                cvar_idx = coll_type * 2
                if commkind == "inter":
                    cvar_idx += 1
                G.out.append("case %d: /* %s - %s */" % (cvar_idx, name, commkind))
                G.out.append("INDENT")
                dump_cvar_cases(name, commkind)
                G.out.append("break;")
                G.out.append("DEDENT")
            coll_type += 1
    dump_close("}")
    dump_close("}")

def dump_MPIR_Coll_init_algo_container():
    G.out.append("")
    decl = "void MPIR_Coll_init_algo_container(MPIR_Csel_coll_sig_s * coll_sig, int algo_id, MPII_Csel_container_s * cnt)"
    add_prototype(decl)
    G.out.append(decl)
    dump_open("{")
    G.out.append("memset(cnt, sizeof(*cnt), 0);")
    G.out.append("cnt->id = algo_id;")
    dump_open("switch (algo_id) {")
    for algo in G.algo_list:
        if "extra_params" in algo:
            struct_name = algo_struct_name(algo)
            extra_params = algo['cvar_params'].replace(' ', '').split(',')
            cvar_params = algo['cvar_params'].replace(' ', '').split(',')
            coll_name = algo['func-commkind'].split('-')[0]
            G.out.append("case %s:" % algo_id(get_algo_funcname(algo)))
            for i, a in enumerate(extra_params):
                if re.match(r'\w+=(.+)', a):
                    # skip constant parameter
                    continue
                else:
                    G.out.append("    cnt->u.%s.%s = MPIR_CVAR_%s_%s;" % (struct_name, a, coll_name.upper(), cvar_params[i]))
            G.out.append("    break;")
    dump_close("}")
    dump_close("}")

def dump_MPIR_Coll_check_algo_restriction():
    G.out.append("")
    def dump_check_restriction(coll_name, restriction):
        u = "coll_sig->%s" % coll_name
        if restriction == "inplace":
            G.out.append("    if (!(%s.sendbuf == MPI_IN_PLACE)) return false;" % u)
        elif restriction == "noinplace":
            G.out.append("    if (!(%s.sendbuf != MPI_IN_PLACE)) return false;" % u)
        elif restriction == "power-of-two":
            G.out.append("    if (!MPL_is_pof2(coll_sig->comm_ptr->local_size)) return false;")
        elif restriction == "size-ge-pof2":
            G.out.append("    if (!(%s.count >= MPL_pof2(coll_sig->comm_ptr->local_size)) return false;" % u)
        elif restriction == "commutative":
            G.out.append("    if (!MPIR_Op_is_commutative(%s.op)) return false;" % u)
        elif restriction== "builtin-op":
            G.out.append("    if (!HANDLE_IS_BUILTIN(%s.op)) return false;" % u)
        elif restriction == "parent-comm":
            G.out.append("    if (!MPIR_Comm_is_parent_comm(coll_sig->comm_ptr)) return false;")
        elif restriction == "node-consecutive":
            G.out.append("    if (!MPII_Comm_is_node_consecutive(coll_sig->comm_ptr)) return false;")
        elif restriction == "displs-ordered":
            # assume it's allgatherv
            G.out.append("    if (!MPII_Iallgatherv_is_displs_ordered(coll_sig->comm_ptr->local_size, %s.recvcounts, %s.displs)) return false;" % (u, u))
        else:
            raise Exception("Unsupported restrictions - %s" % restriction)
        pass

    decl = "bool MPIR_COLL_check_algo_restriction(MPIR_Csel_coll_sig_s * coll_sig, int algo_id)"
    add_prototype(decl)
    G.out.append(decl)
    dump_open("{")
    dump_open("switch (algo_id):")
    for algo in G.algo_list:
        if "restrictions" in algo:
            coll_name = algo['func-commkind'].split('-')[0]
            restrictions = algo['restrictions'].replace(' ', '').split(',')
            G.out.append("case %s:" % algo_id(get_algo_funcname(algo)))
            for r in restrictions:
                dump_check_restriction(coll_name, r)
            G.out.append("    break;")
    dump_close("}")
    G.out.append("return true;")
    dump_close("}")

# e.g. MPIR_CSEL_COLL_TYPE__BARRIER, etc.
def dump_MPIR_Csel_coll_type_e():
    G.out2.append("")
    G.out2.append("typedef enum {")
    for a in G.coll_names:
        for is_blocking in (True, False):
            G.out2.append("    %s," % coll_type(a, is_blocking))
    G.out2.append("    %s" % coll_type("END", True))
    G.out2.append("} MPIR_Csel_coll_type_e;")

def dump_MPIR_Csel_container_type_e():
    G.out2.append("")
    G.out2.append("typedef enum {")
    for a in G.algo_list:
        algo_funcname = get_algo_funcname(a)
        G.out2.append("    %s," % algo_id(algo_funcname))
    G.out2.append("    %s" % algo_id("Algorithm_count"))
    G.out2.append("} MPIR_Csel_container_type_e;")

def dump_MPII_Csel_container():
    G.out2.append("")
    def dump_algo_params():
        for algo in G.algo_list:
            if 'extra_params' in algo:
                extra_params = algo['extra_params'].replace(' ', '').split(',')
                G.out2.append("        struct {")
                for a in extra_params:
                    if re.match(r'\w+=(.+)', a):
                        # skip constant parameter
                        continue
                    else:
                        G.out2.append("            int %s;" % a)
                G.out2.append("        } %s;" % algo_struct_name(algo))

    G.out2.append("typedef struct MPII_Csel_container {")
    G.out2.append("    MPIR_Csel_container_type_e id;")
    G.out2.append("    union {")
    dump_algo_params()
    G.out2.append("    } u;")
    G.out2.append("} MPII_Csel_container_s;")

#---------------------------------------- 
def add_prototype(l):
    G.prototypes.append(l)

def load_coll_algos(algo_txt):
    All = {}
    with open(algo_txt) as In:
        (func_commkind, algo_list, algo) = (None, None, None)
        for line in In:
            if RE.match(r'(\w+-(intra|inter)|general):', line):
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

def dump_coll_mpir(name, blocking_type):
    def get_func_args(func):
        args = []
        for p in func['parameters']:
            if p['name'] == 'comm':
                args.append('comm_ptr')
            else:
                args.append(p['name'])
        return ', '.join(args)

    func = G.FUNCS["mpi_" + name]
    func_params = get_func_params(func, name, blocking_type)
    func_args = get_func_args(func)
    if blocking_type == "blocking":
        func_params += ", int coll_attr"
    else:
        func_args += ", request"

    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()

    G.out.append("")
    add_prototype("int MPIR_%s(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("return MPIR_%s_impl(%s);" % (Name, func_args))
    dump_close('}')

# ----
def get_func_name(name, blocking_type):
    if blocking_type == "blocking":
        return name
    elif blocking_type == "nonblocking":
        return 'i' + name
    elif blocking_type == "persistent":
        return name + "_init"

def get_algo_coll_name(algo):
    if algo["func-commkind"] == "general":
        raise Exception("general algo!")

    func_name, commkind = algo["func-commkind"].split("-")
    if func_name.startswith('i'):
        return func_name[1:]
    else:
        return func_name

def get_algo_funcname(algo):
    if algo["func-commkind"] == "general":
        return algo['name']

    func_name, commkind = algo["func-commkind"].split("-")
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

def get_coll_args(coll_name, kind):
    func = G.FUNCS["mpi_" + coll_name]
    args = []
    if kind == "csel":
        for p in func['parameters']:
            if p['name'] == 'comm':
                args.append("coll_sig->comm_ptr")
            else:
                args.append("coll_sig->u.%s.%s" % (coll_name, p['name']))
    else:
        raise Exception("unexpected kind")
    return ', '.join(args)

def get_coll_params(coll_name):
    func = G.FUNCS["mpi_" + coll_name]
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
                prefix = "cnt->u.%s." % (algo_struct_name(algo))
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
    if algo['name'].startswith('tsp_'):
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

def coll_type(coll_name, is_blocking):
    prefix = "MPIR_CSEL_COLL_TYPE"
    if is_blocking:
        return "%s__%s" % (prefix, coll_name.upper())
    else:
        return "%s__I%s" % (prefix, coll_name.upper())

def cvar_name(coll_name, is_blocking, commkind):
    if is_blocking:
        return "MPIR_CVAR_%s_%s_ALGORITHM" % (coll_name.upper(), commkind.upper())
    else:
        return "MPIR_CVAR_I%s_%s_ALGORITHM" % (coll_name.upper(), commkind.upper())

def algo_id(algo_funcname):
    prefix = "MPII_CSEL_CONTAINER_TYPE__ALGORITHM"
    # TODO: fix the tsp function name
    if RE.match(r'MPIR_TSP_(\w+)_sched_intra_(\w+)', algo_funcname):
        return "%s__MPIR_%s_intra_tsp_%s" % (prefix, RE.m.group(1), RE.m.group(2))
    else:
        return "%s__%s" % (prefix, algo_funcname)

def algo_struct_name(algo):
    algo_funcname = get_algo_funcname(algo)
    struct_name = re.sub(r'MPIR_', '', algo_funcname).lower()
    return struct_name

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

def dump_coll_algos_h(f, prototypes, lines):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)

        print("#ifndef COLL_ALGOS_H_INCLUDED", file=Out)
        print("#define COLL_ALGOS_H_INCLUDED", file=Out)
        print("", file=Out)

        for l in lines:
            print(l, file=Out)


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

def dump_macro_open(macro, dowhile=False):
    G.out2.append("#define %s \\" % macro)
    if dowhile:
        G.out2.append("    do { \\")

def dump_macro_close(dowhile=False):
    if dowhile:
        G.out2.append("    } while (0)")
    G.out2.append("")

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
