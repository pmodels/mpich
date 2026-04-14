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
    G.out.append("#include \"iallgatherv/iallgatherv.h\"")
    G.out.append("#include \"coll_csel.h\"")
    for a in G.coll_names:
        dump_coll(a, "blocking")
        dump_coll(a, "nonblocking")
        dump_coll(a, "persistent")

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
            G.out.append("int mpi_errno = MPI_SUCCESS;")
            G.out.append("")
            dump_algo_prep(func_name, algo)
            dump_split(1, "mpi_errno = %s(%s);" % (algo_funcname, algo_args))
            G.out.append("MPIR_ERR_CHECK(mpi_errno);")
            dump_fn_exit()
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
        if algo['func-commkind'] == "general":
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
    # temporarily add nb entries
    for coll in G.coll_names:
        algo = {"name": "nb", "func-commkind": "%s-intra" % coll, "allcomm": 1}
        G.algos[algo["func-commkind"]].append(algo)

def dump_coll(name, blocking_type):
    if blocking_type == "blocking":
        dump_allcomm_auto_blocking(name)
        dump_mpir_impl_blocking(name)
    elif blocking_type == "nonblocking":
        dump_allcomm_sched_auto(name)
        dump_sched_impl(name)
        dump_mpir_impl_nonblocking(name)
        dump_mpir_nonblocking_tag(name)
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
    G.out.append(".coll_type = MPIR_CSEL_COLL_TYPE__INTRA_%s," % NAME)
    G.out.append(".comm_ptr = comm_ptr,")
    for p in func['parameters']:
        if not re.match(r'comm$', p['name']):
            G.out.append(".u.%s.%s = %s," % (func_name, p['name'], p['name']))
    dump_close("};")
    dump_open("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {")
    G.out.append("coll_sig.coll_type = MPIR_CSEL_COLL_TYPE__INTER_%s;" % NAME)
    dump_close("};")
    G.out.append("")
    G.out.append("MPII_Csel_container_s *cnt = MPIR_Csel_search(MPIR_Csel_selection, &coll_sig);")
    G.out.append("MPIR_Assert(cnt);")
    G.out.append("")

    # -- switch
    def dump_cnt_algo_blocking(algo, commkind):
        if "allcomm" in algo:
            commkind = "allcomm"
        algo_name = get_algo_name(algo)
        algo_args = get_algo_args(args, algo, "csel")
        algo_params = get_algo_params(params, algo)
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
    G.out.append(".coll_type = MPIR_CSEL_COLL_TYPE__INTRA_%s," % NAME)
    G.out.append(".comm_ptr = comm_ptr,")
    for p in func['parameters']:
        if not re.match(r'comm$', p['name']):
            G.out.append(".u.%s.%s = %s," % (func_name, p['name'], p['name']))
    dump_close("};")
    dump_open("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {")
    G.out.append("coll_sig.coll_type = MPIR_CSEL_COLL_TYPE__INTER_%s;" % NAME)
    dump_close("};")
    G.out.append("")
    G.out.append("MPII_Csel_container_s *cnt = MPIR_Csel_search(MPIR_Csel_selection, &coll_sig);")
    G.out.append("MPIR_Assert(cnt);")
    G.out.append("")

    # -- switch
    def dump_cnt_algo_tsp(algo, commkind):
        G.out.append("MPII_GENTRAN_CREATE_SCHED_P();")
        algo_name = get_algo_name(algo)
        algo_args = get_algo_args(args, algo, "csel")
        algo_params = get_algo_params(params, algo)
        dump_split(3, "mpi_errno = MPIR_TSP_%s_sched_%s_%s(%s);" % (Name, commkind, algo_name, algo_args))

    def dump_cnt_algo_sched(algo, commkind):
        G.out.append("MPII_SCHED_CREATE_SCHED_P();")
        algo_name = get_algo_name(algo)
        algo_args = get_algo_args(args, algo, "csel")
        algo_params = get_algo_params(params, algo)
        dump_split(3, "mpi_errno = MPIR_%s_%s_%s(%s);" % (Name, commkind, algo_name, algo_args))

    def dump_cnt_algo_request(algo, commkind):
        G.out.append("*sched_type_p = MPIR_SCHED_REQUEST;")
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
            algo_funcname = get_algo_funcname(algo)
            G.out.append("case %s:" % algo_id(algo_funcname))
            G.out.append("INDENT")
            if algo['name'].startswith('tsp_'):
                dump_cnt_algo_tsp(algo, use_commkind)
            elif algo['name'].startswith('sched_'):
                dump_cnt_algo_sched(algo, use_commkind)
            else:
                dump_cnt_algo_request(algo, use_commkind)
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
    func_params = get_func_params(params, name, "blocking-impl")
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
    dump_split(0, "int MPIR_%s_impl(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("int coll_attr = 0;")
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

    def dump_algo_request(algo, commkind):
        G.out.append("*sched_type_p = MPIR_SCHED_REQUEST;")
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
                elif algo['name'].startswith('sched_'):
                    dump_algo_sched(algo, commkind)
                else:
                    dump_algo_request(algo, commkind)
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

def dump_mpir_nonblocking_tag(name):
    blocking_type = "nonblocking"
    func = G.FUNCS["mpi_" + name]
    params, args = get_params_and_args(func)
    func_params = get_func_params(params, name, "tag")

    func_name = get_func_name(name, blocking_type)
    Name = func_name.capitalize()
    NAME = func_name.upper()

    G.out.append("")
    add_prototype("int MPIR_%s_tag(%s)" % (Name, func_params))
    dump_split(0, "int MPIR_%s_tag(%s)" % (Name, func_params))
    dump_open('{')
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    G.out.append("enum MPIR_sched_type sched_type;")
    G.out.append("void *sched;")
    G.out.append("")
    G.out.append("*request = NULL;")
    func_args = get_func_args(args, name, "mpir_impl_tag")
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
    if blocking_type == "blocking":
        func_args = get_func_args(args, name, "blocking-impl")
    else:
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

        G.out.append("if(!MPIR_Typerep_reduce_is_supported(op, count, datatype))")
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

    dump_split(1, "mpi_errno = MPIR_%s_impl(%s);" % (Name, func_args))
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
        elif a == "!inplace":
            cond_list.append("sendbuf != MPI_IN_PLACE")
        elif a == "pof2":
            cond_list.append("MPL_is_pof2(comm_ptr->local_size)")
        elif a == "count_ge_pof2":
            cond_list.append("count >= MPL_pof2(comm_ptr->local_size)")
        elif a == "commutative":
            cond_list.append("MPIR_Op_is_commutative(op)")
        elif a== "builtin_op":
            cond_list.append("HANDLE_IS_BUILTIN(op)")
        elif a == "hierarchical":
            cond_list.append("MPIR_Comm_is_parent_comm(comm_ptr)")
        elif a == "node_consecutive":
            cond_list.append("MPII_Comm_is_node_consecutive(comm_ptr)")
        elif a == "displs_ordered":
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
def get_algo_args(args, algo, kind):
    algo_args = args
    if 'extra_params' in algo:
        algo_args += ", " + get_algo_extra_args(algo, kind)

    if algo['name'].startswith('tsp_'):
        algo_args += ", *sched_p"
    elif algo['name'].startswith('sched_'):
        algo_args += ", *sched_p"
    elif algo['func-commkind'].startswith('i'):
        # sched_p is "MPIR_Request ** request" here
        algo_args += ", (MPIR_Request **) sched_p"
    elif not algo['func-commkind'].startswith('neighbor_'):
        algo_args += ", coll_attr"

    return algo_args

def get_algo_params(params, algo):
    algo_params = params
    if 'extra_params' in algo:
        algo_params += ", " + get_algo_extra_params(algo)

    if algo['name'].startswith('tsp_'):
        algo_params += ", MPIR_TSP_sched_t sched"
    elif algo['name'].startswith('sched_'):
        algo_params += ", MPIR_Sched_t s"
    elif algo['func-commkind'].startswith('i'):
        algo_params += ", MPIR_Request ** request"
    elif not algo['func-commkind'].startswith('neighbor_'):
        algo_params += ", int coll_attr"

    return algo_params

def get_algo_name(algo):
    # the name used in algo function name
    if algo['name'].startswith('tsp_'):
        return algo['name'][4:]
    else:
        return algo['name']

def get_func_params(params, name, kind):
    func_params = params
    if kind == "blocking":
        if not name.startswith('neighbor_'):
            func_params += ", int coll_attr"
    elif kind == "blocking-impl":
        pass
    elif kind == "nonblocking":
        func_params += ", MPIR_Request ** request"
    elif kind == "tag":
        func_params += ", int tag, MPIR_Request ** request"
    elif kind == "persistent":
        func_params += ", MPIR_Info * info_ptr, MPIR_Request ** request"
    elif kind == "sched_auto":
        func_params += ", MPIR_Sched_t s"
    elif kind == "allcomm_sched_auto":
        func_params += ", int tag, bool is_persistent, void **sched_p, enum MPIR_sched_type *sched_type_p"
    elif kind == "sched_impl":
        func_params += ", int tag, bool is_persistent, void **sched_p, enum MPIR_sched_type *sched_type_p"
    else:
        raise Exception("get_func_params - unexpected kind = %s" % kind)

    return func_params

def get_func_args(args, name, kind):
    func_args = args
    if kind == "blocking":
        if not name.startswith('neighbor_'):
            func_args += ", coll_attr"
    elif kind == "blocking-impl":
        pass
    elif kind == "nonblocking":
        func_args += ", request"
    elif kind == "persistent":
        func_args += ", info_ptr, request"
    elif kind == "allcomm_sched_auto":
        func_args += ", tag, is_persistent, sched_p, sched_type_p"
    elif kind == "mpir_impl_nonblocking":
        func_args += ", 0, false, &sched, &sched_type"
    elif kind == "mpir_impl_tag":
        func_args += ", tag, false, &sched, &sched_type"
    elif kind == "mpir_impl_persistent":
        func_args += ", 0, true, &req->u.persist_coll.sched, &req->u.persist_coll.sched_type"
    else:
        raise Exception("get_func_args - unexpected kind = %s" % kind)

    return func_args

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
                if indent > 0 and not RE.match(r'#(if|endif)', l):
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
