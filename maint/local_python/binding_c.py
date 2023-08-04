##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.binding_common import *
from local_python import RE

import sys
import re
import copy

# ---- top-level routines ----

def dump_mpi_c(func, is_large=False):
    """Dumps the function's C source code to G.out array"""

    # whether it is a large count function
    func['_is_large'] = is_large

    check_func_directives(func)
    filter_c_parameters(func)
    check_params_with_large_only(func)

    # for poly functions, decide impl interface
    check_large_parameters(func)

    process_func_parameters(func)

    G.mpi_declares.append(get_declare_function(func, is_large, "proto"))

    # collect error codes additional from auto generated ones
    if 'error' in func:
        for a in func['error'].split(", "):
            G.err_codes[a] = 1

    # Some routines, e.g. MPIR_Comm_split_filesystem, are defined in romio or libromio.la, but are
    # only referenced from libpmpi.la. This causes an issue in static linking when weak symbols are
    # not available.
    #
    # As an hack, we reference these symbols in selected routines to force the inclusion of romio
    # routines.
    def dump_romio_reference(name):
        if G.need_dump_romio_reference:
            G.out.append("#if defined(HAVE_ROMIO) && defined(MPICH_MPI_FROM_PMPI)")
            G.out.append("void *dummy_refs_%s[] = {" % name)
            G.out.append("    (void *) MPIR_Comm_split_filesystem,")
            G.out.append("    (void *) MPIR_ROMIO_Get_file_errhand,")
            G.out.append("    (void *) MPIR_ROMIO_Set_file_errhand,")
            G.out.append("};")
            G.out.append("#endif")
            G.need_dump_romio_reference = False

    # -- "dump" accumulates output lines in G.out
    if not is_large:
        # only include once (skipping "BIG")
        if 'include' in func:
            for a in func['include'].replace(',', ' ').split():
                G.out.append("#include \"%s\"" % a)

        # hack to get romio routines to work with static linking + no-weak-symbols
        if re.match(r'mpi_(init|get_library_version|session_init|t_init_thread)', func['name'], re.IGNORECASE):
            dump_romio_reference(func['name'])

    G.out.append("")

    dump_profiling(func)

    if 'polymorph' in func:
        # MPII_ function to support C/Fortran Polymorphism, eg MPI_Comm_get_attr
        G.out.append("#ifndef MPICH_MPI_FROM_PMPI")
        dump_function_internal(func, kind="polymorph")
        G.out.append("#endif /* MPICH_MPI_FROM_PMPI */")
        G.out.append("")

    if 'polymorph' in func:
        dump_function_internal(func, kind="call-polymorph")
    elif 'replace' in func and 'body' not in func:
        declare_call_replace_internal(func)
    else:
        dump_function_internal(func, kind="normal")
    G.out.append("")

    # Create the MPI and QMPI wrapper functions that will call the above, "real" version of the
    # function in the internal prefix
    dump_qmpi_wrappers(func, func['_is_large'])

def get_func_file_path(func, root_dir):
    file_path = None
    dir_path = root_dir + '/' + func['dir']
    if 'file' in func:
        file_path = dir_path + '/' + func['file'] + ".c"
    elif RE.match(r'MPI_T_(\w+)', func['name'], re.IGNORECASE):
        name = RE.m.group(1)
        file_path = dir_path + '/' + name.lower() + ".c"
    elif RE.match(r'MPIX?_(\w+)', func['name'], re.IGNORECASE):
        name = RE.m.group(1)
        file_path = dir_path + '/' + name.lower() + ".c"
    else:
        raise Exception("Error in function name pattern: %s\n" % func['name'])

    return file_path

def get_mansrc_file_path(func, root_dir):
    file_path = None
    dir_path = root_dir
    if 'file' in func:
        file_path = dir_path + '/' + func['file'] + ".txt"
    elif RE.match(r'MPI_T_(\w+)', func['name'], re.IGNORECASE):
        name = RE.m.group(1)
        file_path = dir_path + '/' + name.lower() + ".txt"
    elif RE.match(r'MPIX?_(\w+)', func['name'], re.IGNORECASE):
        name = RE.m.group(1)
        file_path = dir_path + '/' + name.lower() + ".txt"
    else:
        raise Exception("Error in function name pattern: %s\n" % func['name'])

    return file_path

def dump_c_file(f, lines):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        (indent, prev_empty) = (0, 0)
        for l in G.copyright_c:
            print(l, file=Out)
        for l in lines:
            # stripping the newline for consistency
            l = l.rstrip()
            # handle special directives etc.
            if RE.match(r'(INDENT|DEDENT)', l):
                # indentations
                a = RE.m.group(1)
                if a == "INDENT":
                    indent += 1
                else:
                    indent -= 1
            elif RE.match(r'fn_\w+:', l):
                # goto labels, use 2-space indentation
                print("  %s" % (l), file=Out)
            elif RE.match(r'\s*$', l):
                # empty lines, avoid double empty lines
                if not prev_empty:
                    print("", file=Out)
                    prev_empty = 1
            else:
                # print the line with correct indentations
                if indent > 0 and not RE.match(r'#(if|endif)', l):
                    print("    " * indent, end='', file=Out)
                print(l, file=Out)
                prev_empty = 0

def dump_Makefile_mk(f):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_mk:
            print(l, file=Out)

        print("mpi_sources += \\", file=Out)
        n = len(G.mpi_sources)
        for i, f in enumerate(G.mpi_sources):
            if i < n - 1:
                print("    %s \\" % f, file=Out)
            else:
                print("    %s" % f, file=Out)

        n = len(G.doc3_src_txt)
        if n > 0:
            print("doc3_src_txt += \\", file=Out)
            for i, f in enumerate(G.doc3_src_txt):
                if i < n - 1:
                    print("    %s \\" % f, file=Out)
                else:
                    print("    %s" % f, file=Out)

def dump_mpir_impl_h(f):
    def dump_mpix_symbols():
        # define MPI symbols from future API to MPIX
        print("", file=Out)
        for a, t in G.mpix_symbols.items():
            if t == "symbols":
                print("#define %s %s" % (a, re.sub(r'^MPI_', 'MPIX_', a)), file=Out)

    print("  --> [%s]" %f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)
        print("#ifndef MPIR_IMPL_H_INCLUDED", file=Out)
        print("#define MPIR_IMPL_H_INCLUDED", file=Out)
        dump_mpix_symbols()
        print("", file=Out)
        for l in G.impl_declares:
            print(l, file=Out)
        print("", file=Out)
        print("#endif /* MPIR_IMPL_H_INCLUDED */", file=Out)

def get_qmpi_decl_from_func_decl(func_decl, kind=""):
    if RE.match(r'(.*) (MPIX?_\w+)\((.*?)\)(.*)', func_decl):
        T, name, args, tail = RE.m.group(1,2,3,4)
    else:
        raise Exception("Bad pattern in declaration %s" % func_decl)

    if args == 'void':
        t = "%s Q%s(QMPI_Context context, int tool_id)" % (T, name)
    else:
        t = "%s Q%s(QMPI_Context context, int tool_id, %s)" % (T, name, args)

    if kind == 'proto':
        while RE.search(r'MPICH_ATTR_POINTER_WITH_TYPE_TAG\((\d+),(\d+)\)(.*)', tail):
            i1, i2, tail = RE.m.group(1, 2, 3)
            t += " MPICH_ATTR_POINTER_WITH_TYPE_TAG(%d,%d)" % (int(i1) + 2, int(i2) + 2)

        t += " MPICH_API_PUBLIC"

    return t

def get_qmpi_typedef_from_func_decl(func_decl):
    if RE.match(r'(.*) (MPIX?_\w+)\((.*?)\)', func_decl):
        T, name, args = RE.m.group(1,2,3)
    else:
        raise Exception("Bad pattern in declaration %s" % func_decl)

    if args == 'void':
        t = "typedef %s (Q%s_t) (QMPI_Context context, int tool_id);" % (T, name)
    else:
        t = "typedef %s (Q%s_t) (QMPI_Context context, int tool_id, %s);" % (T, name, args)
    return t

def need_skip_qmpi(func_name):
    # If this is a large count function, it needs to have the suffix removed since we add that
    # later, but it's not in the G.FUNCS list below
    if func_name.lower()[-2:] == "_c":
        func_name = func_name[:-2]

    # Some of the MPIX_ functions are not in the list because they are functions to be added in a
    # future MPI standard. Internally, we treat them as MPI_ functions and add the X later, but they
    # will be in G.FUNCS without the X.
    no_x_func_name = func_name
    if no_x_func_name.lower()[:4] == "mpix":
        no_x_func_name = "mpi" + no_x_func_name[4:]

    if func_name.lower() in G.FUNCS or no_x_func_name.lower() in G.FUNCS:
        if func_name.lower() in G.FUNCS:
            func = G.FUNCS[func_name.lower()];
        elif no_x_func_name.lower() in G.FUNCS:
            func = G.FUNCS[no_x_func_name.lower()];

        if 'dir' not in func or 'not_implemented' in func:
            return True
        elif re.match(r'MPI_DUP_FN', func['name']):
            return True
        else:
            return False
    else:
        # Warn?
        return True

def dump_mpi_proto_h(f):
    def dump_line(s, tail, Out):
        tlist = split_line_with_break(s, tail, 100)
        for l in tlist:
            print(l, file=Out)
    def dump_proto_line(l, Out):
        if RE.match(r'(.*?\))\s+(MPICH.*)', l):
            s, tail = RE.m.group(1,2)
            dump_line(s, tail + ';', Out)

    # -- sort the prototypes into groups --
    list_a = []  # prototypes the fortran needs
    list_b = []  # tool prototypes
    list_c = []  # large prototypes
    for l in G.mpi_declares:
        if re.match(r'int (MPIX?_T_|MPIX_Grequest_)', l):
            list_b.append(l)
        elif re.match(r'int MPIX?_\w+_c\(', l):
            list_c.append(l)
        else:
            list_a.append(l)

    # -- dump the file --
    print("  --> [%s]" %f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)
        print("#ifndef MPI_PROTO_H_INCLUDED", file=Out)
        print("#define MPI_PROTO_H_INCLUDED", file=Out)
        print("", file=Out)

        # -- mpi prototypes --
        print("/* Begin Prototypes */", file=Out)
        for l in list_a:
            dump_proto_line(l, Out)
        print("/* End Prototypes */", file=Out)
        print("", file=Out)

        # -- tool prototypes --
        for l in list_b:
            dump_proto_line(l, Out)
        print("", file=Out)

        # -- large prototypes --
        for l in list_c:
            dump_proto_line(l, Out)
        print("", file=Out)

        # -- PMPI prototypes --
        for l in G.mpi_declares:
            if re.match(r'int MPI_DUP_FN', l):
                continue
            dump_proto_line(re.sub(' MPI', ' PMPI', l, 1), Out)
        print("", file=Out)

        # -- QMPI function enum --
        # We need this all the time to avoid unknown types
        print("enum QMPI_Functions_enum {", file=Out)
        for l in G.mpi_declares:
            m = re.match(r'[a-zA-Z0-9_]* ([a-zA-Z0-9_]*)\(.*', l);
            func_name = m.group(1);
            if need_skip_qmpi(func_name):
                continue
            print("    " + func_name.upper() + "_T,", file=Out)
        print("    MPI_LAST_FUNC_T", file=Out)
        print("};", file=Out)
        print("", file=Out)

        # -- QMPI prototypes --
        for func_decl in G.mpi_declares:
            m = re.match(r'[a-zA-Z0-9_]* ([a-zA-Z0-9_]*)\(.*', func_decl);
            if need_skip_qmpi(m.group(1)):
                continue
            func_decl = get_qmpi_decl_from_func_decl(func_decl, 'proto')
            dump_proto_line(func_decl, Out)

        print("", file=Out)

        # -- QMPI function typedefs --
        for func_decl in G.mpi_declares:
            m = re.match(r'[a-zA-Z0-9_]* ([a-zA-Z0-9_]*)\(.*', func_decl);
            if need_skip_qmpi(m.group(1)):
                continue
            func_decl = get_qmpi_typedef_from_func_decl(func_decl)
            dump_line(func_decl, '', Out)
        print("", file=Out)

        print("#endif /* MPI_PROTO_H_INCLUDED */", file=Out)

def dump_errnames_txt(f):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_mk:
            print(l, file=Out)
        for l in G.mpi_errnames:
            print(l, file=Out)

def dump_mtest_mpix_h(f):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        print("#ifndef MTEST_MPIX_H_INCLUDED", file=Out)
        print("#define MTEST_MPIX_H_INCLUDED", file=Out)
        print("", file=Out)
        for a in G.mpix_symbols:
            print("#define %s %s" % (a, re.sub(r'MPI_', 'MPIX_', a)), file=Out)
        print("", file=Out)
        print("#endif /* MTEST_MPIX_H_INCLUDED */", file=Out)

def dump_qmpi_register_h(f):
    print("  --> [%s]" %f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)
        print("#ifndef QMPI_REGISTER_H_INCLUDED", file=Out)
        print("#define QMPI_REGISTER_H_INCLUDED", file=Out)
        print("", file=Out)
        print("#ifdef ENABLE_QMPI", file=Out)
        print("", file=Out)
        print("static inline int MPII_qmpi_register_internal_functions(void)", file=Out)
        print("{", file=Out)
        for l in G.mpi_declares:
            m = re.match(r'[a-zA-Z0-9_]* ([a-zA-Z0-9_]*)\(.*', l);
            func_name = m.group(1);
            if need_skip_qmpi(func_name):
                continue
            print("    MPIR_QMPI_pointers[%s_T] = (void (*)(void)) &Q%s;" % (func_name.upper(), func_name), file=Out)
        print("", file=Out)
        print("    return MPI_SUCCESS;", file=Out)
        print("}", file=Out)
        print("", file=Out)
        print("#endif /* ENABLE_QMPI */", file=Out)
        print("", file=Out)
        print("#endif /* QMPI_REGISTER_H_INCLUDED */", file=Out)

# ---- pre-processing  ----

def check_func_directives(func):
    # add default to ease the check later
    if 'skip' not in func:
        func['skip'] = ""
    if 'extra' not in func:
        func['extra'] = ""

    if func['dir'] == "mpit":
        func['_skip_Fortran'] = 1

    if RE.search(r'ThreadSafe', func['skip'], re.IGNORECASE):
        func['_skip_ThreadSafe'] = 1
    if RE.search(r'Fortran', func['skip'], re.IGNORECASE):
        func['_skip_Fortran'] = 1
    if RE.search(r'(global_cs|initcheck)', func['skip'], re.IGNORECASE):
        func['_skip_global_cs'] = 1
    if RE.search(r'initcheck', func['skip'], re.IGNORECASE):
        func['_skip_initcheck'] = 1
    if RE.search(r'Errors', func['skip'], re.IGNORECASE):
        func['_skip_err_codes'] = 1

    if RE.search(r'ignore_revoked_comm', func['extra'], re.IGNORECASE):
        func['_comm_valid_ptr_flag'] = 'TRUE'
    else:
        func['_comm_valid_ptr_flag'] = 'FALSE'

    if RE.search(r'errtest_comm_intra', func['extra'], re.IGNORECASE):
        func['_errtest_comm_intra'] = 1

    func['_skip_validate'] = {}
    for a in re.findall(r'validate-(\w+)', func['skip']):
        func['_skip_validate'][a] = 1
    if 'code-error_check-tail' in func:
        for a in func['code-error_check-tail']:
            func['_skip_validate'][a] = 1

    # additional docnotes
    func['_docnotes'] = []
    if 'docnotes' in func:
        func['_docnotes'] = func['docnotes'].replace(' ', '').split(',')
        if RE.search(r'SignalSafe', func['docnotes'], re.IGNORECASE):
            print("Function %s is declared \"SignalSafe\", consider switch to `.skip: global_cs`" % func['name'], file=sys.stderr)

        if RE.search(r'(NotThreadSafe|ThreadSafeNoUpdate)', func['docnotes']):
            func['_skip_ThreadSafe'] = 1
        if RE.search(r'(NotThreadSafe)', func['docnotes']):
            func['_skip_global_cs'] = 1

    if not '_skip_ThreadSafe' in func:
        func['_docnotes'].append('ThreadSafe')
    if not '_skip_Fortran' in func:
        func['_docnotes'].append('Fortran')

    # additional error codes
    if 'errorcodes' in func:
        for a in func['errorcodes'].replace(' ', '').split(','):
            G.err_codes[a] = 1

    # additional exit_routines (clean up)
    if 'code-clean_up' not in func:
        func['code-clean_up'] = []

    # cleanup internal states
    func.pop('_got_comm_size', None)
    func.pop('_got_topo_size', None)

def filter_c_parameters(func):
    if "c_parameters" not in func:
        c_params = []
        for p in func['parameters']:
            if RE.search(r'c_parameter', p['suppress']):
                pass
            else:
                c_params.append(p)
        func['c_parameters'] = c_params

def check_params_with_large_only(func):
    if '_has_large_only' not in func:
        func['_has_large_only'] = 0
        for p in func['c_parameters']:
            if p['large_only']:
                func['_has_large_only'] += 1
        if func['_has_large_only']:
            func['params_large'] = func['c_parameters']
            func['params_small'] = []
            for p in func['c_parameters']:
                if not p['large_only']:
                    func['params_small'].append(p)
    if func['_has_large_only']:
        if not func['_is_large']:
            func['c_parameters'] = func['params_small']
        else:
            func['c_parameters'] = func['params_large']

def process_func_parameters(func):
    """ Scan parameters and populate a few lists to facilitate generation."""
    # Note: we'll attach the lists to func at the end
    validation_list, handle_ptr_list, impl_arg_list, impl_param_list = [], [], [], []
    pointertag_list = []  # needed to annotate MPICH_ATTR_POINTER_WITH_TYPE_TAG

    # init to empty list or we will have double entries due to being called twice (small and large)
    func['_has_handle_out'] = []

    func_name = func['name']
    n = len(func['c_parameters'])
    i = 0
    while i < n:
        p = func['c_parameters'][i]
        (group_kind, group_count) = ("", 0)
        if i + 3 <= n and RE.search(r'BUFFER', p['kind']):
            group_kind, group_count = get_userbuffer_group(func_name, func['c_parameters'], i)
        if group_count > 0:
            t = ''
            for j in range(group_count):
                temp_p = func['c_parameters'][i + j]
                if t:
                    t += ","
                t += temp_p['name']
                impl_arg_list.append(temp_p['name'])
                impl_param_list.append(get_impl_param(func, temp_p))
            validation_list.append({'kind': group_kind, 'name': t})
            # -- pointertag_list
            if re.search(r'alltoallw', func_name, re.IGNORECASE):
                pass
            elif group_count == 3:
                pointertag_list.append("%d,%d" % (i + 1, i + 3))
            elif group_count == 4:
                pointertag_list.append("%d,%d" % (i + 1, i + 4))
            elif group_count == 5:
                pointertag_list.append("%d,%d" % (i + 1, i + 4))
                pointertag_list.append("%d,%d" % (i + 2, i + 4))
            # -- skip to next
            i += group_count
            continue

        do_handle_ptr = 0
        (kind, name) = (p['kind'], p['name'])
        if '_has_comm' not in func and kind == "COMMUNICATOR" and p['param_direction'] == 'in':
            func['_has_comm'] = name
        elif name == "win":
            func['_has_win'] = name
        elif name == "session":
            func['_has_session'] = name

        if 'ANY' in func['_skip_validate'] or kind in func['_skip_validate'] or name in func['_skip_validate']:
            # -- user bypass --
            pass
        elif p['param_direction'] == 'out':
            # -- output parameter --
            if p['kind'] == 'STATUS':
                if p['length']:
                    length = p['length']
                    if length == '*':
                        if RE.match(r'MPI_(Test|Wait)all', func_name, re.IGNORECASE):
                            length = "count"
                        elif RE.match(r'MPI_(Test|Wait)some', func_name, re.IGNORECASE):
                            length = "incount"
                        else:
                            raise Exception("Unexpected")
                    validation_list.append({'kind': "STATUS-length", 'name': name, 'length': length})
                else:
                    validation_list.append({'kind': "STATUS", 'name': name})
            elif p['length']:
                validation_list.append({'kind': "ARGNULL-length", 'name': name, 'length': p['length']})
            else:
                validation_list.append({'kind': "ARGNULL", 'name': name})
            if RE.search(r'(get_errhandler|mpi_comm_get_parent)$', func_name, re.IGNORECASE):
                # we may get the built-in handler, which doesn't have pointer
                pass
            elif kind == "GREQUEST_CLASS" or kind == "DATATYPE":
                pass
            elif kind in G.handle_out_do_ptrs:
                do_handle_ptr = 2
        elif p['length']:
            # -- array parameter --
            if kind == "REQUEST":
                if RE.match(r'mpi_startall', func_name, re.IGNORECASE):
                    do_handle_ptr = 3
                elif RE.match(r'mpix?_(wait|test)', func_name, re.IGNORECASE):
                    do_handle_ptr = 3
            elif kind == "RANK":
                validation_list.append({'kind': "RANK-ARRAY", 'name': name})
            elif RE.match(r'\w+$', p['length']):
                if RE.match(r'(POLY)?DTYPE_NUM_ELEM_NNI', kind):
                    validation_list.append({'kind':"COUNT-ARRAY", 'name': name, 'length': p['length']})
                elif RE.match(r'DATATYPE', kind):
                    validation_list.append({'kind':"TYPE-ARRAY", 'name': name, 'length': p['length']})
                elif RE.match(r'(POLY)?DISPLACEMENT.*_COUNT', kind):
                    validation_list.append({'kind':"DISP-ARRAY", 'name': name, 'length': p['length']})
                else:
                    # FIXME
                    pass
            else:
                # FIXME
                pass
        elif kind == "DATATYPE":
            if RE.match(r'mpi_type_(get|set|delete)_attr|mpi_type_(set_name|get_name|lb|ub|extent)', func_name, re.IGNORECASE):
                do_handle_ptr = 1
            else:
                if is_pointer_type(p):
                    validation_list.append({'kind': "datatype_and_ptr", 'name': '*' + name})
                else:
                    validation_list.append({'kind': "datatype_and_ptr", 'name': name})
        elif kind == "OPERATION":
            if RE.match(r'mpi_op_(free)', func_name, re.IGNORECASE):
                do_handle_ptr = 1
            elif RE.match(r'mpi_r?accumulate', func_name, re.IGNORECASE):
                validation_list.append({'kind': "OP_ACC", 'name': name})
            elif RE.match(r'mpi_(r?get_accumulate|fetch_and_op)', func_name, re.IGNORECASE):
                validation_list.append({'kind': "OP_GACC", 'name': name})
            elif RE.match(r'mpi_op_(commutative)', func_name, re.IGNORECASE):
                validation_list.append({'kind': "op_ptr", 'name': name})
            else:
                validation_list.append({'kind': "op_and_ptr", 'name': name})
        elif kind == "MESSAGE" and p['param_direction'] == 'inout':
            do_handle_ptr = 1
        elif kind == "KEYVAL":
            if RE.search(r'_(set_attr|delete_attr|free_keyval)$', func_name):
                do_handle_ptr = 1
            if is_pointer_type(p):
                validation_list.append({'kind': "KEYVAL", 'name': '*' + name})
            else:
                validation_list.append({'kind': "KEYVAL", 'name': name})
        elif kind == "GREQUEST_CLASS":
            # skip validation for now
            pass
        elif kind in G.handle_mpir_types:
            do_handle_ptr = 1
            if kind == "INFO" and not RE.match(r'mpi_(info_.*|.*_set_info)$', func_name, re.IGNORECASE):
                p['can_be_null'] = "MPI_INFO_NULL"
            elif kind == "REQUEST" and RE.match(r'mpix?_(wait|test|request_get_status|parrived)', func_name, re.IGNORECASE):
                p['can_be_null'] = "MPI_REQUEST_NULL"
            elif kind == "STREAM" and RE.match(r'mpix?_stream_(comm_create|progress)', func_name, re.IGNORECASE):
                p['can_be_null'] = "MPIX_STREAM_NULL"
        elif kind == "RANK" and name == "root":
            validation_list.insert(0, {'kind': "ROOT", 'name': name})
        elif RE.match(r'(COUNT|TAG)$', kind):
            validation_list.append({'kind': RE.m.group(1), 'name': name})
        elif RE.match(r'RANK(_NNI)?$', kind):
            if RE.match(r'mpi_intercomm_create_from_groups', func_name, re.IGNORECASE):
                # TODO: add validation
                pass
            else:
                validation_list.append({'kind': 'RANK', 'name': name})
        elif RE.match(r'(POLY)?(XFER_NUM_ELEM|DTYPE_NUM_ELEM_NNI|DTYPE_PACK_SIZE)', kind):
            validation_list.append({'kind': "COUNT", 'name': name})
        elif RE.match(r'(POLY)?DTYPE_NUM_ELEM', kind):
            if name != 'stride': # NOTE: fragile
                validation_list.append({'kind': "COUNT", 'name': name})
        elif RE.match(r'WINDOW_SIZE|WIN_ATTACH_SIZE', kind):
            validation_list.append({'kind': "WIN_SIZE", 'name': name})
        elif RE.match(r'(ALLOC_MEM_NUM_BYTES|(POLY)?NUM_PARAM_VALUES)', kind):
            validation_list.append({'kind': "ARGNEG", 'name': name})
        elif RE.match(r'(C_)?BUFFER', kind) and RE.match(r'MPI_Win_(allocate|create|attach)', func_name):
            validation_list.append({'kind': "WINBUFFER", 'name': name})
        elif RE.match(r'(POLY)?(RMA_DISPLACEMENT)', kind):
            if name == 'disp_unit':
                validation_list.append({'kind': "WIN_DISPUNIT", 'name': name})
            else:
                validation_list.append({'kind': "RMADISP", 'name': name})
        elif RE.match(r'(.*_NNI|ARRAY_LENGTH|INFO_VALUE_LENGTH|KEY_INDEX|INDEX|NUM_DIMS|DIMENSION|COMM_SIZE|DISPLACEMENT_COUNT)', kind):
            if p['param_direction'] == 'inout':
                validation_list.append({'kind': "ARGNULL", 'name': name})
                validation_list.append({'kind': "ARGNEG", 'name': "*" + name})
            elif RE.search(r'count', name):
                validation_list.append({'kind': "COUNT", 'name': name})
            else:
                validation_list.append({'kind': "ARGNEG", 'name': name})
        elif RE.match(r'(.*_PI)', kind):
            validation_list.append({'kind': "ARGNONPOS", 'name': name})
        elif kind == "STRING" and name == "key":
            validation_list.append({'kind': "infokey", 'name': name})
        elif RE.match(r'(CAT|CVAR|PVAR)_INDEX', kind):
            validation_list.append({'kind': "mpit_%s_index" % RE.m.group(1), 'name': name})
        elif RE.match(r'(CVAR|PVAR|TOOLS_ENUM)$', kind):
            if kind == "CVAR" or kind == "PVAR":
                t_kind = "mpit_" + kind.lower() + "_handle"
            else:
                t_kind = "mpit_enum_handle"
            t_name = name
            if RE.search(r'direction=inout', p['t']):
                t_name = '*' + name
            validation_list.append({'kind': t_kind, 'name': t_name})
        elif kind == "TOOLENUM_INDEX":
            # MPI_T_enum_get_item, assume 1st param is enumtype
            validation_list.append({'kind': "mpit_enum_item", 'name': "enumtype, " + name})
        elif kind == "PVAR_SESSION":
            t_name = name
            if RE.search(r'direction=inout', p['t']):
                t_name = '*' + name
            validation_list.append({'kind': "mpit_pvar_session", 'name': t_name})
        elif kind == "PVAR_CLASS":
            validation_list.append({'kind': "mpit_pvar_class", 'name': name})
        elif kind == "EVENT_REGISTRATION":
            validation_list.append({'kind': "mpit_event_registration", 'name': name})
        elif kind == "EVENT_INSTANCE":
            validation_list.append({'kind': "mpit_event_instance", 'name': name})
        elif RE.match(r'(FUNCTION)', kind):
            if RE.match(r'mpi_(keyval_create|\w+_create_keyval)', func_name, re.IGNORECASE):
                # MPI_NULL_COPY_FN etc are defined as NULL
                pass
            else:
                validation_list.append({'kind': "ARGNULL", 'name': name})
        elif RE.match(r'(ERROR_CLASS|ERROR_CODE|FILE|ATTRIBUTE_VAL|EXTRA_STATE|LOGICAL)', kind):
            # no validation for these kinds
            pass
        elif RE.match(r'(POLY)?(DTYPE_STRIDE_BYTES|DISPLACEMENT_AINT_COUNT)$', kind):
            # e.g. stride in MPI_Type_vector, MPI_Type_create_resized
            pass
        elif is_pointer_type(p):
            validation_list.append({'kind': "ARGNULL", 'name': name})
        else:
            print("Missing error checking: func=%s, name=%s, kind=%s" % (func_name, name, kind), file=sys.stderr)

        if do_handle_ptr == 1:
            if p['param_direction'] == 'inout':
                # assume only one such parameter
                func['_has_handle_inout'] = p
            if (p['param_direction'] == 'inout' or p['pointer']) and not p['length']:
                p['_pointer'] = 1
            handle_ptr_list.append(p)
            impl_arg_list.append(name + "_ptr")
            impl_param_list.append("%s *%s_ptr" % (G.handle_mpir_types[kind], name))
        elif do_handle_ptr == 2:
            # Just MPI_Comm_idup[_with_info] have 2 handle output, but use a list anyway
            func['_has_handle_out'].append(p)

            impl_arg_list.append('&' + name + "_ptr")
            impl_param_list.append("%s **%s_ptr" % (G.handle_mpir_types[kind], name))
        elif do_handle_ptr == 3:
            # arrays
            handle_ptr_list.append(p)
            if kind == "REQUEST":
                ptrs_name = "request_ptrs"
                p['_ptrs_name'] = ptrs_name
                if RE.match(r'mpi_startall', func['name'], re.IGNORECASE):
                    impl_arg_list.append(ptrs_name)
                    impl_param_list.append("MPIR_Request **%s" % ptrs_name)
                else:
                    impl_arg_list.append(name)
                    impl_param_list.append("MPI_Request %s[]" % name)
            else:
                print("Unhandled handle array: " + name, file=sys.stderr)
        elif "code-handle_ptr-tail" in func and name in func['code-handle_ptr-tail']:
            mpir_type = G.handle_mpir_types[kind]
            if p['length']:
                impl_arg_list.append(name + "_ptrs")
                impl_param_list.append("%s **%s_ptrs" % (mpir_type, name))
            else:
                impl_arg_list.append(name + "_ptr")
                impl_param_list.append("%s *%s_ptr" % (mpir_type, name))
        else:
            impl_arg_list.append(name)
            impl_param_list.append(get_impl_param(func, p))
        i += 1

    if RE.match(r'MPI_(Wait|Test)$', func_name):
        func['_has_comm'] = "comm"
        func['_comm_from_request'] = 1

    func['_need_validation'] = 0
    if len(validation_list):
        func['_validation_list'] = validation_list
        func['_need_validation'] = 1
    if len(handle_ptr_list):
        func['_handle_ptr_list'] = handle_ptr_list
        func['_need_validation'] = 1
    if 'code-error_check' in func:
        func['_need_validation'] = 1
    func['_impl_arg_list'] = impl_arg_list
    func['_impl_param_list'] = impl_param_list
    if len(pointertag_list):
        func['_pointertag_list'] = pointertag_list

# ---- simple parts ----

def dump_copy_right():
    G.out.append("/*")
    G.out.append(" * Copyright (C) by Argonne National Laboratory")
    G.out.append(" *     See COPYRIGHT in top-level directory")
    G.out.append(" */")
    G.out.append("")

def dump_qmpi_wrappers(func, is_large):
    parameters = ""
    for p in func['c_parameters']:
        parameters = parameters + ", " + p['name']

    func_name = get_function_name(func, is_large)
    func_decl = get_declare_function(func, is_large)
    qmpi_decl = get_qmpi_decl_from_func_decl(func_decl)

    static_call = get_static_call_internal(func, is_large)

    G.out.append("#ifdef ENABLE_QMPI")
    G.out.append("#ifndef MPICH_MPI_FROM_PMPI")
    dump_line_with_break(qmpi_decl)
    G.out.append("{")
    if func_name == "MPI_Pcontrol":
        G.out.append("    va_list varargs;")
        G.out.append("    va_start(varargs, level);")
        G.out.append("")
    G.out.append("    return " + static_call + ";")
    G.out.append("}")
    G.out.append("#endif /* MPICH_MPI_FROM_PMPI */")

    dump_line_with_break(func_decl)
    G.out.append("{")
    G.out.append("    QMPI_Context context;")
    G.out.append("    Q%s_t *fn_ptr;" % (func_name))
    G.out.append("")
    G.out.append("    context.storage_stack = NULL;")
    G.out.append("")
    if func_name == "MPI_Pcontrol":
        G.out.append("    va_list varargs;")
        G.out.append("    va_start(varargs, level);")
        G.out.append("")
    if "initcheck" in func['skip']:
        G.out.append("    int mpi_errno = MPI_SUCCESS;")
        G.out.append("    mpi_errno = MPII_qmpi_init();")
        G.out.append("    if (mpi_errno != MPI_SUCCESS) {")
        G.out.append("        return mpi_errno;")
        G.out.append("    }")
        G.out.append("")
    G.out.append("    if (MPIR_QMPI_num_tools == 0)")
    dump_line_with_break("        return Q%s(context, 0%s);" % (func_name, parameters))
    G.out.append("")
    dump_line_with_break("    fn_ptr = (Q%s_t *) MPIR_QMPI_first_fn_ptrs[%s_T];" % (func_name, func_name.upper()))
    G.out.append("")
    dump_line_with_break("    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[%s_T]%s);" % (func_name.upper(), parameters));
    G.out.append("}")
    G.out.append("#else /* ENABLE_QMPI */")

    G.out.append("")

    dump_line_with_break(func_decl)
    G.out.append("{")
    if func_name == "MPI_Pcontrol":
        G.out.append("    va_list varargs;")
        G.out.append("    va_start(varargs, level);")
        G.out.append("")
    G.out.append("    return " + static_call + ";")
    G.out.append("}")
    G.out.append("#endif /* ENABLE_QMPI */")

def dump_profiling(func):
    func_name = get_function_name(func, func['_is_large'])
    G.out.append("/* -- Begin Profiling Symbol Block for routine %s */" % func_name)
    G.out.append("#if defined(HAVE_PRAGMA_WEAK)")
    G.out.append("#pragma weak %s = P%s" % (func_name, func_name))
    G.out.append("#elif defined(HAVE_PRAGMA_HP_SEC_DEF)")
    G.out.append("#pragma _HP_SECONDARY_DEF P%s  %s" % (func_name, func_name))
    G.out.append("#elif defined(HAVE_PRAGMA_CRI_DUP)")
    G.out.append("#pragma _CRI duplicate %s as P%s" % (func_name, func_name))
    G.out.append("#elif defined(HAVE_WEAK_ATTRIBUTE)")
    s = get_declare_function(func, func['_is_large'])
    dump_line_with_break(s, " __attribute__ ((weak, alias(\"P%s\")));" % (func_name))
    G.out.append("#endif")
    G.out.append("/* -- End Profiling Symbol Block */")

    G.out.append("")
    G.out.append("/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build")
    G.out.append("   the MPI routines */")
    G.out.append("#ifndef MPICH_MPI_FROM_PMPI")
    G.out.append("#undef %s" % func_name)
    G.out.append("#define %s P%s" % (func_name, func_name))
    G.out.append("#endif /* MPICH_MPI_FROM_PMPI */")
    G.out.append("")

def dump_manpage(func, out):
    def dump_description(s):
        words = s.split()
        n = len(words)
        i0 = 0
        i = 0
        l = 0
        for w in words:
            if l == 0:
                l += len(w)
            else:
                l += len(w) + 1
                if l > 80:
                    out.append('  ' + ' '.join(words[i0:i]))
                    i0 = i
                    l = len(w)
            i += 1
            continue
        if l > 0:
            out.append('  ' + ' '.join(words[i0:]))
    # ----
    if not func['desc']:
        # place holder to make the man page render
        func['desc'] = "[short description]"
    out.append("/*D")
    out.append("   %s - %s" % (get_function_name(func, False), func['desc']))
    out.append("")

    # Synopsis
    out.append("Synopsis:")
    func_decl = get_declare_function(func, False)
    tlist = split_line_with_break(func_decl, '', 80)
    out.append(".vb")
    out.extend(tlist)
    out.append(".ve")
    if func['_has_poly']:
        func_decl = get_declare_function(func, True)
        tlist = split_line_with_break(func_decl, '', 80)
        out.append(".vb")
        out.extend(tlist)
        out.append(".ve")
    out.append("")

    lis_map = G.MAPS['LIS_KIND_MAP']
    for p in func['c_parameters']:
        lis_type = lis_map[p['kind']]
        if 'desc' not in p:
            if p['kind'] in G.default_descriptions:
                p['desc'] = G.default_descriptions[p['kind']]
            else:
                p['desc'] = p['name']
        if not re.search(r'\)$', p['desc']):
            p['desc'] += " (%s)" % (lis_type)

    input_list, output_list, inout_list = [], [], []
    for p in func['c_parameters']:
        if p['param_direction'] == 'out':
            output_list.append(p)
        elif p['param_direction'] == 'inout':
            inout_list.append(p)
        else:
            input_list.append(p)
    dump_manpage_list(input_list, "Input Parameters", out)
    dump_manpage_list(inout_list, "Input/Output Parameters", out)
    dump_manpage_list(output_list, "Output Parameters", out)

    # Add the custom notes (specified in e.g. pt2pt_api.txt) as is.
    if 'notes' in func:
        for l in func['notes']:
            out.append(l)
        out.append("")

    if 'replace' in func:
        if RE.match(r'\s*(deprecated|removed)', func['replace'], re.IGNORECASE):
            out.append(".N %s" % RE.m.group(1).capitalize())
        else:
            print("Missing reasons in %s .replace" % func['name'], file=sys.stderr)

        if RE.search(r'with\s+(\w+)', func['replace']):
            out.append("   The replacement for this routine is '%s'." % RE.m.group(1))
        out.append("")

    # document info keys
    if G.hints and func['name'] in G.hints:
        print("Got info hints in %s" % func['name'])
        out.append("Info hints:")
        for a in G.hints[func['name']]:
            out.append(". %s - %s, default = %s." % (a['name'], a['type'], a['default']))
            dump_description(a['description'])
        out.append("")

    for note in func['_docnotes']:
        out.append(".N %s" % note)
        if note == "Fortran":
            has = {}
            for p in func['c_parameters']:
                if p['kind'] == "status":
                    if p['length']:
                        has['FortStatusArray'] = 1
                    else:
                        has['FortranStatus'] = 1
            for k in has:
                out.append(".N %s" % k)
        out.append("")

    if 'notes2' in func:
        for l in func['notes2']:
            out.append(l)
        out.append("")

    if '_skip_err_codes' not in func:
        out.append(".N Errors")
        out.append(".N MPI_SUCCESS")
        for err in sorted (G.err_codes.keys()):
            out.append(".N %s" % (err))
        out.append(".N MPI_ERR_OTHER")
        out.append("")
    if 'seealso' in func:
        out.append(".seealso: %s" % func['seealso'])
    out.append("D*/")
    out.append("")

def dump_manpage_list(list, header, out):
    count = len(list)
    if count == 0:
        return
    out.append("%s:" % header)
    if count == 1:
        p = list[0]
        out.append(". %s - %s" % (p['name'], p['desc']))
    else:
        for i, p in enumerate(list):
            lead = "."
            if i == 0:
                lead = "+"
            elif i == count - 1:
                lead = "-"
            out.append("%s %s - %s" % (lead, p['name'], p['desc']))
    out.append("")

def get_function_internal_prototype(func_decl):
    func_decl = re.sub(r'MPI(X?)_([a-zA-Z0-9_]*\()', r'internal\1_\2', func_decl, 1)
    func_decl = "static " + func_decl
    return func_decl

# ---- the function part ----
def dump_function_internal(func, kind):
    """Appends to G.out array the MPI function implementation."""
    func_name = get_function_name(func, func['_is_large']);

    s = get_declare_function(func, func['_is_large'])
    if kind == "polymorph":
        (extra_param, extra_arg) = get_polymorph_param_and_arg(func['polymorph'])
        s = re.sub(r'MPI(X?)_([a-zA-Z0-9_]*\()', r'MPI\1I_\2', s, 1)
        s = re.sub(r'\)$', ', '+extra_param+')', s)
        # prepare for the latter body of routines calling MPIR impl
        RE.search(r'(\w+)$', extra_param)
        func['_impl_arg_list'].append(RE.m.group(1))
        func['_impl_param_list'].append(extra_param)
    else:
        G.out.append("")
        s = get_function_internal_prototype(s)

    dump_line_with_break(s)
    G.out.append("{")
    G.out.append("INDENT")

    if "impl" in func and func['impl'] == "direct":
        # e.g. MPI_Aint_add
        dump_function_direct(func)
    elif kind == 'call-polymorph':
        (extra_param, extra_arg) = get_polymorph_param_and_arg(func['polymorph'])
        repl_name = re.sub(r'MPI(X?)_', r'MPI\1I_', func_name, 1)
        repl_args = get_function_args(func) + ', ' + extra_arg
        repl_call = "mpi_errno = %s(%s);" % (repl_name, repl_args)
        dump_function_replace(func, repl_call)
    else:
        dump_function_normal(func)

    G.out.append("DEDENT")
    G.out.append("}")

    if func['_has_poly'] and func['_poly_impl'] != "separate" and func['_is_large']:
        pass
    else:
        if "decl" in func:
            push_impl_decl(func, func['decl'])
        elif 'impl' in func:
            if RE.match(r'topo_fns->', func['impl']):
                push_impl_decl(func)
        elif 'body' in func:
            pass
        else:
            push_impl_decl(func)

def declare_call_replace_internal(func):
    m = re.search(r'with\s+(MPI_\w+)', func['replace'])
    repl_name = m.group(1).lower()
    if repl_name not in G.FUNCS:
        raise Exception("Replacement function %s not found!" % repl_name)
    repl_func = G.FUNCS[repl_name]
    s = get_declare_function(repl_func, func['_is_large'])
    s = "static " + re.sub(r'MPI(X?)_', r'internal_', s, 1)

    G.out.append("")
    dump_line_with_break(s + ';')

# used in dump_qmpi_wrappers, call the internal function
def get_static_call_internal(func, is_large):
    func_name = get_function_name(func, is_large)
    if 'replace' in func and 'body' not in func:
        m = re.search(r'with\s+(MPI_\w+)', func['replace'])
        func_name = m.group(1)
    static_call = re.sub(r'MPI(X?)_', r'internal\1_', func_name, 1)
    static_call = static_call + "(" + get_function_args(func) + ")"
    return static_call

def check_large_parameters(func):
    if not func['_has_poly']:
        func['_poly_impl'] = "separate"
        return

    # Set func['_poly_impl']
    if 'poly_impl' in func:
        # always prefer explicit .poly_impl directive
        func['_poly_impl'] = func['poly_impl']
    elif 'code-large_count' in func:
        # { -- large_count } code block exist
        func['_poly_impl'] = "separate"
    elif func['dir'] == 'datatype':
        func['_poly_impl'] = "separate"
    else:
        func['_poly_impl'] = "use-aint"

    # Gather large parameters. Potentially we need copy in & out
    func['_poly_in_list'] = []
    func['_poly_in_arrays'] = []
    func['_poly_out_list'] = []
    func['_poly_inout_list'] = []
    func['_poly_need_filter'] = False
    for p in func['c_parameters']:
        if RE.match(r'POLY', p['kind']):
            if p['param_direction'] == 'out':
                func['_poly_out_list'].append(p)
                func['_poly_need_filter'] = True
            elif p['param_direction'] == 'inout':
                # MPI_{Pack,Unpack}[_external]
                func['_poly_inout_list'].append(p)
                func['_poly_need_filter'] = True
            elif p['length']:
                func['_poly_in_arrays'].append(p)
                func['_poly_need_filter'] = True
            elif p['kind'] == "POLYFUNCTION":
                # we'll have separate versions that doesn't need cast or swap
                pass
            else:
                func['_poly_in_list'].append(p)

def dump_function_normal(func):
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    if '_handle_ptr_list' in func:
        for p in func['_handle_ptr_list']:
            dump_handle_ptr_var(func, p)
    if '_comm_from_request' in func:
        G.out.append("MPIR_Comm *comm_ptr = NULL;")
    if 'code-declare' in func:
        for l in func['code-declare']:
            G.out.append(l)

    if not '_skip_initcheck' in func:
        G.out.append("")
        if func['dir'] == "mpit":
            G.err_codes['MPI_T_ERR_NOT_INITIALIZED'] = 1
            G.out.append("MPIT_ERRTEST_MPIT_INITIALIZED();")
        else:
            G.out.append("MPIR_ERRTEST_INITIALIZED_ORDIE();")

    if not '_skip_global_cs' in func:
        G.out.append("")
        if func['dir'] == 'mpit':
            G.out.append("MPIR_T_THREAD_CS_ENTER();")
        else:
            G.out.append("MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);")
    G.out.append("MPIR_FUNC_TERSE_ENTER;")

    if '_handle_ptr_list' in func:
        G.out.append("")
        G.out.append("#ifdef HAVE_ERROR_CHECKING")
        G.out.append("{")
        G.out.append("    MPID_BEGIN_ERROR_CHECKS;")
        G.out.append("    {")
        G.out.append("INDENT")
        G.out.append("INDENT")
        for p in func['_handle_ptr_list']:
            dump_validate_handle(func, p)
        G.out.append("DEDENT")
        G.out.append("DEDENT")
        G.out.append("    }")
        G.out.append("    MPID_END_ERROR_CHECKS;")
        G.out.append("}")
        G.out.append("#endif \x2f* HAVE_ERROR_CHECKING */")
        G.out.append("")
        for p in func['_handle_ptr_list']:
            dump_convert_handle(func, p)
    if 'code-handle_ptr' in func:
        for l in func['code-handle_ptr']:
            G.out.append(l)
    if func['_need_validation']:
        G.out.append("")
        G.out.append("#ifdef HAVE_ERROR_CHECKING")
        G.out.append("{")
        G.out.append("    MPID_BEGIN_ERROR_CHECKS;")
        G.out.append("    {")
        G.out.append("INDENT")
        G.out.append("INDENT")
        if '_handle_ptr_list' in func:
            for p in func['_handle_ptr_list']:
                dump_validate_handle_ptr(func, p)
        if '_validation_list' in func:
            for t in func['_validation_list']:
                dump_validation(func, t)
        if 'code-error_check' in func:
            for l in func['code-error_check']:
                if RE.match(r'CHECK(ENUM|MASK):\s*(\w+),\s*(\w+),\s*(.+)', l):
                    dump_CHECKENUM(RE.m.group(2), RE.m.group(3), RE.m.group(4), RE.m.group(1))
                else:
                    G.out.append(l)
        G.out.append("DEDENT")
        G.out.append("DEDENT")
        G.out.append("    }")
        G.out.append("    MPID_END_ERROR_CHECKS;")
        G.out.append("}")
        G.out.append("#endif \x2f* HAVE_ERROR_CHECKING */")
        G.out.append("")

    check_early_returns(func)
    G.out.append("")

    # ----
    def dump_body_of_routine():
        if RE.search(r'threadcomm', func['extra'], re.IGNORECASE):
            G.out.append("#ifdef ENABLE_THREADCOMM")
            dump_if_open("comm_ptr->threadcomm")
            dump_body_threadcomm(func)
            G.out.append("goto fn_exit;")
            dump_if_close()
            G.out.append("#endif")

        if 'body' in func:
            if func['_is_large'] and func['_poly_impl'] == "separate":
                if 'code-large_count' not in func:
                    raise Exception("%s missing large count code block." % func['name'])
                for l in func['code-large_count']:
                    G.out.append(l)
            else:
                for l in func['body']:
                    G.out.append(l)
        elif 'impl' in func:
            if RE.match(r'mpid', func['impl'], re.IGNORECASE):
                dump_body_impl(func, "mpid")
            elif RE.match(r'topo_fns->(\w+)', func['impl'], re.IGNORECASE):
                dump_body_topo_fns(func, RE.m.group(1))
            else:
                print("Error: unhandled special impl: [%s]" % func['impl'])
        elif func['dir'] == 'coll':
            dump_body_coll(func)
        else:
            dump_body_impl(func, "mpir")

    # ----
    G.out.append("/* ... body of routine ... */")

    if func['_is_large'] and 'code-large_count' not in func:
        # BIG but internally is using MPI_Aint
        impl_args_save = copy.copy(func['_impl_arg_list'])

        dump_if_open("sizeof(MPI_Count) == sizeof(MPI_Aint)")
        # Same size, just casting the _impl_arg_list
        add_poly_impl_cast(func)
        dump_body_of_routine()

        dump_else()
        # Need filter to check limits and potentially swap args
        func['_impl_arg_list'] = copy.copy(impl_args_save)
        G.out.append("/* MPI_Count is bigger than MPI_Aint */")
        dump_poly_pre_filter(func)
        dump_body_of_routine()
        dump_poly_post_filter(func)

        dump_if_close()

    elif not func['_is_large'] and func['_has_poly'] and func['_poly_impl'] != "separate":
        # SMALL but internally is using MPI_Aint
        dump_poly_pre_filter(func)
        dump_body_of_routine()
        dump_poly_post_filter(func)

    else:
        # normal
        dump_body_of_routine()

    G.out.append("/* ... end of body of routine ... */")

    # ----
    G.out.append("")
    G.out.append("fn_exit:")
    for l in func['code-clean_up']:
        G.out.append(l)
    func['code-clean_up'] = []
    G.out.append("MPIR_FUNC_TERSE_EXIT;")

    if not '_skip_global_cs' in func:
        if func['dir'] == 'mpit':
            G.out.append("MPIR_T_THREAD_CS_EXIT();")
        else:
            G.out.append("MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);")
    G.out.append("return mpi_errno;")
    G.out.append("")
    G.out.append("fn_fail:")
    if func['dir'] == 'mpit':
        # MPI_T always return the mpi_errno
        pass
    else:
        dump_mpi_fn_fail(func)
    G.out.append("goto fn_exit;")

def replace_impl_arg_list(arg_list, old, new):
    for i in range(len(arg_list)):
        if arg_list[i] == old:
                arg_list[i] = new

def check_aint_fits(is_large, val):
    # only need check when swapping `MPI_Count` to `MPI_Aint`
    if is_large:
        dump_if_open("%s > MPIR_AINT_MAX" % val)
        dump_error("too_big_for_input", "%s", '"%s"' % val)
        dump_if_close()

def add_poly_impl_cast(func):
    for p in func['_poly_in_list']:
        replace_impl_arg_list(func['_impl_arg_list'], p['name'], "(MPI_Aint) " + p['name'])
    for p in func['_poly_inout_list'] + func['_poly_out_list'] + func['_poly_in_arrays']:
        replace_impl_arg_list(func['_impl_arg_list'], p['name'], "(MPI_Aint *) " + p['name'])

def check_poly_is_aint(func, p):
    if func['_is_large']:
        return G.MAPS['BIG_C_KIND_MAP'][p['kind']] == "MPI_Aint"
    else:
        return G.MAPS['SMALL_C_KIND_MAP'][p['kind']] == "MPI_Aint"

def dump_poly_pre_filter(func):
    def filter_output():
        # internal code will use MPI_Aint
        for p in func['_poly_out_list']:
            if not check_poly_is_aint(func, p):
                G.out.append("MPI_Aint %s_c;" % p['name'])
        for p in func['_poly_inout_list']:
            if not check_poly_is_aint(func, p):
                G.out.append("MPI_Aint %s_c = *%s;" % (p['name'], p['name']))

        for p in func['_poly_out_list'] + func['_poly_inout_list']:
            if not check_poly_is_aint(func, p):
                replace_impl_arg_list(func['_impl_arg_list'], p['name'], "&%s_c" % p['name'])

    def filter_array():
        if RE.search(r'((all)?gatherv|scatterv|alltoall[vw]|reduce_scatter)(_init)?\b', func['name'], re.IGNORECASE):
            dump_coll_v_swap(func)
        elif RE.search(r'(h?indexed(_block)?|struct|(d|sub)array)', func['name'], re.IGNORECASE):
            dump_type_create_swap(func)
        else:
            if func['_poly_in_arrays']:
                # non-v-collectives should use separate impl
                raise Exception("Unhandled POLY-array function %s" % func['name'])
    # ----
    if func['_is_large']:
        for p in func['_poly_in_list']:
            if not check_poly_is_aint(func, p):
                check_aint_fits(True, p['name'])
        for p in func['_poly_inout_list']:
            if not check_poly_is_aint(func, p):
                check_aint_fits(True, '*' + p['name'])

    filter_output()
    filter_array()

def dump_poly_post_filter(func):
    def filter_output(int_max):
        for p in func['_poly_out_list'] + func['_poly_inout_list']:
            if not check_poly_is_aint(func, p):
                val = p['name'] + "_c"
                if int_max:
                    # Seems in all cases we can return MPI_UNDEFINED rather than
                    # error.
                    if True:
                        dump_if_open("%s > %s" % (val, int_max))
                        G.out.append("*%s = MPI_UNDEFINED;" % p['name'])
                        dump_else()
                        G.out.append("*%s = %s;" % (p['name'], val))
                        dump_if_close()
                    else:
                        dump_if_open("%s > %s" % (val, int_max))
                        dump_error("too_big_for_output", "%s", "\"%s\"" % val)
                        dump_if_close()
                        G.out.append("*%s = %s;" % (p['name'], val))
                else:
                    G.out.append("*%s = %s;" % (p['name'], val))

    def filter_array(int_max):
        if RE.search(r'((all)?gatherv|scatterv|alltoall[vw]|reduce_scatter)(_init)?\b', func['name'], re.IGNORECASE):
            dump_coll_v_exit(func)
        elif RE.search(r'(h?indexed(_block)?|struct|(d|sub)array)', func['name'], re.IGNORECASE):
            dump_type_create_exit(func)

    # ----
    int_max = None
    if not func['_is_large']:
        int_max = "INT_MAX"
    filter_output(int_max)
    filter_array(int_max)

def dump_type_create_swap(func):
    for p in func['_poly_in_arrays']:
        n = "count"
        if RE.search(r'create_(d|sub)array', func['name'], re.IGNORECASE):
            n = "ndims"
        new_name = p['name'] + '_c'
        G.out.append("MPI_Aint *%s = MPL_malloc(%s * sizeof(MPI_Aint), MPL_MEM_OTHER);" % (new_name, n))
        dump_for_open("i", n)
        check_aint_fits(func['_is_large'], "%s[i]" % p['name'])
        G.out.append("%s[i] = %s[i];" % (new_name, p['name']))
        dump_for_close()
        replace_impl_arg_list(func['_impl_arg_list'], p['name'], new_name)

def dump_type_create_exit(func):
    for p in func['_poly_in_arrays']:
        G.out.append("MPL_free(%s_c);" % p['name'])

def push_impl_decl(func, impl_name=None):
    if not impl_name:
        impl_name = re.sub(r'^MPIX?_', 'MPIR_', func['name']) + "_impl"

    if func['_is_large']:
        # add suffix to differentiate
        impl_name = re.sub(r'_impl$', '_large_impl', impl_name)

    if func['_impl_param_list']:
        params = ', '.join(func['_impl_param_list'])
        if func['dir'] == 'coll':
            # block collective use an extra errflag
            if not RE.match(r'MPI_(I.*|Neighbor.*|.*_init)$', func['name']):
                params = params + ", MPIR_Errflag_t errflag"
    else:
        params="void"

    if func['dir'] == 'coll':
        # collective also dump MPIR_Xxx(...)
        mpir_name = re.sub(r'^MPIX?_', 'MPIR_', func['name'])
        G.impl_declares.append("int %s(%s);" % (mpir_name, params))
    # dump MPIR_Xxx_impl(...)
    G.impl_declares.append("int %s(%s);" % (impl_name, params))

def push_threadcomm_impl_decl(func):
    impl_name = re.sub(r'^mpix?_(comm_)?', 'MPIR_Threadcomm_', func['name'].lower())
    impl_name += '_impl'

    params = ', '.join(func['_impl_param_list'])

    G.impl_declares.append("int %s(%s);" % (impl_name, params))

def dump_CHECKENUM(var, errname, t, type="ENUM"):
    val_list = t.split()
    if type == "ENUM":
        cond_list = []
        for val in val_list:
            cond_list.append("%s != %s" % (var, val))
        cond = ' && '.join(cond_list)
    elif type == "MASK":
        cond = "%s != (%s & (%s))" % (var, var, ' | '.join(val_list))
    dump_if_open(cond)
    G.out.append("MPIR_ERR_SET1(mpi_errno, MPI_ERR_ARG, \"**%s\", \"**%s %%d\", %s);" % (errname, errname, var))
    G.out.append("goto fn_fail;")
    dump_if_close()

def dump_body_coll(func):
    # collectives call MPIR_Xxx
    mpir_name = re.sub(r'^MPIX?_', 'MPIR_', func['name'])

    args = ", ".join(func['_impl_arg_list'])

    if RE.match(r'MPI_(I.*|.*_init)$', func['name'], re.IGNORECASE):
        # non-blocking collectives
        G.out.append("MPIR_Request *request_ptr = NULL;")
        dump_line_with_break("mpi_errno = %s(%s);" % (mpir_name, args))
        dump_error_check("")
        G.out.append("if (!request_ptr) {")
        G.out.append("    request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);")
        G.out.append("}")
        G.out.append("*request = request_ptr->handle;")
    elif RE.match(r'mpi_neighbor_', func['name'], re.IGNORECASE):
        dump_line_with_break("mpi_errno = %s(%s);" % (mpir_name, args))
        dump_error_check("")
    else:
        # blocking collectives
        dump_line_with_break("mpi_errno = %s(%s, MPIR_ERR_NONE);" % (mpir_name, args))
        dump_error_check("")

def dump_coll_v_swap(func):
    # -- wrappers to make code cleaner
    def replace_arg(old, new):
        replace_impl_arg_list(func['_impl_arg_list'], old, new)

    def check_fit(val):
        check_aint_fits(func['_is_large'], val)

    # -- swapping routines
    def allocate_tmp_array(n):
        G.out.append("MPI_Aint *tmp_array = MPL_malloc(%s * sizeof(MPI_Aint), MPL_MEM_OTHER);" % n)
    def swap_one(n, counts):
        dump_for_open("i", n)
        check_fit("%s[i]" % counts)
        G.out.append("tmp_array[i] = %s[i];" % counts)
        dump_for_close()
    def swap_next(base, n, counts):
        dump_for_open("i", n)
        check_fit("%s[i]" % counts)
        G.out.append("tmp_array[%s + i] = %s[i];" % (base, counts))
        dump_for_close()

    # -------------------------
    if RE.match(r'mpi_i?neighbor_', func['name'], re.IGNORECASE):
        # neighborhood collectives
        G.out.append("int indegree, outdegree, weighted;")
        G.out.append("mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);")
        if RE.search(r'allgatherv', func['name'], re.IGNORECASE):
            allocate_tmp_array("indegree * 2")
            swap_one("indegree", "recvcounts")
            swap_next("indegree", "indegree", "displs")
            replace_arg('recvcounts', 'tmp_array')
            replace_arg('displs', 'tmp_array + indegree')
        elif RE.search(r'alltoallv', func['name'], re.IGNORECASE):
            allocate_tmp_array("(outdegree + indegree) * 2")
            swap_one("outdegree", "sendcounts")
            swap_next("outdegree", "outdegree", "sdispls")
            swap_next("outdegree * 2", "indegree", "recvcounts")
            swap_next("outdegree * 2 + indegree", "indegree", "rdispls")
            replace_arg('sendcounts', 'tmp_array')
            replace_arg('sdispls', 'tmp_array + outdegree')
            replace_arg('recvcounts', 'tmp_array + outdegree * 2')
            replace_arg('rdispls', 'tmp_array + outdegree * 2 + indegree')
        else: # neighbor_alltoallw
            allocate_tmp_array("(outdegree + indegree)")
            swap_one("outdegree", "sendcounts")
            swap_next("outdegree", "indegree", "recvcounts")
            replace_arg('sendcounts', 'tmp_array')
            replace_arg('recvcounts', 'tmp_array + outdegree')
    # classical collectives
    elif RE.match(r'(mpi_i?reduce_scatter(_init)?\b)', func['name'], re.IGNORECASE):
        G.out.append("int n = comm_ptr->local_size;")
        allocate_tmp_array("n")
        swap_one("n", "recvcounts")
        replace_arg('recvcounts', 'tmp_array')
    else:
        cond = "(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)"
        G.out.append("int n = %s ? comm_ptr->remote_size : comm_ptr->local_size;" % cond)
        if RE.search(r'alltoall[vw]', func['name'], re.IGNORECASE):
            allocate_tmp_array("n * 4")
            dump_if_open("sendbuf != MPI_IN_PLACE")
            swap_one("n", "sendcounts")
            swap_next("n", "n", "sdispls")
            dump_if_close()
            swap_next("n * 2", "n", "recvcounts")
            swap_next("n * 3", "n", "rdispls")
            replace_arg('sendcounts', 'tmp_array')
            replace_arg('sdispls', 'tmp_array + n')
            replace_arg('recvcounts', 'tmp_array + n * 2')
            replace_arg('rdispls', 'tmp_array + n * 3')
        elif RE.search(r'allgatherv', func['name'], re.IGNORECASE):
            allocate_tmp_array("n * 2")
            swap_one("n", "recvcounts")
            swap_next("n", "n", "displs")
            replace_arg('recvcounts', 'tmp_array')
            replace_arg('displs', 'tmp_array + n')
        else:
            allocate_tmp_array("n * 2")
            if RE.search(r'scatterv', func['name'], re.IGNORECASE):
                counts = "sendcounts"
            else: # gatherv
                counts = "recvcounts"
            # only root need the v-array
            cond_intra = "comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM"
            cond_inter = "comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM"
            cond_a = cond_intra + " && comm_ptr->rank == root"
            cond_b = cond_inter + " && root == MPI_ROOT"

            dump_if_open("(%s) || (%s)" % (cond_a, cond_b))
            swap_one("n", counts)
            swap_next("n", "n", "displs")
            dump_if_close()

            replace_arg(counts, 'tmp_array')
            replace_arg('displs', 'tmp_array + n')

def dump_coll_v_exit(func):
    G.out.append("MPL_free(tmp_array);")

def dump_body_topo_fns(func, method):
    comm_ptr = func['_has_comm'] + "_ptr"
    dump_if_open("%s->topo_fns && %s->topo_fns->%s" % (comm_ptr, comm_ptr, method))
    # The extension will output `MPI_Comm *` rather than `MPIR_Comm **`
    args = re.sub(r'&(\w+)_ptr', r'\1', ", ".join(func['_impl_arg_list']))
    dump_line_with_break("mpi_errno = %s->topo_fns->%s(%s);" % (comm_ptr, method, args))
    dump_error_check("")
    dump_else()
    dump_body_impl(func)
    dump_if_close()

def dump_body_impl(func, prefix='mpir'):
    # mpi_errno = MPIR_Xxx_impl(...);
    if func['_has_handle_out']:
        for p in func['_has_handle_out']:
            (name, kind) = (p['name'], p['kind'])
            mpir_type = G.handle_mpir_types[kind]
            G.out.append("%s *%s_ptr ATTRIBUTE((unused)) = NULL;" % (mpir_type, name))
            if kind in G.handle_NULLs:
                G.out.append("*%s = %s;" % (name, G.handle_NULLs[kind]))
    elif RE.match(r'mpi_type_', func['name'], re.IGNORECASE):
        p = func['c_parameters'][-1]
        if p['kind'] == "DATATYPE" and p['param_direction'] == 'out':
            # check it is not a datatype array (MPI_Type_get_contents)
            if not p['length']:
                G.out.append("*%s = MPI_DATATYPE_NULL;" % p['name'])

    impl = func['name']
    if func['_is_large'] and func['_poly_impl'] == "separate":
        impl += '_large'
    if prefix == 'mpid':
        impl = re.sub(r'^MPIX?_', 'MPID_', impl)
    else:
        impl = re.sub(r'^MPIX?_', 'MPIR_', impl) + "_impl"

    args = ", ".join(func['_impl_arg_list'])
    dump_line_with_break("mpi_errno = %s(%s);" % (impl, args))
    dump_error_check("")

    if func['_has_handle_out']:
        # assuming we are creating new handle(s)
        for p in func['_has_handle_out']:
            (name, kind) = (p['name'], p['kind'])
            dump_if_open("%s_ptr" % name)
            if name == 'win':
                G.out.append("/* Initialize a few fields that have specific defaults */")
                G.out.append("win_ptr->name[0] = 0;")
                G.out.append("win_ptr->errhandler = 0;")
            G.out.append("MPIR_OBJ_PUBLISH_HANDLE(*%s, %s_ptr->handle);" % (name, name))
            dump_if_close()
    elif '_has_handle_inout' in func:
        # assuming we the func is free the handle
        p = func['_has_handle_inout']
        kind = p['kind']
        if kind in G.handle_NULLs:
            G.out.append("*%s = %s;" % (p['name'], G.handle_NULLs[p['kind']]))
        else:
            print("Not sure how to handle inout %s" % p['name'], file=sys.stderr)

def dump_body_threadcomm(func):
    # return MPIR_Threadcomm_Xxx(...);
    impl = func['name']
    if func['_is_large'] and func['_poly_impl'] == "separate":
        impl += '_large'
    impl += '_impl'
    impl = re.sub(r'^mpix?_(comm_)?', 'MPIR_Threadcomm_', impl.lower())

    args = ", ".join(func['_impl_arg_list'])
    if RE.match(r'.*request_ptr$', args):
        G.out.append("MPIR_Request *request_ptr = NULL;")
    elif RE.match(r'.*newcomm_ptr$', args):
        G.out.append("MPIR_Comm *newcomm_ptr = NULL;")
    dump_line_with_break("mpi_errno = %s(%s);" % (impl, args))
    dump_error_check("")
    if RE.match(r'.*request_ptr$', args):
        G.out.append("*request = request_ptr->handle;")
    elif RE.match(r'.*newcomm_ptr$', args):
        G.out.append("*newcomm = newcomm_ptr->handle;")

    push_threadcomm_impl_decl(func)

def dump_function_replace(func, repl_call):
    G.out.append("int mpi_errno = MPI_SUCCESS;")

    G.out.append("MPIR_FUNC_TERSE_ENTER;")
    G.out.append("")
    G.out.append(repl_call)
    G.out.append("")
    G.out.append("MPIR_FUNC_TERSE_EXIT;")
    G.out.append("return mpi_errno;")

def dump_function_direct(func):
    for l in func['body']:
        G.out.append(l)

# -- fn_fail ----
def dump_mpi_fn_fail(func):
    G.out.append("/* --BEGIN ERROR HANDLINE-- */")

    if RE.match(r'mpi_(finalized|initialized)', func['name'], re.IGNORECASE):
        G.out.append("#ifdef HAVE_ERROR_CHECKING")
        cond = "MPIR_Errutil_is_initialized()"
        dump_if_open(cond)
        s = get_fn_fail_create_code(func)
        G.out.append(s)
        G.out.append("mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);")
        dump_if_close()
        G.out.append("#endif")
    else:
        G.out.append("#ifdef HAVE_ERROR_CHECKING")
        s = get_fn_fail_create_code(func)
        dump_line_with_break(s)
        G.out.append("#endif")
        if '_has_comm' in func:
            G.out.append("mpi_errno = MPIR_Err_return_comm(%s_ptr, __func__, mpi_errno);" % func['_has_comm'])
        elif '_has_win' in func:
            G.out.append("mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);")
        elif RE.match(r'mpi_session_init', func['name'], re.IGNORECASE):
            G.out.append("mpi_errno = MPIR_Err_return_session_init(errhandler_ptr, __func__, mpi_errno);")
        elif '_has_session' in func:
            G.out.append("mpi_errno = MPIR_Err_return_session(session_ptr, __func__, mpi_errno);")
        else:
            G.out.append("mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);")

    G.out.append("/* --END ERROR HANDLING-- */")

def get_fn_fail_create_code(func):
    s = "mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,"

    func_name = get_function_name(func, func['_is_large'])
    err_name = func_name.lower()
    mapping = get_kind_map('C', func['_is_large'])

    (fmts, args, err_fmts) = ([], [], [])
    fmt_codes = {'RANK': "i", 'TAG': "t", 'COMMUNICATOR': "C", 'ASSERT': "A", 'DATATYPE': "D", 'ERRHANDLER': "E", 'FILE': "F", 'GROUP': "G", 'INFO': "I", 'OPERATION': "O", 'REQUEST': "R", 'WINDOW': "W", 'SESSION': "S", 'KEYVAL': "K", "GREQUEST_CLASS": "x", "STREAM": "x"}
    for p in func['c_parameters']:
        kind = p['kind']
        name = p['name']
        fmt = None
        if kind == "STRING" and p['param_direction'] != 'out':
            fmt = 's'
        elif is_pointer_type(p):
            fmt = 'p'
        elif kind in fmt_codes:
            fmt = fmt_codes[kind]
        elif mapping[kind] == "int":
            fmt = 'd'
        elif mapping[kind] == "MPI_Aint":
            fmt = 'L'
        elif mapping[kind] == "MPI_Count":
            fmt = 'c'
        else:
            print("Error format [%s] not found" % kind, file=sys.stderr)

        fmts.append('%' + fmt)
        if fmt == 'L':
            args.append("(long long) " + name)
        else:
            args.append(name)
        if RE.match(r'[spdLc]', fmt):
            err_fmts.append(name + "=%" + fmt)
        else:
            err_fmts.append('%' + fmt)

    G.mpi_errnames.append("**%s:%s failed" % (err_name, func_name))
    if args:
        G.mpi_errnames.append("**%s %s:%s(%s) failed" % (err_name, ' '.join(fmts), func_name, ', '.join(err_fmts)))
        s += " \"**%s\", \"**%s %s\", %s);" % (err_name, err_name, ' '.join(fmts), ', '.join(args))
    else:
        s += " \"**%s\", 0);" % (err_name)
    return s

# -- early returns ----
def check_early_returns(func):
    if 'earlyreturn' in func:
        early_returns = func['earlyreturn'].split(',\s*')
        for kind in early_returns:
            if RE.search(r'pt2pt_proc_null', kind, re.IGNORECASE):
                dump_early_return_pt2pt_proc_null(func)
    if 'code-early_return' in func:
        for l in func['code-early_return']:
            G.out.append(l)

def dump_early_return_pt2pt_proc_null(func):
    check_rank, has_request, has_message, has_status, has_flag = '', '', '', '', ''
    for p in func['c_parameters']:
        kind = p['kind']
        name = p['name']
        if kind.startswith("RANK"):
            check_rank = name
        elif p['param_direction'] == 'out':
            if kind == "REQUEST":
                has_request = name
            elif kind == "MESSAGE":
                has_message = name
            elif kind == "STATUS":
                has_status = name
            elif name == "flag":
                has_flag = name
    G.out.append("/* Return immediately for dummy process */")
    G.out.append("if (unlikely(%s == MPI_PROC_NULL)) {" % check_rank)
    G.out.append("INDENT")
    if has_request:
        request_kind = ''
        if RE.search(r'mpi_.*(send|recv|probe)$', func['name'], re.IGNORECASE):
            a = RE.m.group(1)
            request_kind = "MPIR_REQUEST_KIND__" + a.upper()
        elif RE.search(r'mpi_r(put|get|accumulate|get_accumulate)$', func['name'], re.IGNORECASE):
            request_kind = "MPIR_REQUEST_KIND__RMA"
        else:
            raise Exception("Unexpected %s for pt2pt_proc_null" % func['name'])
        G.out.append("MPIR_Request *lw_req = MPIR_Request_create_complete(%s);" % request_kind)
        G.out.append("MPIR_ERR_CHKANDSTMT(lw_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,")
        G.out.append("                    \"**nomemreq\");")
        G.out.append("*request = lw_req->handle;")
    if has_message:
        G.out.append("*message = MPI_MESSAGE_NO_PROC;")
    if has_status:
        G.out.append("MPIR_Status_set_procnull(status);")
    if has_flag:
        G.out.append("*flag = TRUE;")
    G.out.append("goto fn_exit;")
    G.out.append("DEDENT")
    G.out.append("}")

# -- validations (complex, sorry) ----
def dump_handle_ptr_var(func, p):
    (kind, name) = (p['kind'], p['name'])
    if kind == "REQUEST" and p['length']:
        if RE.match(r'mpix?_(test|wait)all', func['name'], re.IGNORECASE):
            # FIXME:we do not convert pointers for MPI_Testall and MPI_Waitall
            #       (for performance reasons). This probably this can be changed
            pass
        else:
            G.out.append("MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];")
            G.out.append("MPIR_Request **request_ptrs = request_ptr_array;")
    else:
        mpir = G.handle_mpir_types[kind]
        G.out.append("%s *%s_ptr ATTRIBUTE((unused)) = NULL;" % (mpir, name))

def dump_validate_handle(func, p):
    func_name = func['name']
    (kind, name) = (p['kind'], p['name'])
    if '_pointer' in p:
        name = "*" + p['name']
        G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (p['name'], p['name']))

    if kind == "COMMUNICATOR":
        if name == 'peer_comm':
            # intercomm_create only checks peer_comm if it is local_leader
            pass
        else:
            G.err_codes['MPI_ERR_COMM'] = 1
            G.out.append("MPIR_ERRTEST_COMM(%s, mpi_errno);" % name)
    elif kind == "GROUP":
        G.err_codes['MPI_ERR_GROUP'] = 1
        G.out.append("MPIR_ERRTEST_GROUP(%s, mpi_errno);" % name)
    elif kind == "WINDOW":
        G.err_codes['MPI_ERR_WIN'] = 1
        G.out.append("MPIR_ERRTEST_WIN(%s, mpi_errno);" % name)
    elif kind == "ERRHANDLER":
        G.err_codes['MPI_ERR_ARG'] = 1
        G.out.append("MPIR_ERRTEST_ERRHANDLER(%s, mpi_errno);" % name)
    elif kind == "REQUEST":
        G.err_codes['MPI_ERR_REQUEST'] = 1
        if p['length']:
            dump_if_open("%s > 0" % p['length'])
            G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
            dump_for_open('i', p['length'])
            if RE.match(r'mpi_startall', func['name'], re.IGNORECASE):
                G.out.append("MPIR_ERRTEST_REQUEST(%s[i], mpi_errno);" % name)
            else:
                G.out.append("MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(%s[i], i, mpi_errno);" % name)
            dump_for_close()
            dump_if_close()
        else:
            if 'can_be_null' in p:
                G.out.append("MPIR_ERRTEST_REQUEST_OR_NULL(%s, mpi_errno);" % name)
            else:
                G.out.append("MPIR_ERRTEST_REQUEST(%s, mpi_errno);" % name)
    elif kind == "MESSAGE":
        G.err_codes['MPI_ERR_REQUEST'] = 1
        G.out.append("MPIR_ERRTEST_REQUEST(%s, mpi_errno);" % name)
    elif kind == "DATATYPE":
        G.err_codes['MPI_ERR_TYPE'] = 1
        G.out.append("MPIR_ERRTEST_DATATYPE(%s, \"%s\", mpi_errno);" % (name, name))
    elif kind == "OP":
        G.err_codes['MPI_ERR_OP'] = 1
        G.out.append("MPIR_ERRTEST_OP(%s, mpi_errno);" % name)
    elif kind == "INFO":
        G.err_codes['MPI_ERR_INFO'] = 1
        if 'can_be_null' in p:
            G.out.append("MPIR_ERRTEST_INFO_OR_NULL(%s, mpi_errno);" % name)
        else:
            G.out.append("MPIR_ERRTEST_INFO(%s, mpi_errno);" % name)

def dump_convert_handle(func, p):
    (kind, name) = (p['kind'], p['name'])
    ptr_name = name + "_ptr"
    if '_pointer' in p:
        name = "*" + p['name']

    if kind == "REQUEST" and p['length']:
        if RE.match(r'mpix?_(test|wait)all', func['name'], re.IGNORECASE):
            # We do not convert pointers for MPI_Testall and MPI_Waitall
            pass
        else:
            G.out.append("if (%s > MPIR_REQUEST_PTR_ARRAY_SIZE) {" % p['length'])
            G.out.append("    int nbytes = %s * sizeof(MPIR_Request *);" % p['length'])
            G.out.append("    request_ptrs = (MPIR_Request **) MPL_malloc(nbytes, MPL_MEM_OBJECT);")
            G.out.append("    if (request_ptrs == NULL) {")
            G.out.append("        MPIR_CHKMEM_SETERR(mpi_errno, nbytes, \"request pointers\");")
            G.out.append("        goto fn_fail;")
            G.out.append("    }")
            G.out.append("}")
            func['code-clean_up'].append("if (%s > MPIR_REQUEST_PTR_ARRAY_SIZE) {" % (p['length']))
            func['code-clean_up'].append("    MPL_free(request_ptrs);")
            func['code-clean_up'].append("}")

            G.out.append("")
            dump_for_open('i', p['length'])
            G.out.append("MPIR_Request_get_ptr(%s[i], request_ptrs[i]);" % name)
            dump_for_close()
    else:
        mpir = G.handle_mpir_types[kind]
        if "can_be_null" in p:
            G.out.append("if (%s != %s) {" % (name, p['can_be_null']))
            G.out.append("    %s_get_ptr(%s, %s);" % (mpir, name, ptr_name))
            G.out.append("}")
        else:
            G.out.append("%s_get_ptr(%s, %s);" % (mpir, name, ptr_name))

        if kind == "REQUEST" and "_comm_from_request" in func:
            G.out.append("if (%s) {" % ptr_name)
            G.out.append("    /* Because %s may be freed, save a copy of comm_ptr for use at fn_fail */")
            G.out.append("    comm_ptr = %s->comm;" % ptr_name)
            G.out.append("}")

def dump_validate_handle_ptr(func, p):
    func_name = func['name']
    (kind, name) = (p['kind'], p['name'])
    ptr_name = name + "_ptr"
    if '_pointer' in p:
        name = "*" + p['name']
    mpir = G.handle_mpir_types[kind]
    if kind == "REQUEST" and p['length']:
        G.err_codes['MPI_ERR_REQUEST'] = 1
        if RE.match(r'mpix?_(test|wait)all', func['name'], re.IGNORECASE):
            # MPI_Testall and MPI_Waitall do pointer conversion inside MPIR_{Test,Wait}all
            pass
        else:
            ptr = p['_ptrs_name'] + '[i]'
            dump_for_open('i', p['length'])
            if RE.match(r'mpi_startall', func['name'], re.IGNORECASE):
                G.out.append("MPIR_Request_valid_ptr(%s, mpi_errno);" % ptr)
                dump_error_check("")
                G.out.append("MPIR_ERRTEST_STARTREQ(%s, mpi_errno);" % ptr)
                G.out.append("MPIR_ERRTEST_STARTREQ_ACTIVE(%s, mpi_errno);" % ptr)
            else:
                dump_if_open("%s[i] != MPI_REQUEST_NULL" % name)
                G.out.append("MPIR_Request_valid_ptr(%s, mpi_errno);" % ptr)
                dump_error_check("")
                dump_if_close()
            dump_for_close()
    elif p['kind'] == "MESSAGE":
        G.err_codes['MPI_ERR_REQUEST'] = 1
        G.out.append("if (%s != MPI_MESSAGE_NO_PROC) {" % name)
        G.out.append("    MPIR_Request_valid_ptr(%s, mpi_errno);" % ptr_name)
        dump_error_check("    ")
        G.out.append("    MPIR_ERR_CHKANDJUMP((%s->kind != MPIR_REQUEST_KIND__MPROBE)," % ptr_name)
        G.out.append("                        mpi_errno, MPI_ERR_ARG, \"**reqnotmsg\");")
        G.out.append("}")
    elif kind == "COMMUNICATOR" and RE.match(r'mpi_intercomm_create', func_name, re.IGNORECASE):
        # use custom code in func['cond-error_check']
        pass
    elif kind == "COMMUNICATOR":
        G.err_codes['MPI_ERR_COMM'] = 1
        G.out.append("MPIR_Comm_valid_ptr(%s, mpi_errno, %s);" % (ptr_name, func['_comm_valid_ptr_flag']))
        dump_error_check("")

        if '_errtest_comm_intra' in func:
            G.out.append("MPIR_ERRTEST_COMM_INTRA(%s, mpi_errno);" % ptr_name)
    else:
        if "can_be_null" in p:
            G.out.append("if (%s != %s) {" % (name, p['can_be_null']))
            G.out.append("    %s_valid_ptr(%s, mpi_errno);" % (mpir, ptr_name))
            dump_error_check("    ")
            G.out.append("}")
        else:
            G.out.append("%s_valid_ptr(%s, mpi_errno);" % (mpir, ptr_name))
            dump_error_check("")

        if kind == "REQUEST" and RE.match(r'mpi_start', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_STARTREQ(%s, mpi_errno);" % ptr_name)
            G.out.append("MPIR_ERRTEST_STARTREQ_ACTIVE(%s, mpi_errno);" % ptr_name)

        if kind == "REQUEST" and RE.match(r'mpi_pready(_range|_list)?$', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_PREADYREQ(%s, mpi_errno);" % ptr_name)

        if kind == "REQUEST" and RE.match(r'mpi_parrived', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_PARRIVEDREQ(%s, mpi_errno);" % ptr_name)

        if kind == "WINDOW" and RE.match(r'mpi_win_shared_query', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_WIN_FLAVOR(win_ptr, MPI_WIN_FLAVOR_SHARED, mpi_errno);")

        if G.handle_error_codes[kind]:
            G.err_codes[G.handle_error_codes[kind]] = 1

def dump_validation(func, t):
    func_name = func['name']
    (kind, name) = (t['kind'], t['name'])
    if kind == "RANK":
        G.err_codes['MPI_ERR_RANK'] = 1
        if '_has_comm' in func:
            comm_ptr = func['_has_comm'] + '_ptr'
        elif '_has_win' in func:
            comm_ptr = "win_ptr->comm_ptr"
        else:
            raise Exception("dump_validation RANK: missing comm_ptr in %s" % func_name)

        if RE.match(r'mpi_(i?m?probe|i?recv|recv_init)$', func_name, re.IGNORECASE) or name == "recvtag":
            # allow MPI_ANY_SOURCE, MPI_PROC_NULL
            G.out.append("MPIR_ERRTEST_RECV_RANK(%s, %s, mpi_errno);" % (comm_ptr, name))
        elif RE.match(r'mpi_(i?sendrecv)', func_name, re.IGNORECASE) and name == "source":
            G.out.append("MPIR_ERRTEST_RECV_RANK(%s, %s, mpi_errno);" % (comm_ptr, name))
        elif RE.match(r'(dest|target_rank)$', name) or RE.match(r'mpi_win_', func_name, re.IGNORECASE):
            # allow MPI_PROC_NULL
            G.out.append("MPIR_ERRTEST_SEND_RANK(%s, %s, mpi_errno);" % (comm_ptr, name))
        else:
            G.out.append("MPIR_ERRTEST_RANK(%s, %s, mpi_errno);" % (comm_ptr, name))
    elif kind == "TAG":
        G.err_codes['MPI_ERR_TAG'] = 1
        if RE.match(r'mpi_(i?m?probe|i?recv|recv_init)$', func_name, re.IGNORECASE) or name == "recvtag":
            G.out.append("MPIR_ERRTEST_RECV_TAG(%s, mpi_errno);" % name)
        else:
            G.out.append("MPIR_ERRTEST_SEND_TAG(%s, mpi_errno);" % name)
    elif kind == "ROOT":
        if RE.search(r'mpi_comm_', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);")
        else:
            G.out.append("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {")
            G.out.append("    MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);")
            G.out.append("} else {")
            G.out.append("    MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);")
            G.out.append("}")
    elif kind == "COUNT":
        G.err_codes['MPI_ERR_COUNT'] = 1
        G.out.append("MPIR_ERRTEST_COUNT(%s, mpi_errno);" % name)
    elif kind == "WIN_SIZE":
        G.err_codes['MPI_ERR_SIZE'] = 1
        G.out.append("MPIR_ERRTEST_WIN_SIZE(%s, mpi_errno);" % name)
    elif kind == "WIN_DISPUNIT":
        G.err_codes['MPI_ERR_DISP'] = 1
        G.out.append("MPIR_ERRTEST_WIN_DISPUNIT(%s, mpi_errno);" % name)
    elif kind == "RMADISP":
        G.err_codes['MPI_ERR_DISP'] = 1
        G.out.append("if (win_ptr->create_flavor != MPI_WIN_FLAVOR_DYNAMIC) {")
        G.out.append("    MPIR_ERRTEST_DISP(%s, mpi_errno);" % name)
        G.out.append("}")
    elif kind == "WINBUFFER":
        G.err_codes['MPI_ERR_ARG'] = 1
        G.out.append("if (size > 0) {")
        G.out.append("    MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
        G.out.append("}")
    elif RE.search(r'USERBUFFER', kind):
        G.err_codes['MPI_ERR_BUFFER'] = 1
        G.err_codes['MPI_ERR_COUNT'] = 1
        G.err_codes['MPI_ERR_TYPE'] = 1
        p = name.split(',')
        if kind == "USERBUFFER-simple":
            dump_validate_userbuffer_simple(func, p[0], p[1], p[2])
        elif kind == "USERBUFFER-partition":
            dump_validate_userbuffer_partition(func, p[0], p[1], p[2], p[3])
        elif kind == "USERBUFFER-reduce":
            G.err_codes['MPI_ERR_OP'] = 1
            dump_validate_userbuffer_reduce(func, p[0], p[1], p[2], p[3], p[4])
        elif kind == "USERBUFFER-neighbor":
            dump_validate_userbuffer_simple(func, p[0], p[1], p[2])
        elif kind.startswith("USERBUFFER-neighbor"):
            dump_validate_userbuffer_neighbor_vw(func, kind, p[0], p[1], p[3], p[2])
        elif RE.search(r'-[vw]$', kind):
            dump_validate_userbuffer_coll(func, kind, p[0], p[1], p[3], p[2])
        else:
            dump_validate_userbuffer_coll(func, kind, p[0], p[1], p[2], "")
    elif RE.match(r'STATUS$', kind):
        G.err_codes['MPI_ERR_ARG'] = 1
        dump_if_open("%s != MPI_STATUS_IGNORE" % name)
        G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
        dump_if_close()
    elif RE.match(r'STATUS-length$', kind):
        G.err_codes['MPI_ERR_ARG'] = 1
        dump_if_open("%s != MPI_STATUSES_IGNORE && %s > 0" % (name, t['length']))
        G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
        dump_if_close()
    elif RE.match(r'(ARGNULL)$', kind):
        if func['dir'] == 'mpit':
            G.err_codes['MPI_T_ERR_INVALID'] = 1
            G.out.append("MPIT_ERRTEST_ARGNULL(%s);" % name)
        else:
            G.err_codes['MPI_ERR_ARG'] = 1
            G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
    elif RE.match(r'(ARGNULL-length)$', kind):
        G.err_codes['MPI_ERR_ARG'] = 1
        dump_if_open("%s > 0" % t['length'])
        G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
        dump_if_close()
    elif RE.match(r'(ARGNEG)$', kind):
        if func['dir'] == 'mpit':
            G.err_codes['MPI_T_ERR_INVALID'] = 1
            G.out.append("MPIT_ERRTEST_ARGNEG(%s);" % name)
        else:
            G.err_codes['MPI_ERR_ARG'] = 1
            G.out.append("MPIR_ERRTEST_ARGNEG(%s, \"%s\", mpi_errno);" % (name, name))
    elif RE.match(r'(ARGNONPOS)$', kind):
        G.err_codes['MPI_ERR_ARG'] = 1
        G.out.append("MPIR_ERRTEST_ARGNONPOS(%s, \"%s\", mpi_errno, MPI_ERR_ARG);" % (name, name))
    elif RE.match(r'(TYPE_RMA_ATOMIC|OP_ACC|OP_GACC)$', kind):
        a = RE.m.group(1)
        G.out.append("MPIR_ERRTEST_%s(%s, mpi_errno);" % (a, name))
    elif kind == "datatype_and_ptr":
        G.err_codes['MPI_ERR_TYPE'] = 1
        dump_validate_datatype(func, name)
    elif kind == "op_ptr":
        G.err_codes['MPI_ERR_OP'] = 1
        dump_validate_op(name, "", False)
    elif kind == "op_and_ptr":
        G.err_codes['MPI_ERR_OP'] = 1
        dump_validate_op(name, "", True)
    elif kind == 'KEYVAL':
        G.err_codes['MPI_ERR_KEYVAL'] = 1
        if RE.match(r'mpi_comm_', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_COMM, \"%s\", mpi_errno);" % (name, name))
        elif RE.match(r'mpi_type_', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_DATATYPE, \"%s\", mpi_errno);" % (name, name))
        elif RE.match(r'mpi_win_', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_WIN, \"%s\", mpi_errno);" % (name, name))
        if not RE.match(r'\w+_(get_attr)', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_KEYVAL_PERM(%s, mpi_errno);" % name)
    elif kind == "group_check_valid_ranks":
        G.out.append("if (group_ptr) {")
        G.out.append("    mpi_errno = MPIR_Group_check_valid_ranks(group_ptr, ranks, n);")
        dump_error_check("    ")
        G.out.append("}")
    elif kind == "infokey":
        G.out.append("MPIR_ERR_CHKANDJUMP((!%s), mpi_errno, MPI_ERR_INFO_KEY, \"**infokeynull\");" % (name))
        G.out.append("int keylen = (int) strlen(%s);" % name)
        G.out.append("MPIR_ERR_CHKANDJUMP((keylen > MPI_MAX_INFO_KEY), mpi_errno, MPI_ERR_INFO_KEY, \"**infokeylong\");")
        G.out.append("MPIR_ERR_CHKANDJUMP((keylen == 0), mpi_errno, MPI_ERR_INFO_KEY, \"**infokeyempty\");")
    elif RE.match(r'mpit_(cat|cvar|pvar)_index', kind, re.I):
        T = RE.m.group(1).upper()
        G.err_codes['MPI_T_ERR_INVALID_INDEX'] = 1
        G.out.append("MPIT_ERRTEST_%s_INDEX(%s);" % (T, name))
    elif RE.match(r'mpit_(enum|cvar|pvar)_handle', kind, re.I):
        T = RE.m.group(1).upper()
        G.err_codes['MPI_T_ERR_INVALID_HANDLE'] = 1
        G.out.append("MPIT_ERRTEST_%s_HANDLE(%s);" % (T, name))
    elif kind == "mpit_enum_item":
        # note: name is "enum_, index_"
        G.err_codes['MPI_T_ERR_INVALID_ITEM'] = 1
        G.out.append("MPIT_ERRTEST_ENUM_ITEM(%s);" % name)
    elif kind == "mpit_pvar_session":
        G.err_codes['MPI_T_ERR_INVALID_SESSION'] = 1
        G.out.append("MPIT_ERRTEST_PVAR_SESSION(%s);" % name)
    elif kind == "mpit_pvar_class":
        G.err_codes['MPI_T_ERR_INVALID_NAME'] = 1
        G.out.append("MPIT_ERRTEST_PVAR_CLASS(%s);" % name)
    elif kind == "mpit_event_registration":
        G.err_codes['MPI_T_ERR_INVALID_HANDLE'] = 1
        G.out.append('MPIT_ERRTEST_EVENT_REG_HANDLE(%s);' % name)
    elif kind == "mpit_event_instance":
        G.err_codes['MPI_T_ERR_INVALID_HANDLE'] = 1
        G.out.append('MPIT_ERRTEST_EVENT_INSTANCE_HANDLE(%s);' % name)
    elif kind == "RANK-ARRAY":
        # FIXME
        pass
    elif kind == "DISP-ARRAY":
        G.err_codes['MPI_ERR_ARG'] = 1
        dump_if_open("%s > 0" % t['length'])
        G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
        dump_if_close()
    elif kind == "COUNT-ARRAY":
        G.err_codes['MPI_ERR_ARG'] = 1
        G.err_codes['MPI_ERR_COUNT'] = 1
        dump_if_open("%s > 0" % t['length'])
        G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
        dump_for_open('i', t['length'])
        G.out.append("MPIR_ERRTEST_COUNT(%s[i], mpi_errno);" % name)
        dump_for_close()
        dump_if_close()
    elif kind == "TYPE-ARRAY":
        G.err_codes['MPI_ERR_ARG'] = 1
        G.err_codes['MPI_ERR_TYPE'] = 1
        dump_if_open("%s > 0" % t['length'])
        G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
        dump_for_open('i', t['length'])
        if re.match(r'mpi_type_create_struct', func_name, re.IGNORECASE):
            # MPI_DATATYPE_NULL not allowed
            cond = "!HANDLE_IS_BUILTIN(%s[i])" % name
        else:
            cond = "%s[i] != MPI_DATATYPE_NULL && !HANDLE_IS_BUILTIN(%s[i])" % (name, name)
        dump_if_open(cond)
        G.out.append("MPIR_Datatype *datatype_ptr;")
        G.out.append("MPIR_Datatype_get_ptr(%s[i], datatype_ptr);" % name)
        G.out.append("MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);")
        dump_error_check('')
        dump_if_close()
        dump_for_close()
        dump_if_close()
    else:
        print("Unhandled validation kind: %s - %s" % (kind, name), file=sys.stderr)

def dump_validate_userbuffer_simple(func, buf, ct, dt):
    check_no_op = False
    if RE.match(r'mpi_r?get_accumulate', func['name'], re.IGNORECASE) and buf.startswith("origin_"):
        check_no_op = True
    if check_no_op:
        dump_if_open("op != MPI_NO_OP")
    G.out.append("MPIR_ERRTEST_COUNT(%s, mpi_errno);" % ct)
    if func['dir'] == 'rma':
        # RMA doesn't make sense to have zero-count message, always validate datatype
        dump_validate_datatype(func, dt)
        G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (buf, ct, dt))
    else:
        dump_if_open("%s > 0" % ct)
        dump_validate_datatype(func, dt)
        G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (buf, ct, dt))
        dump_if_close()
    if check_no_op:
        dump_if_close()

def dump_validate_userbuffer_partition(func, buf, partitions, ct, dt):
    dump_validate_userbuffer_simple(func, buf, ct, dt)
    dump_if_open("%s > 0" % ct)
    G.out.append("MPIR_ERRTEST_ARGNONPOS(%s, \"%s\", mpi_errno, MPI_ERR_ARG);" % (partitions, partitions))
    dump_if_close()

def dump_validate_userbuffer_neighbor_vw(func, kind, buf, ct, dt, disp):
    dump_validate_get_topo_size(func)
    size = "outdegree"
    if RE.search(r'recv', buf):
        size = "indegree"

    if not RE.search(r'-w$', kind):
        dump_validate_datatype(func, dt)

    dump_for_open('i', size)
    ct += '[i]'
    if RE.search(r'-w$', kind):
        dt += '[i]'
        dump_if_open("%s > 0" % ct)
        dump_validate_datatype(func, dt)
        dump_if_close()
    G.out.append("MPIR_ERRTEST_COUNT(%s, mpi_errno);" % ct)
    G.out.append("if (%s[i] == 0) {" % disp)
    G.out.append("    MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (buf, ct, dt))
    G.out.append("}")
    dump_for_close()

def dump_validate_userbuffer_reduce(func, sbuf, rbuf, ct, dt, op):
    cond_intra = "comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM"
    cond_inter = "comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM"
    if RE.match(r'mpi_reduce_local$', func['name'], re.IGNORECASE):
        dump_validate_op(op, dt, True)
        dump_validate_datatype(func, dt)
        G.out.append("if (%s > 0) {" % ct)
        G.out.append("    MPIR_ERRTEST_ALIAS_COLL(%s, %s, mpi_errno);" % (sbuf, rbuf))
        G.out.append("}")
        G.out.append("MPIR_ERRTEST_NAMED_BUF_INPLACE(%s, \"%s\", %s, mpi_errno);" % (sbuf, sbuf, ct))
        G.out.append("MPIR_ERRTEST_NAMED_BUF_INPLACE(%s, \"%s\", %s, mpi_errno);" % (rbuf, rbuf, ct))
    elif RE.match(r'mpi_i?reduce(_init)?$', func['name'], re.IGNORECASE):
        # exclude intercomm MPI_PROC_NULL
        G.out.append("if (" + cond_intra + " || root != MPI_PROC_NULL) {")
        G.out.append("INDENT")
        dump_validate_op(op, dt, True)
        dump_validate_datatype(func, dt)

        cond_a = cond_intra + " && comm_ptr->rank == root"
        cond_b = cond_inter + " && root == MPI_ROOT"
        G.out.append("if ((" + cond_a + ") || (" + cond_b + ")) {")
        # test recvbuf
        G.out.append("    MPIR_ERRTEST_RECVBUF_INPLACE(%s, %s, mpi_errno);" % (rbuf, ct))
        G.out.append("    MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (rbuf, ct, dt))
        G.out.append("    if (%s > 0 && %s != MPI_IN_PLACE) {" % (ct, sbuf))
        G.out.append("        MPIR_ERRTEST_ALIAS_COLL(%s, %s, mpi_errno);" % (sbuf, rbuf))
        G.out.append("    }")
        G.out.append("}")
        # test sendbuf
        cond_a = cond_intra
        cond_b = cond_inter + " && root != MPI_ROOT && root != MPI_PROC_NULL"
        dump_if_open("(" + cond_a + ") || (" + cond_b + ")")
        G.out.append("if (" + cond_inter + ") {")
        G.out.append("    MPIR_ERRTEST_SENDBUF_INPLACE(%s, %s, mpi_errno);" % (sbuf, ct))
        G.out.append("}")
        G.out.append("if (%s > 0 && %s != MPI_IN_PLACE) {" % (ct, sbuf))
        G.out.append("    MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (sbuf, ct, dt))
        G.out.append("}")
        dump_if_close()

        G.out.append("DEDENT")
        G.out.append("}")
    else:
        dump_validate_op(op, dt, True)
        dump_validate_datatype(func, dt)
        (sct, rct) = (ct, ct)
        if RE.search(r'reduce_scatter(_init)?$', func['name'], re.IGNORECASE):
            dump_validate_get_comm_size(func)
            G.out.append("int sum = 0;")
            dump_for_open('i', 'comm_size')
            G.out.append("MPIR_ERRTEST_COUNT(%s[i], mpi_errno);" % ct)
            G.out.append("sum += %s[i];" % ct)
            dump_for_close()
            sct = "sum"
            rct = ct + "[comm_ptr->rank]"
        G.out.append("MPIR_ERRTEST_RECVBUF_INPLACE(%s, %s, mpi_errno);" % (rbuf, rct))
        G.out.append("if (" + cond_inter + ") {")
        G.out.append("    MPIR_ERRTEST_SENDBUF_INPLACE(%s, %s, mpi_errno);" % (sbuf, sct))
        G.out.append("} else if (%s != MPI_IN_PLACE && %s != 0) {" % (sbuf, sct))
        G.out.append("    MPIR_ERRTEST_ALIAS_COLL(%s, %s, mpi_errno);" % (sbuf, rbuf))
        G.out.append("}")

        G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (rbuf, rct, dt))
        G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (sbuf, sct, dt))

def dump_validate_userbuffer_coll(func, kind, buf, ct, dt, disp):
    func_name = func['name']
    cond_intra = "comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM"
    cond_inter = "comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM"

    inplace = True
    if RE.search(r'-noinplace', kind):
        inplace = False

    check_alias = None
    if inplace:
        check_alias = cond_intra

    with_vw = False
    if RE.search(r'-[vw]$', kind):
        with_vw = True
        dump_validate_get_comm_size(func)

    # -- whether the buffer need be checked (or ignored)
    check_buf = None
    if RE.search(r'_i?(gather|scatter)', func_name, re.IGNORECASE):
        if inplace:
            cond_a = cond_intra + " && %s != MPI_IN_PLACE" % buf
            cond_b = cond_inter + " && root != MPI_ROOT && root != MPI_PROC_NULL"
        else:
            cond_a = cond_intra + " && comm_ptr->rank == root"
            cond_b = cond_inter + " && root == MPI_ROOT"
        check_buf = "(%s) || (%s)" % (cond_a, cond_b)
        if inplace:
            check_alias = cond_intra + " && comm_ptr->rank == root"
    elif inplace:
            check_buf = " %s != MPI_IN_PLACE" % buf

    if check_buf:
        G.out.append("if (%s) {" % check_buf)
        G.out.append("INDENT")

    if not RE.search(r'-w$', kind):
        dump_validate_datatype(func, dt)

    if with_vw:
        dump_for_open('i', 'comm_size')
        ct += '[i]'
        if RE.search(r'-w$', kind):
            dt += '[i]'
            dump_if_open("%s > 0" % ct)
            dump_validate_datatype(func, dt)
            dump_if_close()

    #  -- test wrong MPI_IN_PLACE
    SEND = "SEND"
    if RE.search(r'recv', buf):
        SEND = "RECV"
    if RE.search(r'-noinplace', kind):
        G.out.append("MPIR_ERRTEST_%sBUF_INPLACE(%s, %s, mpi_errno);" % (SEND, buf, ct))
    else:
        G.out.append("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {")
        G.out.append("    MPIR_ERRTEST_%sBUF_INPLACE(%s, %s, mpi_errno);" % (SEND, buf, ct))
        G.out.append("}")

    # -- check count && buffer
    G.out.append("MPIR_ERRTEST_COUNT(%s, mpi_errno);" % ct)
    if disp:
        dump_if_open("%s[i] == 0" % disp)
    G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (buf, ct, dt))
    if disp:
        dump_if_close()

    if with_vw:
        dump_for_close() # i = 0:comm_size

    if check_alias:
        G.out.append("if (%s) {" % check_alias)
        G.out.append("INDENT")
        if RE.search(r'i?(alltoall)', func_name, re.IGNORECASE):
            cond = "sendtype == recvtype && sendcount == recvcount && sendcount != 0"
            if RE.search(r'-v$', kind):
                cond = "sendtype == recvtype && sendcounts == recvcounts"
            elif RE.search(r'-w$', kind):
                cond = "sendtypes == recvtypes && sendcounts == recvcounts"
            G.out.append("if (%s) {" % cond)
            G.out.append("    MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);")
            G.out.append("}")
        elif RE.search(r'i?(allgather|gather|scatter)(v?)(_init)?$', func_name, re.IGNORECASE):
            t1, t2 = RE.m.group(1, 2)
            (a, b) = ("send", "recv")
            if RE.match(r'scatter', t1, re.IGNORECASE):
                (a, b) = ("recv", "send")
            cond = "sendtype == recvtype && sendcount == recvcount && sendcount != 0"
            if t2 == "v":
                cond = "sendtype == recvtype && %scount != 0 && %scounts[comm_ptr->rank] !=0" % (a, b)

            buf2 = "(char *) %sbuf + comm_ptr->rank * %scount * %stype_size" % (b, b, b)
            if t2 == "v":
                buf2 = "(char *) %sbuf + displs[comm_ptr->rank] * %stype_size" % (b, b)

            G.out.append("if (%s) {" % cond)
            G.out.append("    MPI_Aint %stype_size;" % b)
            G.out.append("    MPIR_Datatype_get_size_macro(%stype, %stype_size);" % (b, b))
            G.out.append("    MPIR_ERRTEST_ALIAS_COLL(%s, %s, mpi_errno);" % (buf, buf2))
            G.out.append("}")
        else:
            raise Exception("dump_validate_userbuffer_coll: inplace alias check: unexpected %s" % func_name)
        G.out.append("DEDENT")
        G.out.append("}")

    if check_buf:
        G.out.append("DEDENT")
        G.out.append("}")

def dump_validate_datatype(func, dt):
    G.out.append("MPIR_ERRTEST_DATATYPE(%s, \"datatype\", mpi_errno);" % dt)
    G.out.append("if (!HANDLE_IS_BUILTIN(%s)) {" % dt)
    G.out.append("    MPIR_Datatype *datatype_ptr ATTRIBUTE((unused)) = NULL;")
    G.out.append("    MPIR_Datatype_get_ptr(%s, datatype_ptr);" % dt)
    G.out.append("    MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);")
    dump_error_check("    ")
    if not RE.match(r'mpi_(type_|get_count|pack_external_size|status_set_elements)', func['name'], re.IGNORECASE):
        G.out.append("    MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);")
        dump_error_check("    ")
    G.out.append("}")

def dump_validate_op(op, dt, is_coll):
    if is_coll:
        G.out.append("MPIR_ERRTEST_OP(%s, mpi_errno);" % op)
    G.out.append("if (!HANDLE_IS_BUILTIN(%s)) {" % op)
    G.out.append("    MPIR_Op *op_ptr = NULL;")
    G.out.append("    MPIR_Op_get_ptr(%s, op_ptr);" % op)
    G.out.append("    MPIR_Op_valid_ptr(op_ptr, mpi_errno);")
    if dt:
        G.out.append("} else {")
        G.out.append("    mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(%s)) (%s);" % (op, dt))
        # check predefined datatype and replace with basic_type if necessary
        G.out.append("    if (mpi_errno != MPI_SUCCESS) {")
        G.out.append("        MPI_Datatype alt_dt = MPIR_Op_get_alt_datatype(%s, %s);" % (op, dt))
        G.out.append("        if (alt_dt != MPI_DATATYPE_NULL) {")
        G.out.append("            %s = alt_dt;" % dt)
        G.out.append("            mpi_errno = MPI_SUCCESS;")
        G.out.append("        }")
        G.out.append("    }")
    G.out.append("}")
    dump_error_check("")

def dump_validate_get_comm_size(func):
    if '_got_comm_size' not in func:
        if RE.match(r'mpi_i?(reduce_scatter_init|reduce_scatter)\b', func['name'], re.IGNORECASE):
            G.out.append("int comm_size = comm_ptr->local_size;")
        else:
            G.out.append("int comm_size;")
            G.out.append("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {")
            G.out.append("    comm_size = comm_ptr->remote_size;")
            G.out.append("} else {")
            G.out.append("    comm_size = comm_ptr->local_size;")
            G.out.append("}")
        func['_got_comm_size'] = 1

def dump_validate_get_topo_size(func):
    if '_got_topo_size' not in func:
        G.out.append("int indegree, outdegree, weighted;")
        G.out.append("mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);")
        func['_got_topo_size'] = 1

# ---- supporting routines (reusable) ----

def get_function_args(func):
    arg_list = []
    for p in func['c_parameters']:
        arg_list.append(p['name'])
    return ', '.join(arg_list)

def get_declare_function(func, is_large, kind=""):
    filter_c_parameters(func)
    name = get_function_name(func, is_large)
    mapping = get_kind_map('C', is_large)

    ret = "int"
    if 'return' in func:
        ret = mapping[func['return']]

    params = get_C_params(func, mapping)
    s_param = ', '.join(params)
    s = "%s %s(%s)" % (ret, name, s_param)

    if kind == 'proto':
        if '_pointertag_list' in func:
            for t in func['_pointertag_list']:
                s += " MPICH_ATTR_POINTER_WITH_TYPE_TAG(%s)" % t
        s += " MPICH_API_PUBLIC"
    return s

def get_C_params(func, mapping):
    param_list = []
    for p in func['c_parameters']:
        param_list.append(get_C_param(p, func, mapping))
    if not len(param_list):
        return ["void"]
    else:
        return param_list

def get_impl_param(func, param):
    mapping = get_kind_map('C', func['_is_large'])

    s = get_C_param(param, func, mapping)
    if RE.match(r'POLY', param['kind']):
        if func['_is_large']:
            # internally we always use MPI_Aint for poly type
            s = re.sub(r'MPI_Count', r'MPI_Aint', s)
        elif func['_poly_impl'] != "separate":
            # SMALL but internally use MPI_Aint
            s = re.sub(r'\bint\b', r'MPI_Aint', s)
    return s

def get_polymorph_param_and_arg(s):
    # s is a polymorph spec, e.g. "MPIR_Attr_type attr_type=MPIR_ATTR_PTR"
    # Note: we assume limit of single extra param for now (which is sufficient)
    RE.match(r'^\s*(.+?)\s*(\w+)\s*=\s*(.+)', s)
    extra_param = RE.m.group(1) + ' ' + RE.m.group(2)
    extra_arg = RE.m.group(3)
    return (extra_param, extra_arg)

def dump_error_check(sp):
    G.out.append("%sif (mpi_errno) {" % sp)
    G.out.append("%s    goto fn_fail;" % sp)
    G.out.append("%s}" % sp)

def dump_if_open(cond):
    G.out.append("if (%s) {" % cond)
    G.out.append("INDENT")

def dump_else():
    G.out.append("DEDENT")
    G.out.append("} else {")
    G.out.append("INDENT")

def dump_if_close():
    G.out.append("DEDENT")
    G.out.append("}")

def dump_for_open(idx, count):
    G.out.append("for (int %s = 0; %s < %s; %s++) {" % (idx, idx, count, idx))
    G.out.append("INDENT")

def dump_for_close():
    G.out.append("DEDENT")
    G.out.append("}")

def dump_error(errname, errfmt, errargs):
    G.out.append("mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,")
    G.out.append("                                 __func__, __LINE__, MPI_ERR_OTHER,")
    G.out.append("                                 \"**%s\"," % errname)
    G.out.append("                                 \"**%s %s\", %s);" % (errname, errfmt, errargs))
    G.out.append("goto fn_fail;")

def dump_line_with_break(s, tail=''):
    tlist = split_line_with_break(s, tail, 100)
    G.out.extend(tlist)
