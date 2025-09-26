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

    G.coll_names = ["barrier", "bcast", "gather", "gatherv", "scatter", "scatterv", "allgather", "allgatherv", "alltoall", "alltoallv", "alltoallw", "reduce", "allreduce", "reduce_scatter", "reduce_scatter_block", "scan", "exscan", "neighbor_allgather", "neighbor_allgatherv", "neighbor_alltoall", "neighbor_alltoallv", "neighbor_alltoallw"]

    # Loading coll_algorithms.txt. It sets -
    #   - G.conditions: a list of conditions that can be used as restrictions and in JSON tuning files
    #   - G.algos:      a two level array: [func-commkind][algo]
    load_coll_algos("src/mpi/coll/coll_algorithms.txt")
    # Prepare a one level algo array for conveninece -
    #   - G.algo_list:    a one level array [algo]
    G.algo_list = collect_algo_list()

    G.out = []  # output to mpir_coll.c
    G.out2 = [] # output to coll_algos.h
    G.prototypes = []
    G.out.append("#include \"mpiimpl.h\"")
    G.out.append("#include \"coll_csel.h\"")
    for a in G.coll_names:
        dump_coll_impl(a, "blocking")
        dump_coll_impl(a, "nonblocking")
        dump_coll_impl(a, "persistent")
        dump_coll_impl(a, "nonblocking_tag")

    # TEMP: dump mpir functions.
    #       Current code base call MPIR_ functions in compositional algorithms. Create a wrapper that call _impl
    #       for now. We will refactor the compositional algorithms later.
    for a in G.coll_names:
        dump_coll_mpir(a, "blocking")
        dump_coll_mpir(a, "nonblocking")

    # dump the container version of the algorithms
    dump_algo_cnt_fns()
    add_algo_prototypes()
    for a in G.coll_names:
        add_sched_auto_prototypes(a)

    # initialize MPIR_Coll_cvar_table and MPIR_Coll_type_names
    dump_MPII_Coll_type_init()
    # initialize MPIR_Coll_algo_table and MPIR_Coll_algo_names
    dump_MPII_Coll_algo_init()
    # initialize MPIR_Coll_condition_names
    dump_MPII_Csel_init_condition_names()
    # parsing routines for loading JSONs
    dump_MPII_Csel_parse_container()
    dump_MPII_Csel_parse_operator()
    # check operator condition in Csel search
    dump_MPII_Csel_run_condition()
    # routines for checking algorithm CVARs
    dump_MPIR_Coll_cvar_to_algo_id()
    dump_MPIR_Coll_init_algo_container()
    dump_MPIR_Coll_check_algo_restriction()

    # enum for coll_type, define MPIR_CSEL_NUM_COLL_TYPES
    dump_MPIR_Csel_coll_type_e()
    # enum for algorithm id, define MPIR_CSEL_NUM_ALGORITHMS
    dump_MPIR_Csel_container_type_e()
    # enum CSEL_NODE_TYPE, define MPIR_CSEL_NUM_CONDITIONS
    dump_MPIR_Csel_node_type_e()
    # algorithm container struct
    dump_MPII_Csel_container()
    G.out2.append("")

    dump_c_file("src/mpi/coll/mpir_coll.c", G.out)
    dump_coll_algos_h("src/mpi/coll/include/coll_algos.h", G.prototypes, G.out2)

#----------------------------------------
def collect_algo_list():
    algo_list = []
    for coll in G.coll_names:
        for commkind in ("intra", "inter"):
            for blocking in (True, False):
                if blocking:
                    func_commkind = coll + '-' + commkind
                else:
                    func_commkind = 'i' + coll + '-' + commkind
                if func_commkind in G.algos:
                    for algo in G.algos[func_commkind]:
                        if "allcomm" in algo and func_commkind.endswith("inter"):
                            continue
                        algo_list.append(algo)
    for algo in G.algos['general']:
        algo_list.append(algo)
    return algo_list

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

    G.out.append("")
    for algo in G.algo_list:
        if algo["func-commkind"] != 'general':
            func_name, commkind = algo['func-commkind'].split("-")
            if func_name.startswith('i'):
                # use blocking func for base parameters
                func = G.FUNCS["mpi_" + func_name[1:]]
            else:
                func = G.FUNCS["mpi_" + func_name]
            algo_funcname = get_algo_funcname(algo)
            algo_args = get_algo_args(func, func_name, algo)
            decl = "int %s_cnt(%s)" % (algo_funcname, "MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt")
            add_prototype(decl)
            dump_split(0, decl)
            dump_open('{')
            if 'macro_guard' in algo:
                G.out.append("#if %s" % algo['macro_guard'])
            G.out.append("int mpi_errno = MPI_SUCCESS;")
            G.out.append("")
            dump_algo_prep(func_name, algo)
            G.out.append("MPIR_Coll_algo_counters[%s]++;" % algo_id(algo_funcname))
            dump_split(1, "mpi_errno = %s(%s);" % (algo_funcname, algo_args))
            G.out.append("MPIR_ERR_CHECK(mpi_errno);")
            dump_fn_exit()
            if 'macro_guard' in algo:
                G.out.append("#else")
                G.out.append("return MPI_ERR_OTHER;")
                G.out.append("#endif")
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
            elif algo['name'].startswith('sched_'):
                params += ", MPIR_Sched_t s"
            else:
                params += ", MPIR_Request **request"
        elif func_name.startswith('neighbor_'):
            pass
        else:
            params += ", int coll_attr" # coll_attr

        return params

    for algo in G.algo_list:
        if 'inline' in algo:
            # inline functions are already defined in headers, do not need prototypes
            continue
        elif algo['func-commkind'] == "general":
            continue
        func_name, commkind = algo['func-commkind'].split("-")
        if func_name.startswith('i'):
            # use blocking func for base parameters
            func = G.FUNCS["mpi_" + func_name[1:]]
        else:
            func = G.FUNCS["mpi_" + func_name]

        algo_funcname = get_algo_funcname(algo)
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

#---- dumping to G.out (mpir_coll.c) ----

def dump_MPII_Coll_type_init():
    G.out.append("")
    decl = "void MPII_Coll_type_init(void)"
    G.out.append(decl)
    dump_open('{')

    # populate MPIR_Coll_cvar_table[] with CVAR values at init time
    for a in G.coll_names:
        for commkind in ("intra", "inter"):
            for is_blocking in (True, False):
                if commkind == "inter" and re.match(r'(scan|exscan|neighbor_)', a):
                    # CVARs for these inter-collective does not exist
                    G.out.append("MPIR_Coll_cvar_table[%s] = 0;" % (coll_type(a, is_blocking, commkind)))
                else:
                    G.out.append("MPIR_Coll_cvar_table[%s] = %s;" % (coll_type(a, is_blocking, commkind), cvar_name(a, is_blocking, commkind)))

    # populate MPIR_Coll_type_names[]
    for a in G.coll_names:
        for commkind in ("intra", "inter"):
            for is_blocking in (True, False):
                if is_blocking:
                    coll_type_name = a + '-' + commkind
                else:
                    coll_type_name = 'i' + a + '-' + commkind
                G.out.append("MPIR_Coll_type_names[%s] = \"%s\";" % (coll_type(a, is_blocking, commkind), coll_type_name))

    dump_close('}')

def dump_MPII_Csel_init_condition_names():
    G.out.append("")
    decl = "void MPII_Csel_init_condition_names(void)"
    G.out.append(decl)
    dump_open('{')
    for a in G.conditions:
        G.out.append("MPIR_Csel_condition_names[%s] = \"%s\";" % (condition_id(a), get_condition_name(a)))
    dump_close('}')

def dump_MPII_Coll_algo_init():
    G.out.append("")
    decl = "void MPII_Coll_algo_init(void)"
    G.out.append(decl)
    dump_open('{')
    for a in G.algo_list:
        algo_funcname = get_algo_funcname(a)
        idx = algo_id(algo_funcname)
        if a['func-commkind'] != 'general':
            algo_funcname += "_cnt"
        G.out.append("MPIR_Coll_algo_table[%s] = %s;" % (idx, algo_funcname))
    for a in G.algo_list:
        algo_funcname = get_algo_funcname(a)
        idx = algo_id(algo_funcname)
        G.out.append("MPIR_Coll_algo_names[%s] = \"%s\";" % (idx, algo_funcname))
    dump_close('}')

def dump_MPII_Csel_parse_container():
    """Generate MPII_Csel_parse_container_params()."""
    G.out.append("")
    def dump_json_foreach_open():
        dump_open("json_object_object_foreach(obj, key, val) {")
        G.out.append("char *ckey = MPL_strdup_no_spaces(key);")

    def dump_json_foreach_close():
        G.out.append("MPL_free(ckey);")
        dump_close("}")

    def dump_parse_params():
        dump_open("switch (cnt->id) {")
        for algo in G.algo_list:
            if 'extra_params' in algo:
                struct_name = algo_struct_name(algo)
                extra_params = algo['extra_params'].replace(' ', '').split(',')
                G.out.append("case %s:" % algo_id(get_algo_funcname(algo)))
                dump_open('{') # protect json_object_object_foreach
                G.out.append("int num_params = 0;")
                num_expect = 0
                dump_json_foreach_open()
                ifstr = "if"
                for a in extra_params:
                    if re.match(r'\w+=(.+)', a):
                        # skip constant parameter
                        continue
                    else:
                        num_expect += 1
                        n = len(a) + 1
                        atoi = "atoi"
                        if a == "tree_type":
                            atoi = "get_tree_type_from_string"
                        G.out.append("%s (!strncmp(ckey, \"%s=\", %d)) {" % (ifstr, a, n))
                        G.out.append("    cnt->u.%s.%s = %s(ckey + %d);" % (struct_name, a, atoi, n))
                        G.out.append("    num_params++;")
                        ifstr = "} else if"
                G.out.append("}")
                dump_json_foreach_close();
                dump_open("if (num_params != %d) {" % num_expect)
                G.out.append("printf(\"MPII_Csel_parse_container_params: algorithm %s expect %d parameters\\\n\");" % (struct_name, num_expect))
                dump_close('}')
                G.out.append("MPIR_Assert(num_params == %d);" % num_expect)
                dump_close('}')
                G.out.append("    break;")
        G.out.append("default:")
        G.out.append("    break;")
        dump_close('}') # switch

    decl = "void MPII_Csel_parse_container_params(struct json_object *obj, MPII_Csel_container_s *cnt)"
    G.out.append(decl)
    dump_open('{')
    dump_parse_params()
    dump_close('}')

def dump_MPII_Csel_run_condition():
    decl = "MPIR_Csel_node_s *MPII_Csel_run_condition(MPIR_Csel_node_s *node, MPIR_Csel_coll_sig_s *coll_sig)"
    G.out.append(decl)
    dump_open('{')
    G.out.append("bool cond;")
    dump_open("switch(node->type) {")
    for a in G.conditions:
        has_thresh = condition_has_thresh(a)
        cond_func = get_condition_function(a)
        dump_open("case %s:" % condition_id(a))
        if has_thresh:
            G.out.append("cond = (%s(coll_sig) <= node->u.condition.thresh);" % cond_func)
        else:
            G.out.append("cond = %s(coll_sig);" % cond_func)
        G.out.append("if (node->u.condition.negate) cond = !cond;")
        G.out.append("return cond ? node->success : node->failure;")
        dump_close("")
    G.out.append("default:")
    G.out.append("    break;")
    dump_close('}') # switch
    G.out.append("return NULL;")
    dump_close('}')

def dump_MPII_Csel_parse_operator():
    def dump_static_parse_thresh():
        # following are interchangeable:
        #     cond<=thresh is the same as cond(thresh)
        #     cond<thresh is the same as cond(thresh-1)
        #     cond>thresh is the same as !cond(thresh)
        #     cond>=thresh is the same as !cond(thresh-1)
        G.out.append("static void parse_thresh(const char *ckey, MPIR_Csel_node_s *node)")
        dump_open('{')
        G.out.append("if (ckey[0] == '(') {")
        G.out.append("    node->u.condition.thresh = atoi(ckey + 1);")
        G.out.append("} else if (ckey[0] == '<' && ckey[1] == '=') {")
        G.out.append("    node->u.condition.thresh = atoi(ckey + 2);")
        G.out.append("} else if (ckey[0] == '<') {")
        G.out.append("    node->u.condition.thresh = atoi(ckey + 1) - 1;")
        G.out.append("} else if (ckey[0] == '>' && ckey[1] == '=') {")
        G.out.append("    node->u.condition.thresh = atoi(ckey + 2) - 1;")
        G.out.append("    node->u.condition.negate = true;")
        G.out.append("} else if (ckey[0] == '>') {")
        G.out.append("    node->u.condition.thresh = atoi(ckey + 1);")
        G.out.append("    node->u.condition.negate = true;")
        G.out.append("} else {")
        G.out.append("    MPIR_Assert(0);")
        G.out.append('}')
        dump_close('}')
        G.out.append("")

    dump_static_parse_thresh()

    decl = "int MPII_Csel_parse_operator(const char *ckey, MPIR_Csel_node_s *node)"
    G.out.append(decl)
    dump_open('{')
    dump_open("if (ckey[0] == '!') {")
    G.out.append("node->u.condition.negate = true;")
    G.out.append("ckey++;")
    dump_else()
    G.out.append("node->u.condition.negate = false;")
    dump_close('}')

    if_clase = "if"
    for a in G.conditions:
        name = get_condition_name(a)
        has_thresh = condition_has_thresh(a)
        n = len(name)
        if has_thresh:
            G.out.append("%s (strncmp(ckey, \"%s\", %d) == 0) {" % (if_clase, name, n))
            G.out.append("    node->type = %s;" % condition_id(a))
            G.out.append("    parse_thresh(ckey + %d, node);" % n)
        else:
            G.out.append("%s (strcmp(ckey, \"%s\") == 0) {" % (if_clase, name))
            G.out.append("    node->type = %s;" % condition_id(a))
        if_clase = "} else if"
    G.out.append("} else {")
    G.out.append("    MPIR_Assert(0);")
    G.out.append("    return MPI_ERR_OTHER;")
    G.out.append("}")

    G.out.append("")
    G.out.append("return MPI_SUCCESS;")
    dump_close('}')

def dump_MPIR_Coll_cvar_to_algo_id():
    G.out.append("")
    def dump_cvar_cases(name, commkind):
        algo_id_prefix = "MPII_CSEL_CONTAINER_TYPE__ALGORITHM"

        dump_open("switch (cvar_val) {")
        G.out.append("case MPIR_CVAR_%s_%s_ALGORITHM_auto:" % (name.upper(), commkind.upper()))
        G.out.append("    MPIR_Assert(0); /* auto cvar_val should be 0 and shouldn't be called here */")
        G.out.append("    return %s;" % algo_id_END())
        if commkind == "intra" and not name.startswith('i'):
            G.out.append("case MPIR_CVAR_%s_%s_ALGORITHM_nb:" % (name.upper(), commkind.upper()))
            G.out.append("    return %s__%s;" % (algo_id_prefix, "MPIR_Coll_nb"))

        func_commkind = name + '-' + commkind
        for algo in G.algos[func_commkind]:
            G.out.append("case MPIR_CVAR_%s_%s_ALGORITHM_%s:" % (name.upper(), commkind.upper(), algo['name']))
            G.out.append("    return %s__%s;" % (algo_id_prefix, get_algo_funcname(algo)))
        dump_close("}")

    decl = "int MPIR_Coll_cvar_to_algo_id(int coll_type, int cvar_val)"
    add_prototype(decl)
    G.out.append(decl)
    dump_open("{")
    G.out.append("MPIR_Assert(cvar_val > 0);")
    dump_open("switch (coll_type) {")
    for coll in G.coll_names:
        for commkind in ("intra", "inter"):
            for is_blocking in (True, False):
                if is_blocking:
                    name = coll
                else:
                    name = 'i' + coll
                if commkind == "inter" and re.match(r'(scan|exscan|neighbor_)', coll):
                    continue
                G.out.append("case %s:" % coll_type(coll, is_blocking, commkind))
                G.out.append("INDENT")
                dump_cvar_cases(name, commkind)
                G.out.append("break;")
                G.out.append("DEDENT")
    dump_close("}")
    G.out.append("MPIR_Assert(0);")
    G.out.append("return 0;")
    dump_close("}")

def dump_MPIR_Coll_init_algo_container():
    G.out.append("")
    decl = "void MPIR_Coll_init_algo_container(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_type_e algo_id, MPII_Csel_container_s * cnt)"
    G.out.append(decl)
    dump_open("{")
    G.out.append("memset(cnt, 0, sizeof(*cnt));")
    G.out.append("cnt->id = algo_id;")
    dump_open("switch (algo_id) {")
    for algo in G.algo_list:
        if "extra_params" in algo:
            struct_name = algo_struct_name(algo)
            extra_params = algo['extra_params'].replace(' ', '').split(',')
            cvar_params = algo['cvar_params'].replace(' ', '').split(',')
            coll_name = algo['func-commkind'].split('-')[0]
            G.out.append("case %s:" % algo_id(get_algo_funcname(algo)))
            for i, a in enumerate(extra_params):
                if re.match(r'\w+=(.+)', a):
                    # skip constant parameter
                    continue
                else:
                    cvar_param = "MPIR_CVAR_%s_%s" % (coll_name.upper(), cvar_params[i])
                    if a == "tree_type":
                        cvar_param = "get_tree_type_from_string(%s)" % cvar_param
                    elif cvar_params[i] == "THROTTLE":
                        cvar_param = "MPIR_CVAR_ALLTOALL_THROTTLE"
                    G.out.append("    cnt->u.%s.%s = %s;" % (struct_name, a, cvar_param))
            G.out.append("    break;")
    G.out.append("default:")
    G.out.append("    break;")
    dump_close("}")
    dump_close("}")

def dump_MPIR_Coll_check_algo_restriction():
    G.out.append("")
    def dump_check_restriction(restriction):
        r = restriction
        negate = False
        if restriction.startswith('!'):
            r = restriction[1:]
            negate = True
        if RE.match(r'.*\(.*\)', r):
            raise Exception("Threshold condition %s cannot be used as a restriction" % r)

        if r in G.conditions:
            # We assume we can directly call conditional condition since we are inside the algorithm macro_guard
            cond = "%s(coll_sig)" % get_condition_function(r)
            if negate:
                G.out.append("    if (%s) return false;" % cond)
            else:
                G.out.append("    if (!%s) return false;" % cond)
        else:
            raise Exception("Restriction %s not listed" % restriction)


    decl = "bool MPIR_Coll_check_algo_restriction(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_type_e algo_id)"
    G.out.append(decl)
    dump_open("{")
    dump_open("switch (algo_id) {")
    for algo in G.algo_list:
        if "restrictions" in algo:
            restrictions = algo['restrictions'].replace(' ', '').split(',')
            G.out.append("case %s:" % algo_id(get_algo_funcname(algo)))
            for r in restrictions:
                dump_check_restriction(r)
            G.out.append("    break;")
    G.out.append("default:")
    G.out.append("    return true;")
    dump_close("}")
    G.out.append("return true;")
    dump_close("}")

#---- dumping to G.out2 (coll_algos.h) ----

# e.g. MPIR_CSEL_COLL_TYPE__BARRIER, etc.
def dump_MPIR_Csel_coll_type_e():
    G.out2.append("")
    G.out2.append("enum MPIR_Csel_coll_type {")
    # IMPORTANT: MPIR_Coll_nb algorithm relies on that blocking coll_type is even and its
    #            nonblocking correspondent is coll_type+1.
    for a in G.coll_names:
        for commkind in ("intra", "inter"):
            for is_blocking in (True, False):
                G.out2.append("    %s," % coll_type(a, is_blocking, commkind))
    G.out2.append("    %s" % coll_type_END())
    G.out2.append("};")
    G.out2.append("")
    G.out2.append("#define MPIR_CSEL_NUM_COLL_TYPES %s" % coll_type_END())

def dump_MPIR_Csel_container_type_e():
    G.out2.append("")
    G.out2.append("enum MPII_Csel_container_type {")
    for a in G.algo_list:
        algo_funcname = get_algo_funcname(a)
        G.out2.append("    %s," % algo_id(algo_funcname))
    G.out2.append("    %s" % algo_id_END())
    G.out2.append("};")
    G.out2.append("")
    G.out2.append("#define MPIR_CSEL_NUM_ALGORITHMS %s" % algo_id_END())

def dump_MPIR_Csel_node_type_e():
    """generate condition IDs as CSEL_NODE_TYPE__OPERATOR__XXX."""
    G.out2.append("")
    G.out2.append("typedef enum {")
    for a in G.conditions:
        G.out2.append("    %s," % condition_id(a))
    G.out2.append("    CSEL_NODE_TYPE__OPERATOR__COLLECTIVE,")
    G.out2.append("    CSEL_NODE_TYPE__OPERATOR__ANY,")
    G.out2.append("    CSEL_NODE_TYPE__OPERATOR__CALL,")
    G.out2.append("    CSEL_NODE_TYPE__CONTAINER,")
    G.out2.append("} MPIR_Csel_node_type_e;")
    G.out2.append("")
    G.out2.append("#define MPIR_CSEL_NUM_CONDITIONS %s\n" % "CSEL_NODE_TYPE__OPERATOR__COLLECTIVE")

def dump_MPII_Csel_container():
    """Generate struct MPII_Csel_container."""
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
    G.out2.append("    MPII_Csel_container_type_e id;")
    G.out2.append("    union {")
    dump_algo_params()
    G.out2.append("    } u;")
    G.out2.append("} MPII_Csel_container_s;")

#----------------------------------------
def add_prototype(l):
    G.prototypes.append(l)

def load_coll_algos(algo_txt):
    G.algos = {}
    G.conditions = {}
    with open(algo_txt) as In:
        (func_commkind, algo_list, algo) = (None, None, None)
        for line in In:
            if RE.match(r'(\w+-(intra|inter)|general|conditions):', line):
                func_commkind = RE.m.group(1)
                if func_commkind != "conditions":
                    algo_list = []
                    G.algos[func_commkind] = algo_list
            elif func_commkind == "conditions":
                if RE.match(r'\s+([\w()-]+):\s*(\w+.*)', line):
                    G.conditions[RE.m.group(1)] = RE.m.group(2)
            elif func_commkind:
                if RE.match(r'\s+(\w+)\s*$', line):
                    algo = {"name": RE.m.group(1), "func-commkind": func_commkind}
                    algo_list.append(algo)
                elif RE.match(r'\s+(\w+):\s*(.+)', line):
                    algo[RE.m.group(1)] = RE.m.group(2)

def dump_coll_impl(name, blocking_type):
    func = G.FUNCS["mpi_" + name]
    func_params = get_impl_params(func, blocking_type)

    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()

    decl_name = "MPIR_%s_impl" % Name
    if blocking_type == 'nonblocking_tag':
        decl_name = "MPIR_%s_tag" % Name

    G.out.append("")
    add_prototype("int %s(%s)" % (decl_name, func_params))
    dump_split(0, "int %s(%s)" % (decl_name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("")

    # Initialize coll_sig
    G.out.append("MPIR_Csel_coll_sig_s coll_sig;")
    NAME = name.upper()
    if blocking_type != "blocking":
        # nonblocking and persistent
        NAME = 'I' + NAME
    dump_open("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {")
    G.out.append("coll_sig.coll_type = MPIR_CSEL_COLL_TYPE__INTRA_%s;" % NAME)
    dump_else()
    G.out.append("coll_sig.coll_type = MPIR_CSEL_COLL_TYPE__INTER_%s;" % NAME)
    dump_close('}')
    G.out.append("coll_sig.comm_ptr = comm_ptr;")
    if blocking_type == "persistent":
        G.out.append("coll_sig.is_persistent = true;")
    else:
        G.out.append("coll_sig.is_persistent = false;")
    if blocking_type == "nonblocking_tag":
        G.out.append("coll_sig.tag = tag;")
    else:
        G.out.append("coll_sig.tag = 0;")
    G.out.append("coll_sig.sched = NULL;")

    phash = {}
    for p in func['parameters']:
        if p['name'] == 'comm':
            pass
        else:
            phash[p['name']] = 1
            G.out.append("coll_sig.u.%s.%s = %s;" % (name, p['name'], p['name']))

    G.out.append("MPID_Init_coll_sig(&coll_sig);")

    # Call csel
    G.out.append("")
    G.out.append("mpi_errno = MPIR_Coll_auto(&coll_sig, NULL);")
    G.out.append("MPIR_ERR_CHECK(mpi_errno);")
    G.out.append("")

    # Set request if nonblocking or persistent
    if blocking_type == "blocking":
        pass
    elif blocking_type == "nonblocking" or blocking_type == "nonblocking_tag":
        G.out.append("MPII_SCHED_START(coll_sig.sched_type, coll_sig.sched, comm_ptr, request);")
        G.out.append("")
    elif blocking_type == "persistent":
        G.out.append("MPIR_Request *req = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_COLL);")
        G.out.append("MPIR_ERR_CHKANDJUMP(!req, mpi_errno, MPI_ERR_OTHER, \"**nomem\");")
        G.out.append("MPIR_Comm_add_ref(comm_ptr);")
        G.out.append("req->comm = comm_ptr;")
        G.out.append("MPIR_Comm_save_inactive_request(comm_ptr, req);")
        G.out.append("req->u.persist_coll.sched_type = coll_sig.sched_type;")
        G.out.append("req->u.persist_coll.sched = coll_sig.sched;")
        G.out.append("req->u.persist_coll.real_request = NULL;")
        G.out.append("*request = req;")
        G.out.append("")
    else:
        raise Exception("Wrong blocking_type")

    dump_fn_exit()
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
    func_params = get_impl_params(func, blocking_type)
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

#---------------------------------------- 
def get_func_name(name, blocking_type):
    if blocking_type == "blocking":
        return name
    elif blocking_type == "nonblocking" or blocking_type == "nonblocking_tag":
        return 'i' + name
    elif blocking_type == "persistent":
        return name + "_init"

def get_algo_funcname(algo):
    if "func_name" in algo:
        return algo['func_name']
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
                prefix = "cnt->u.%s." % algo_struct_name(algo)
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

def get_impl_params(func, blocking_type):
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
    elif blocking_type == "nonblocking_tag":
        params.append("int tag")
        params.append("MPIR_Request ** request")
    else:
        raise Exception("get_impl_params - unexpected blocking_type = %s" % blocking_type)

    return ', '.join(params)

# ----------------------
def coll_type(coll_name, is_blocking, commkind):
    prefix = "MPIR_CSEL_COLL_TYPE"
    if is_blocking:
        return "%s__%s_%s" % (prefix, commkind.upper(), coll_name.upper())
    else:
        return "%s__%s_I%s" % (prefix, commkind.upper(), coll_name.upper())

def coll_type_END():
    return "MPIR_CSEL_COLL_TYPE__END"

def cvar_name(coll_name, is_blocking, commkind):
    if is_blocking:
        return "MPIR_CVAR_%s_%s_ALGORITHM" % (coll_name.upper(), commkind.upper())
    else:
        return "MPIR_CVAR_I%s_%s_ALGORITHM" % (coll_name.upper(), commkind.upper())

def algo_id(algo_funcname):
    prefix = "MPII_CSEL_CONTAINER_TYPE__ALGORITHM"
    return "%s__%s" % (prefix, algo_funcname)

def algo_id_END():
    return "MPII_CSEL_CONTAINER_TYPE__ALGORITHM__END"

def algo_struct_name(algo):
    """Union member name for this algo, i.e. cnt->u.xxx, which is a struct for its extra params."""
    algo_funcname = get_algo_funcname(algo)
    struct_name = re.sub(r'MPIR_', '', algo_funcname).lower()
    return struct_name

def condition_has_thresh(a):
    return a.endswith('(thresh)')

def get_condition_name(a):
    name = re.sub(r'\(thresh\)$', '', a)
    return name

def get_condition_function(a):
    if RE.match(r'(\w+)\s*$', G.conditions[a]):
        return RE.m.group(1)
    else:
        return "MPIR_CSEL_check_" + get_condition_name(a)

def condition_id(a):
    prefix = "CSEL_NODE_TYPE__OPERATOR__"
    return prefix + get_condition_name(a)

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
                if indent > 0 and not RE.match(r'#(if|else|endif)', l):
                    print("    " * indent, end='', file=Out)
                print(l, file=Out)

def dump_coll_algos_h(f, prototypes, extra_lines):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)

        print("#ifndef COLL_ALGOS_H_INCLUDED", file=Out)
        print("#define COLL_ALGOS_H_INCLUDED", file=Out)
        print("", file=Out)

        for l in extra_lines:
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

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
