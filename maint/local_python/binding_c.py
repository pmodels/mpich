##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python import RE

import sys
import os
import re

# ---- top-level routines ----

def dump_mpi_c(func, mapping):
    """Dumps the function's C source code to G.out array"""
    check_func_directives(func)
    filter_c_parameters(func)
    check_params_with_large_only(func, mapping)
    process_func_parameters(func, mapping)
    # collect error codes additional from auto generated ones
    if 'error' in func:
        for a in func['error'].split(", "):
            G.err_codes[a] = 1
    # -- "dump" accumulates output lines in G.out
    G.out.append("#include \"mpiimpl.h\"")
    if 'include' in func:
        for a in func['include'].replace(',', ' ').split():
            G.out.append("#include \"%s\"" % a)
    G.out.append("")
    dump_profiling(func, mapping)

    if 'polymorph' in func:
        # MPII_ function to support C/Fortran Polymorphism, eg MPI_Comm_get_attr
        # It needs go inside "#ifndef MPICH_MPI_FROM_PMPI"
        G.out.pop() # #endif from dump_profiling()
        dump_function(func, mapping, kind="polymorph")
        G.out.append("#endif /* MPICH_MPI_FROM_PMPI */")

    G.out.append("")
    dump_manpage(func)
    if 'polymorph' in func:
        dump_function(func, mapping, kind="call-polymorph")
    elif 'replace' in func and 'body' not in func:
        dump_function(func, mapping, kind="call-replace")
    else:
        dump_function(func, mapping, kind="normal")

def get_func_file_path(func, root_dir):
    file_path = None
    dir_path = root_dir + '/' + func['dir']
    if not os.path.isdir(dir_path):
        os.mkdir(dir_path)
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
            if RE.match(r'\[ERR CODES\]', l):
                # man page error codes
                print(".N Errors", file=Out)
                print(".N MPI_SUCCESS\n", file=Out)
                for err in sorted (G.err_codes.keys()):
                    print(".N %s" % (err), file=Out)
                print(".N MPI_ERR_OTHER\n", file=Out)
            elif RE.match(r'(INDENT|DEDENT)', l):
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

def dump_mpir_impl_h(f):
    print("  --> [%s]" %f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)
        print("#ifndef MPIR_IMPL_H_INCLUDED", file=Out)
        print("#define MPIR_IMPL_H_INCLUDED", file=Out)
        print("", file=Out)
        for l in G.impl_declares:
            print(l, file=Out)
        print("", file=Out)
        print("#endif /* MPIR_IMPL_H_INCLUDED */", file=Out)

def dump_errnames_txt(f):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_mk:
            print(l, file=Out)
        for l in G.mpi_errnames:
            print(l, file=Out)
        
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

def filter_c_parameters(func):
    c_params = []
    for p in func['parameters']:
        if RE.search(r'c_parameter', p['suppress']):
            pass
        else:
            c_params.append(p)
    func['c_parameters'] = c_params

def check_params_with_large_only(func, mapping):
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
        if mapping['_name'].startswith('SMALL'):
            func['c_parameters'] = func['params_small']
        else:
            func['c_parameters'] = func['params_large']

def get_userbuffer_group(func, i):
    """internal function used by process_func_parameters"""
    p = func['c_parameters'][i]
    p2 = func['c_parameters'][i + 1]
    p3 = func['c_parameters'][i + 2]
    func_name = func['name']
    if RE.match(r'mpi_i?(alltoall|allgather|gather|scatter)', func_name, re.IGNORECASE):
        type = "inplace"
        if RE.search(r'send', p['name'], re.IGNORECASE) and RE.search(r'scatter', func_name, re.IGNORECASE):
            type = "noinplace"
        elif RE.search(r'recv', p['name'], re.IGNORECASE) and not RE.search(r'scatter', func_name, re.IGNORECASE):
            type = "noinplace"

        if RE.search(r'alltoallw', func_name, re.IGNORECASE):
            group_kind = "USERBUFFER-%s-w" % (type)
            group_count = 4
        elif p3['kind'] == "DATATYPE":
            group_kind = "USERBUFFER-%s" % (type)
            group_count = 3
        else:
            group_kind = "USERBUFFER-%s-v" % (type)
            group_count = 4
    elif RE.match(r'mpi_i?neighbor', func_name, re.IGNORECASE):
        if RE.search(r'alltoallw', func_name, re.IGNORECASE):
            group_kind = "USERBUFFER-neighbor-w"
            group_count = 4
        elif p3['kind'] == "DATATYPE":
            group_kind = "USERBUFFER-neighbor"
            group_count = 3
        else:
            group_kind = "USERBUFFER-neighbor-v"
            group_count = 4
    elif RE.match(r'mpi_i?(allreduce|reduce|scan|exscan)', func_name, re.IGNORECASE):
        group_kind = "USERBUFFER-reduce"
        group_count = 5
    elif RE.search(r'XFER_NUM_ELEM', p2['kind']) and RE.search(r'DATATYPE', p3['kind']):
        group_kind = "USERBUFFER-simple"
        group_count = 3
    else:
        group_kind, group_count = None, 0
    return (group_kind, group_count)

def process_func_parameters(func, mapping):
    """ Scan parameters and populate a few lists to facilitate generation."""
    # Note: we'll attach the lists to func at the end
    validation_list, handle_ptr_list, impl_arg_list, impl_param_list = [], [], [], []
    pointertag_list = []  # needed to annotate MPICH_ATTR_POINTER_WITH_TYPE_TAG

    func_name = func['name']
    n = len(func['c_parameters'])
    i = 0
    while i < n:
        p = func['c_parameters'][i]
        (group_kind, group_count) = ("", 0)
        if i + 3 <= n and RE.search(r'BUFFER', p['kind']):
            group_kind, group_count = get_userbuffer_group(func, i)
        if group_count > 0:
            t = ''
            for j in range(group_count):
                temp_p = func['c_parameters'][i + j]
                if t:
                    t += ","
                t += temp_p['name']
                impl_arg_list.append(temp_p['name'])
                impl_param_list.append(get_C_param(temp_p, mapping))
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

        if 'ANY' in func['_skip_validate'] or kind in func['_skip_validate'] or name in func['_skip_validate']:
            # -- user bypass --
            pass
        elif p['param_direction'] == 'out':
            # -- output parameter --
            validation_list.append({'kind': "ARGNULL", 'name': name})
            if RE.search(r'get_errhandler$', func_name):
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
                elif RE.match(r'mpi_(wait|test)', func_name, re.IGNORECASE):
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
            if RE.match(r'mpi_op_(free|commutative)', func_name, re.IGNORECASE):
                do_handle_ptr = 1
            elif RE.match(r'mpi_r?accumulate', func_name, re.IGNORECASE):
                validation_list.append({'kind': "OP_ACC", 'name': name})
            elif RE.match(r'mpi_(r?get_accumulate|fetch_and_op)', func_name, re.IGNORECASE):
                validation_list.append({'kind': "OP_GACC", 'name': name})
            else:
                validation_list.append({'kind': "op_and_ptr", 'name': name})
        elif kind == "MESSAGE" and p['param_direction'] == 'inout':
            do_handle_ptr = 1
        elif kind == "KEYVAL":
            if RE.search(r'_(set_attr|delete_attr|free_keyval)$', func_name):
                do_handle_ptr = 1
            else:
                validation_list.append({'kind': "KEYVAL", 'name': name})
        elif kind == "GREQUEST_CLASS":
            # skip validation for now
            pass
        elif kind in G.handle_mpir_types:
            do_handle_ptr = 1
            if kind == "INFO" and not RE.match(r'mpi_(info_.*|.*_set_info)$', func_name, re.IGNORECASE):
                p['can_be_null'] = "MPI_INFO_NULL"
            elif kind == "REQUEST" and RE.match(r'mpi_(wait|test|request_get_status)', func_name, re.IGNORECASE):
                p['can_be_null'] = "MPI_REQUEST_NULL"
        elif kind == "RANK" and name == "root":
            validation_list.insert(0, {'kind': "ROOT", 'name': name})
        elif RE.match(r'(COUNT|TAG)$', kind):
            validation_list.append({'kind': RE.m.group(1), 'name': name})
        elif RE.match(r'RANK(_NNI)?$', kind):
            validation_list.append({'kind': 'RANK', 'name': name})
        elif RE.match(r'(POLY)?(XFER_NUM_ELEM|DTYPE_NUM_ELEM_NNI|DTYPE_PACK_SIZE)', kind):
            validation_list.append({'kind': "COUNT", 'name': name})
        elif RE.match(r'(POLY)?DTYPE_NUM_ELEM', kind):
            if name != 'stride': # NOTE: fragile
                validation_list.append({'kind': "COUNT", 'name': name})
        elif RE.match(r'WINDOW_SIZE|WIN_ATTACH_SIZE', kind):
            validation_list.append({'kind': "WIN_SIZE", 'name': name})
        elif RE.match(r'ALLOC_MEM_NUM_BYTES', kind):
            validation_list.append({'kind': "ARGNEG", 'name': name})
        elif RE.match(r'(C_)?BUFFER', kind) and RE.match(r'MPI_Win_(allocate|create|attach)', func_name):
            validation_list.append({'kind': "WINBUFFER", 'name': name})
        elif RE.match(r'(POLY)?(RMA_DISPLACEMENT)', kind):
            if name == 'disp_unit':
                validation_list.append({'kind': "WIN_DISPUNIT", 'name': name})
            else:
                validation_list.append({'kind': "RMADISP", 'name': name})
        elif RE.match(r'(.*_NNI|ARRAY_LENGTH|INFO_VALUE_LENGTH|KEY_INDEX|INDEX|NUM_DIMS|DIMENSION|COMM_SIZE)', kind):
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
            if '_has_handle_out' in func:
                # Just MPI_Comm_idup[_with_info] have 2 handle output, but anyway...
                func['_has_handle_out'].append(p)
            else:
                func['_has_handle_out'] = [p]
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
            impl_param_list.append(get_C_param(p, mapping))
        i += 1

    if RE.match(r'MPI_(Wait|Test)$', func_name):
        func['_has_comm'] = "comm"
        func['_comm_from_request'] = 1

    func['need_validation'] = 0
    if len(validation_list):
        func['validation_list'] = validation_list
        func['need_validation'] = 1
    if len(handle_ptr_list):
        func['handle_ptr_list'] = handle_ptr_list
        func['need_validation'] = 1
    if 'code-error_check' in func:
        func['need_validation'] = 1
    func['impl_arg_list'] = impl_arg_list
    func['impl_param_list'] = impl_param_list
    if len(pointertag_list):
        func['pointertag_list'] = pointertag_list

# ---- simple parts ----

def dump_copy_right():
    G.out.append("/*")
    G.out.append(" * Copyright (C) by Argonne National Laboratory")
    G.out.append(" *     See COPYRIGHT in top-level directory")
    G.out.append(" */")
    G.out.append("")

def dump_profiling(func, mapping):
    func_name = get_function_name(func, mapping)
    G.out.append("/* -- Begin Profiling Symbol Block for routine %s */" % func_name)
    G.out.append("#if defined(HAVE_PRAGMA_WEAK)")
    G.out.append("#pragma weak %s = P%s" % (func_name, func_name))
    G.out.append("#elif defined(HAVE_PRAGMA_HP_SEC_DEF)")
    G.out.append("#pragma _HP_SECONDARY_DEF P%s  %s" % (func_name, func_name))
    G.out.append("#elif defined(HAVE_PRAGMA_CRI_DUP)")
    G.out.append("#pragma _CRI duplicate %s as P%s" % (func_name, func_name))
    G.out.append("#elif defined(HAVE_WEAK_ATTRIBUTE)")
    s = get_declare_function(func, mapping)
    dump_line_with_break(s, " __attribute__ ((weak, alias(\"P%s\")));" % (func_name))
    G.out.append("#endif")
    G.out.append("/* -- End Profiling Symbol Block */")

    G.out.append("")
    G.out.append("/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build")
    G.out.append("   the MPI routines */")
    G.out.append("#ifndef MPICH_MPI_FROM_PMPI")
    G.out.append("#undef %s" % func_name)
    G.out.append("#define %s P%s" % (func_name, func_name))
    G.out.append("")
    G.out.append("#endif")

def dump_manpage(func):
    func_name = func['name']
    G.out.append("/*@")
    G.out.append("   %s - %s" % (func_name, func['desc']))
    G.out.append("")
    lis_map = G.MAPS['LIS_KIND_MAP']
    for p in func['c_parameters']:
        lis_type = lis_map[p['kind']]
        if 'desc' not in p:
            if p['kind'] in G.default_descriptions:
                p['desc'] = G.default_descriptions[p['kind']]
            else:
                p['desc'] = p['name']
        p['desc'] += " (%s)" % (lis_type)

    input_list, output_list, inout_list = [], [], []
    for p in func['c_parameters']:
        if p['param_direction'] == 'out':
            output_list.append(p)
        elif p['param_direction'] == 'inout':
            inout_list.append(p)
        else:
            input_list.append(p)
    dump_manpage_list(input_list, "Input Parameters")
    dump_manpage_list(inout_list, "Input/Output Parameters")
    dump_manpage_list(output_list, "Output Parameters")

    # Add the custom notes (specified in e.g. pt2pt_api.txt) as is.
    if 'notes' in func:
        for l in func['notes']:
            G.out.append(l)
        G.out.append("")

    if 'replace' in func:
        if RE.match(r'\s*(deprecated|removed)', func['replace'], re.IGNORECASE):
            G.out.append(".N %s" % RE.m.group(1).capitalize())
        else:
            print("Missing reasons in %s .replace" % func_name, file=sys.stderr)

        if RE.search(r'with\s+(\w+)', func['replace']):
            G.out.append("   The replacement for this routine is '%s'." % RE.m.group(1))
        G.out.append("")

    for note in func['_docnotes']:
        G.out.append(".N %s" % note)
        if note == "Fortran":
            has = {}
            for p in func['c_parameters']:
                if p['kind'] == "status":
                    if p['length']:
                        has['FortStatusArray'] = 1
                    else:
                        has['FortranStatus'] = 1
            for k in has:
                G.out.append(".N %s" % k)
        G.out.append("")

    if 'notes2' in func:
        for l in func['notes2']:
            G.out.append(l)
        G.out.append("")

    if '_skip_err_codes' not in func:
        G.out.append("[ERR CODES]")
    G.out.append("")
    if 'seealso' in func:
        G.out.append(".seealso: %s" % func['seealso'])
    G.out.append("@*/")
    G.out.append("")

def dump_manpage_list(list, header):
    count = len(list)
    if count == 0:
        return
    G.out.append("%s:" % header)
    if count == 1:
        p = list[0]
        G.out.append(". %s - %s" % (p['name'], p['desc']))
    else:
        for i, p in enumerate(list):
            lead = "."
            if i == 0:
                lead = "+"
            elif i == count - 1:
                lead = "-"
            G.out.append("%s %s - %s" % (lead, p['name'], p['desc']))
    G.out.append("")

# ---- the function part ----
def dump_function(func, mapping, kind):
    """Appends to G.out array the MPI function implementation."""
    func_name = get_function_name(func, mapping);
    state_name = "MPID_STATE_" + func_name.upper()

    s = get_declare_function(func, mapping)
    if kind == "polymorph":
        (extra_param, extra_arg) = get_polymorph_param_and_arg(func['polymorph'])
        s = re.sub(r'MPIX?_', 'MPII_', s, 1)
        s = re.sub(r'\)$', ', '+extra_param+')', s)
        # prepare for the latter body of routines calling MPIR impl
        RE.search(r'(\w+)$', extra_param)
        func['impl_arg_list'].append(RE.m.group(1))
        func['impl_param_list'].append(extra_param)

    dump_line_with_break(s)
    G.out.append("{")
    G.out.append("INDENT")

    if "impl" in func and func['impl'] == "direct":
        # e.g. MPI_Aint_add
        dump_function_direct(func, state_name)
    elif kind == 'call-polymorph':
        (extra_param, extra_arg) = get_polymorph_param_and_arg(func['polymorph'])
        repl_name = re.sub(r'MPIX?_', 'MPII_', func_name, 1)
        repl_args = get_function_args(func) + ', ' + extra_arg
        repl_call = "mpi_errno = %s(%s);" % (repl_name, repl_args)
        dump_function_replace(func, state_name, repl_call)
    elif kind == 'call-replace':
        RE.search(r'with\s+(\w+)', func['replace'])
        repl_name = "P" + RE.m.group(1)
        repl_args = get_function_args(func)
        repl_call = "mpi_errno = %s(%s);" % (repl_name, repl_args)
        dump_function_replace(func, state_name, repl_call)
    else:
        dump_function_normal(func, state_name, mapping)

    G.out.append("DEDENT")
    G.out.append("}")

    if "decl" in func:
        push_impl_decl(func, func['decl'])
    elif 'impl' in func:
        if RE.match(r'topo_fns->', func['impl']):
            push_impl_decl(func)
    elif 'body' in func:
        pass
    else:
        push_impl_decl(func)

def dump_function_polymorph(func, mapping):
    """Same as dump_function, but use `MPII_` prefix and with extra parameters."""
    func_name = get_function_name(func, mapping);
    state_name = "MPID_STATE_" + func_name.upper()

    G.out.append(get_declare_function(func, mapping))
    G.out.append("{")
    G.out.append("INDENT")
    if 'replace' in func and 'body' not in func:
        if RE.search(r'with\s+(\w+)', func['replace']):
            dump_function_replace(func, state_name, "P%s" % RE.m.group(1))
    else:
        dump_function_normal(func, state_name, mapping)
    G.out.append("DEDENT")
    G.out.append("}")

def dump_function_normal(func, state_name, mapping):
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    if 'handle_ptr_list' in func:
        for p in func['handle_ptr_list']:
            dump_handle_ptr_var(func, p)
    if '_comm_from_request' in func:
        G.out.append("MPIR_Comm *comm_ptr = NULL;")
    if 'code-declare' in func:
        for l in func['code-declare']:
            G.out.append(l)

    G.out.append("MPIR_FUNC_TERSE_STATE_DECL(%s);" % state_name)

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
    G.out.append("MPIR_FUNC_TERSE_ENTER(%s);" % state_name)
    if 'handle_ptr_list' in func:
        G.out.append("")
        G.out.append("#ifdef HAVE_ERROR_CHECKING")
        G.out.append("{")
        G.out.append("    MPID_BEGIN_ERROR_CHECKS;")
        G.out.append("    {")
        G.out.append("INDENT")
        G.out.append("INDENT")
        for p in func['handle_ptr_list']:
            dump_validate_handle(func, p)
        G.out.append("DEDENT")
        G.out.append("DEDENT")
        G.out.append("    }")
        G.out.append("    MPID_END_ERROR_CHECKS;")
        G.out.append("}")
        G.out.append("#endif \x2f* HAVE_ERROR_CHECKING */")
        G.out.append("")
        for p in func['handle_ptr_list']:
            dump_convert_handle(func, p)
    if 'code-handle_ptr' in func:
        for l in func['code-handle_ptr']:
            G.out.append(l)
    if func['need_validation']:
        G.out.append("")
        G.out.append("#ifdef HAVE_ERROR_CHECKING")
        G.out.append("{")
        G.out.append("    MPID_BEGIN_ERROR_CHECKS;")
        G.out.append("    {")
        G.out.append("INDENT")
        G.out.append("INDENT")
        if 'handle_ptr_list' in func:
            for p in func['handle_ptr_list']:
                dump_validate_handle_ptr(func, p)
        if 'validation_list' in func:
            for t in func['validation_list']:
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
    G.out.append("/* ... body of routine ... */")
    if 'body' in func:
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

    G.out.append("/* ... end of body of routine ... */")
    G.out.append("")
    G.out.append("fn_exit:")
    for l in func['code-clean_up']:
        G.out.append(l)
    G.out.append("MPIR_FUNC_TERSE_EXIT(%s);" % state_name)

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
        dump_mpi_fn_fail(func, mapping)
    G.out.append("goto fn_exit;")

def push_impl_decl(func, impl_name=None):
    if not impl_name:
        impl_name = re.sub(r'^MPIX?_', 'MPIR_', func['name']) + "_impl"
    if func['impl_param_list']:
        params = ', '.join(func['impl_param_list'])
        if func['dir'] == 'coll' and not RE.match(r'MPI_I', func['name']):
            params = params + ", MPIR_Errflag_t *errflag"
    else:
        params="void"
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
    RE.match(r'MPI_(\w+)', func['name'])
    name = RE.m.group(1)

    args = ", ".join(func['impl_arg_list'])
    if name.startswith('I'):
        # non-blocking collectives
        G.out.append("MPIR_Request *request_ptr = NULL;")
        dump_line_with_break("mpi_errno = MPIR_%s(%s);" % (name, args))
        dump_error_check("")
        G.out.append("if (!request_ptr) {")
        G.out.append("    request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);")
        G.out.append("}")
        G.out.append("*request = request_ptr->handle;")
    else:
        # blocking collectives
        G.out.append("MPIR_Errflag_t errflag = MPIR_ERR_NONE;")
        dump_line_with_break("mpi_errno = MPIR_%s(%s, &errflag);" % (name, args))

def dump_body_topo_fns(func, method):
    comm_ptr = func['_has_comm'] + "_ptr"
    dump_if_open("%s->topo_fns && %s->topo_fns->%s" % (comm_ptr, comm_ptr, method))
    # The extension will output `MPI_Comm *` rather than `MPIR_Comm **`
    args = re.sub(r'&(\w+)_ptr', r'\1', ", ".join(func['impl_arg_list']))
    dump_line_with_break("mpi_errno = %s->topo_fns->%s(%s);" % (comm_ptr, method, args))
    dump_error_check("")
    dump_else()
    dump_body_impl(func)
    dump_if_close()

def dump_body_impl(func, prefix='mpir'):
    # mpi_errno = MPIR_Xxx_impl(...);
    if '_has_handle_out' in func:
        for p in func['_has_handle_out']:
            (name, kind) = (p['name'], p['kind'])
            mpir_type = G.handle_mpir_types[kind]
            G.out.append("%s *%s_ptr = NULL;" % (mpir_type, name))
            if kind in G.handle_NULLs:
                G.out.append("*%s = %s;" % (name, G.handle_NULLs[kind]))
    elif RE.match(r'mpi_type_', func['name'], re.IGNORECASE):
        p = func['c_parameters'][-1]
        if p['kind'] == "DATATYPE" and p['param_direction'] == 'out':
            G.out.append("*%s = MPI_DATATYPE_NULL;" % p['name'])

    if prefix == 'mpid':
        impl = func['name']
        impl = re.sub(r'^MPIX?_', 'MPID_', impl)
    else:
        impl = func['name'] + "_impl"
        impl = re.sub(r'^MPIX?_', 'MPIR_', impl)
    args = ", ".join(func['impl_arg_list'])
    dump_line_with_break("mpi_errno = %s(%s);" % (impl, args))
    dump_error_check("")

    if '_has_handle_out' in func:
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

def dump_function_replace(func, state_name, repl_call):
    G.out.append("int mpi_errno = MPI_SUCCESS;")

    G.out.append("MPIR_FUNC_TERSE_STATE_DECL(%s);" % state_name)
    G.out.append("MPIR_FUNC_TERSE_ENTER(%s);" % state_name)
    G.out.append("")
    G.out.append(repl_call)
    G.out.append("")
    G.out.append("MPIR_FUNC_TERSE_EXIT(%s);" % state_name)
    G.out.append("return mpi_errno;")

def dump_function_direct(func, state_name):
    for l in func['body']:
        G.out.append(l)

# -- fn_fail ----
def dump_mpi_fn_fail(func, mapping):
    G.out.append("/* --BEGIN ERROR HANDLINE-- */")

    if RE.match(r'mpi_(finalized|initialized)', func['name'], re.IGNORECASE):
        G.out.append("#ifdef HAVE_ERROR_CHECKING")
        cond = "MPIR_Errutil_is_initialized()"
        dump_if_open(cond)
        s = get_fn_fail_create_code(func, mapping)
        G.out.append(s)
        G.out.append("mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);")
        dump_if_close()
        G.out.append("#endif")
    else:
        G.out.append("#ifdef HAVE_ERROR_CHECKING")
        s = get_fn_fail_create_code(func, mapping)
        dump_line_with_break(s)
        G.out.append("#endif")
        if '_has_comm' in func:
            G.out.append("mpi_errno = MPIR_Err_return_comm(%s_ptr, __func__, mpi_errno);" % func['_has_comm'])
        elif '_has_win' in func:
            G.out.append("mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);")
        else:
            G.out.append("mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);")

    G.out.append("/* --END ERROR HANDLING-- */")

def get_fn_fail_create_code(func, mapping):
    s = "mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,"

    func_name = get_function_name(func, mapping)
    err_name = func_name.lower()

    (fmts, args, err_fmts) = ([], [], [])
    fmt_codes = {'RANK': "i", 'TAG': "t", 'COMMUNICATOR': "C", 'ASSERT': "A", 'DATATYPE': "D", 'ERRHANDLER': "E", 'FILE': "F", 'GROUP': "G", 'INFO': "I", 'OPERATION': "O", 'REQUEST': "R", 'WINDOW': "W", 'SESSION': "S", 'KEYVAL': "K", "GREQUEST_CLASS": "x"}
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
        G.out.append("MPIR_Request *request_ptr = MPIR_Request_create_complete(%s);" % request_kind)
        G.out.append("MPIR_ERR_CHKANDSTMT(request_ptr == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,")
        G.out.append("                    \"**nomemreq\");")
        G.out.append("*request = request_ptr->handle;")
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
        if RE.match(r'mpi_(test|wait)all', func['name'], re.IGNORECASE):
            # FIXME:we do not convert pointers for MPI_Testall and MPI_Waitall
            #       (for performance reasons). This probably this can be changed
            pass
        else:
            G.out.append("MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];")
            G.out.append("MPIR_Request **request_ptrs = request_ptr_array;")
    else:
        mpir = G.handle_mpir_types[kind]
        G.out.append("%s *%s_ptr = NULL;" % (mpir, name))

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
    elif kind == "KEYVAL":
        G.err_codes['MPI_ERR_KEYVAL'] = 1
        if RE.match(r'mpi_comm_', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_COMM, \"communicator\", mpi_errno);" % name)
        elif RE.match(r'mpi_type_', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_DATATYPE, \"datatype\", mpi_errno);" % name)
        elif RE.match(r'mpi_win_', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_WIN, \"window\", mpi_errno);" % name)

        if not RE.match(r'\w+_(get_attr)', func_name, re.IGNORECASE):
            G.out.append("MPIR_ERRTEST_KEYVAL_PERM(%s, mpi_errno);" % name)

def dump_convert_handle(func, p):
    (kind, name) = (p['kind'], p['name'])
    ptr_name = name + "_ptr"
    if '_pointer' in p:
        name = "*" + p['name']

    if kind == "REQUEST" and p['length']:
        if RE.match(r'mpi_(test|wait)all', func['name'], re.IGNORECASE):
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
        if RE.match(r'mpi_(test|wait)all', func['name'], re.IGNORECASE):
            # MPI_Testall and MPI_Waitall do pointer conversion inside MPIR_{Test,Wait}all
            pass
        else:
            ptr = p['_ptrs_name'] + '[i]'
            dump_for_open('i', p['length'])
            if RE.match(r'mpi_startall', func['name'], re.IGNORECASE):
                G.out.append("MPIR_Request_valid_ptr(%s, mpi_errno);" % ptr)
                dump_error_check("")
                G.out.append("MPIR_ERRTEST_PERSISTENT(%s, mpi_errno);" % ptr)
                G.out.append("MPIR_ERRTEST_PERSISTENT_ACTIVE(%s, mpi_errno);" % ptr)
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
            G.out.append("MPIR_ERRTEST_PERSISTENT(%s, mpi_errno);" % ptr_name)
            G.out.append("MPIR_ERRTEST_PERSISTENT_ACTIVE(%s, mpi_errno);" % ptr_name)

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
    elif RE.match(r'(ARGNULL)$', kind):
        if func['dir'] == 'mpit':
            G.err_codes['MPI_T_ERR_INVALID'] = 1
            G.out.append("MPIT_ERRTEST_ARGNULL(%s);" % name)
        else:
            G.err_codes['MPI_ERR_ARG'] = 1
            G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
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
    elif kind == "op_and_ptr":
        G.err_codes['MPI_ERR_OP'] = 1
        dump_validate_op(name, "")
    elif kind == 'KEYVAL':
        if RE.match('MPI_Comm_', func_name):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_COMM, \"%s\", mpi_errno);" % (name, name))
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
        dump_if_open("%s[i] != MPI_DATATYPE_NULL && !HANDLE_IS_BUILTIN(%s[i])" % (name, name))
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
        dump_validate_datatype(func, dt)
    G.out.append("MPIR_ERRTEST_COUNT(%s, mpi_errno);" % ct)
    G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (buf, ct, dt))
    dump_for_close()

def dump_validate_userbuffer_reduce(func, sbuf, rbuf, ct, dt, op):
    cond_intra = "comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM"
    cond_inter = "comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM"
    if RE.match(r'mpi_reduce_local$', func['name'], re.IGNORECASE):
        dump_validate_op(op, dt)
        dump_validate_datatype(func, dt)
        G.out.append("if (%s > 0) {" % ct)
        G.out.append("    MPIR_ERRTEST_ALIAS_COLL(%s, %s, mpi_errno);" % (sbuf, rbuf))
        G.out.append("}")
        G.out.append("MPIR_ERRTEST_NAMED_BUF_INPLACE(%s, \"%s\", %s, mpi_errno);" % (sbuf, sbuf, ct))
        G.out.append("MPIR_ERRTEST_NAMED_BUF_INPLACE(%s, \"%s\", %s, mpi_errno);" % (rbuf, rbuf, ct))
    elif RE.match(r'mpi_i?reduce$', func['name'], re.IGNORECASE):
        G.out.append("if (" + cond_intra + ") {")
        G.out.append("    MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);")
        G.out.append("} else {")
        G.out.append("    MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);")
        G.out.append("}")

        # exclude intercomm MPI_PROC_NULL
        G.out.append("if (" + cond_intra + " || root != MPI_PROC_NULL) {")
        G.out.append("INDENT")
        dump_validate_op(op, dt)
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
        G.out.append("if (" + cond_inter + ") {")
        G.out.append("    MPIR_ERRTEST_SENDBUF_INPLACE(%s, %s, mpi_errno);" % (sbuf, ct))
        G.out.append("}")
        G.out.append("if (%s > 0 && %s != MPI_IN_PLACE) {" % (ct, sbuf))
        G.out.append("    MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (sbuf, ct, dt))
        G.out.append("}")

        G.out.append("DEDENT")
        G.out.append("}")
    else:
        dump_validate_op(op, dt)
        dump_validate_datatype(func, dt)
        (sct, rct) = (ct, ct)
        if RE.search(r'reduce_scatter$', func['name'], re.IGNORECASE):
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
            dump_validate_datatype(func, dt)

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
    G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (buf, ct, dt))

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
        elif RE.search(r'i?(allgather|gather|scatter)(v?)$', func_name, re.IGNORECASE):
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
    G.out.append("    MPIR_Datatype *datatype_ptr = NULL;")
    G.out.append("    MPIR_Datatype_get_ptr(%s, datatype_ptr);" % dt)
    G.out.append("    MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);")
    dump_error_check("    ")
    if not RE.match(r'mpi_(type_|get_count|pack_external_size|status_set_elements)', func['name'], re.IGNORECASE):
        G.out.append("    MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);")
        dump_error_check("    ")
    G.out.append("}")

def dump_validate_op(op, dt):
    G.out.append("MPIR_ERRTEST_OP(%s, mpi_errno);" % op)
    G.out.append("if (!HANDLE_IS_BUILTIN(%s)) {" % op)
    G.out.append("    MPIR_Op *op_ptr = NULL;")
    G.out.append("    MPIR_Op_get_ptr(%s, op_ptr);" % op)
    G.out.append("    MPIR_Op_valid_ptr(op_ptr, mpi_errno);")
    if dt:
        G.out.append("} else {")
        G.out.append("    mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(%s)) (%s);" % (op, dt))
    G.out.append("}")
    dump_error_check("")

def dump_validate_get_comm_size(func):
    if '_got_comm_size' not in func:
        if RE.match(r'mpi_i?reduce_scatter', func['name'], re.IGNORECASE):
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

def get_function_name(func, mapping):
    if RE.search(r'BIG', mapping['_name']):
        return func['name'] + "_c"
    else:
        return func['name']

def get_function_args(func):
    arg_list = []
    for p in func['c_parameters']:
        arg_list.append(p['name'])
    return ', '.join(arg_list)

def get_declare_function(func, mapping):
    name = get_function_name(func, mapping)

    ret = "int"
    if 'return' in func:
        ret = mapping[func['return']]

    params = get_C_params(func, mapping)
    s_param = ', '.join(params)
    s = "%s %s(%s)" % (ret, name, s_param)
    return s

    G.out.append(s)

def get_C_params(func, mapping):
    param_list = []
    for p in func['c_parameters']:
        param_list.append(get_C_param(p, mapping))
    if not len(param_list):
        return ["void"]
    else:
        return param_list

def get_C_param(param, mapping):
    kind = param['kind']
    if kind == "VARARGS":
        return "..."

    want_star, want_bracket = '', ''
    param_type = mapping[kind]

    if param['func_type']:
        param_type = param['func_type']

    if not param_type:
        raise Exception("Type mapping [%s] %s not found!" % (mapping, kind))
    if not want_star:
        if is_pointer_type(param):
            if kind == "STRING_ARRAY":
                want_star = 1
                want_bracket = 1
            elif kind == "STRING_2DARRAY":
                want_star = 2
                want_bracket = 1
            elif kind == "ARGUMENT_LIST":
                want_star = 3
            elif param['pointer'] is not None and not param['pointer']:
                want_bracket = 1
            elif param['length'] and kind != "STRING":
                want_bracket = 1
            else:
                want_star = 1

    s = ''
    if param['constant']:
        s += "const "
    s += param_type

    if want_star:
        s += " " + "*" * want_star
    else:
        s += " "
    s += param['name']

    if want_bracket:
        s += "[]"
    if isinstance(param['length'], list):
        s += "[%s]" % param['length'][-1]

    return s

def get_polymorph_param_and_arg(s):
    # s is a polymorph spec, e.g. "MPIR_Attr_type attr_type=MPIR_ATTR_PTR"
    # Note: we assum limit of single extra param for now (which is sufficient)
    RE.match(r'^\s*(.+?)\s*(\w+)\s*=\s*(.+)', s)
    extra_param = RE.m.group(1) + ' ' + RE.m.group(2)
    extra_arg = RE.m.group(3)
    return (extra_param, extra_arg)

def is_pointer_type(param):
    if RE.match(r'(STRING\w*)$', param['kind']):
        return 1
    elif RE.match(r'(ATTRIBUTE_VAL\w*|(C_)?BUFFER\d?|STATUS|EXTRA_STATE\d*|TOOL_MPI_OBJ|(POLY)?FUNCTION\w*)$', param['kind']):
        return 1
    elif param['param_direction'] != 'in':
        return 1
    elif param['length']:
        return 1
    elif param['pointer']:
        return 1
    else:
        return 0

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

def split_line_with_break(s, tail, N=100):
    """Breaks a long line with proper indentations.
    This simplistic routine splits on ", ", thus only works with function declarations
    and simple function calls such as those generated by this script. """
    out_list = []

    tlist = []
    n = 0

    # by default, segments indent by additional 4 spaces
    if RE.match(r'(\s*)', s):
        n_lead = len(RE.m.group(1)) + 4

    if len(s) < N:
        tlist.append(s)
        n = len(s)
    elif RE.match(r'(.*?\()(.*)', s):
        # line with function pattern, match indent at openning parenthesis
        s_lead, s_next = RE.m.group(1,2)
        n_lead = len(s_lead)

        for a in s_next.split(', '):
            if n == 0:
                # first line
                tlist = [s_lead, a]
                n = n_lead + len(a)
            elif n + 2 + len(a) < N:
                # just append to tlist
                tlist.append(', ')
                tlist.append(a)
                n += 2 + len(a)
            else:
                # break the line
                tlist.append(',')
                out_list.append(''.join(tlist))
                # start new line with leading spaces
                tlist = [' ' * n_lead, a]
                n = n_lead + len(a)
        # leave last segment with tail
    else:
        # only break long function declaration or call for now
        tlist.append(s)
        n = len(s)

    # tail is mostly for "__attribute__ ((weak, alias(...))));",
    # which contains , that we do not desire to break
    if tail:
        if n + 1 + len(tail) < 100:
            out_list.append(''.join(tlist) + ' ' + tail)
        else:
            out_list.append(''.join(tlist))
            out_list.append(' ' * n_lead + tail)
    else:
        out_list.append(''.join(tlist))

    return out_list

def dump_line_with_break(s, tail=''):
    tlist = split_line_with_break(s, tail, 100)
    G.out.extend(tlist)
