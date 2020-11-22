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

def dump_mpi_c(func):
    """Dumps the function to C source file, e.g. MPI_Send -> pt2pt/send.c"""
    process_func_parameters(func)
    G.out = []
    G.err_codes = {}

    # -- "dump" accumulates output lines in G.out
    dump_copy_right()
    G.out.append("#include \"mpiimpl.h\"")
    G.out.append("")
    dump_profiling(func, "SMALL_C_KIND_MAP")
    G.out.append("")
    dump_manpage(func)
    dump_function(func, "SMALL_C_KIND_MAP")

    # -- decide file_path to write to
    file_path = ''
    if not os.path.isdir(func['dir']):
        os.mkdir(func['dir'])
    if RE.match(r'MPIX?_(\w+)', func['name'], re.I):
        name = RE.m.group(1)
        file_path = func['dir'] + '/' + name.lower() + ".c"
    else:
        raise Exception("Error in function name pattern: %s\n" % func['name'])

    # -- add to mpi_sources for dump_Makefile_mk()
    G.mpi_sources.append("src/binding/c/%s" % (file_path))

    # -- writing G.out to file_path
    print("  --> [%s]" % file_path)
    with open(file_path, "w") as Out:
        (indent, prev_empty) = (0, 0)
        for l in G.out:
            # stripping the newline for consistency
            l = l.rstrip()
            # handle special directives etc.
            if RE.match(r'\[ERR CODES\]', l):
                # man page error codes
                print(".N Errors", file=Out)
                print(".N MPI_SUCCESS\n", file=Out)
                for err in sorted (G.err_codes.keys()):
                    print(".N %s" % (err), file=Out)
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
                print("%s" % (l), file=Out)
                prev_empty = 0

def dump_Makefile_mk():
    print("  --> [%s]" % "Makefile.mk")
    with open("Makefile.mk", "w") as Out:
        print("##", file=Out)
        print("## Copyright (C) by Argonne National Laboratory", file=Out)
        print("##     See COPYRIGHT in top-level directory", file=Out)
        print("##", file=Out)
        print("", file=Out)
        print("mpi_sources += \\", file=Out)
        n = len(G.mpi_sources)
        for i, f in enumerate(G.mpi_sources):
            if i < n - 1:
                print("    %s \\" % f, file=Out)
            else:
                print("    %s" % f, file=Out)

# ---- pre-processing  ----

def process_func_parameters(func):
    """ Scan parameters and populate a few lists to facilitate generation."""
    # Note: we'll attach the lists to func at the end
    validation_list, handle_ptr_list, impl_arg_list = [], [], []

    func_name = func['name']
    n = len(func['params'])
    i = 0
    while i < n:
        p = func['params'][i]
        (group_kind, group_count) = ("", 0)
        if i + 3 <= n and RE.search(r'BUFFER', p['kind']):
            p2 = func['params'][i + 1]
            p3 = func['params'][i + 2]
            if RE.match(r'mpi_i?(alltoall|allgather|gather|scatter)', func_name, re.I):
                type = "inplace"
                if RE.search(r'send', p['name'], re.I) and RE.search(r'scatter', func_name, re.I):
                    type = "noinplace"
                elif RE.search(r'recv', p['name'], re.I) and not RE.search(r'scatter', func_name, re.I):
                    type = "noinplace"

                if RE.search(r'alltoallw', func_name, re.I):
                    group_kind = "USERBUFFER-%s-w" % (type)
                    group_count = 4
                elif p3['kind'] == "DATATYPE":
                    group_kind = "USERBUFFER-%s" % (type)
                    group_count = 3
                else:
                    group_kind = "USERBUFFER-%s-v" % (type)
                    group_count = 4
            elif RE.match(r'mpi_i?(allreduce|reduce|scan|exscan)', func_name, re.I):
                group_kind = "USERBUFFER-reduce"
                group_count = 5
            elif RE.search(r'XFER_NUM_ELEM', p2['kind']) and RE.search(r'DATATYPE', p3['kind']):
                group_kind = "USERBUFFER-simple"
                group_count = 3
        if group_count > 0:
            t = ''
            for j in range(group_count):
                a = func['params'][i + j]['name']
                if t:
                    t += ","
                t += a
                impl_arg_list.append(a)
            validation_list.append({'kind': group_kind, 'name': t})
            i += group_count
            continue
        do_handle_ptr = 0
        (kind, name) = (p['kind'], p['name'])
        if name == "comm":
            func['_has_comm'] = name

        if RE.search(r'direction=out', p['t'], re.I):
            validation_list.append({'kind': "ARGNULL", 'name': name})
        elif kind == "DATATYPE":
            if RE.match(r'mpi_type_(get|set|delete)_attr', func_name, re.I):
                do_handle_ptr = 1
            elif RE.match(r'mpi_(compare_and_swap)', func_name, re.I):
                validation_list.append({'kind': "TYPE_RMA_ATOMIC", 'name': name})
            else:
                validation_list.append({'kind': "datatype_and_ptr", 'name': name})
        elif kind == "OPERATION":
            if RE.match(r'mpi_r?accumulate', func_name, re.I):
                validation_list.append({'kind': "OP_ACC", 'name': name})
            elif RE.match(r'mpi_(r?get_accumulate|fetch_and_op)', func_name, re.I):
                validation_list.append({'kind': "OP_GACC", 'name': name})
            else:
                validation_list.append({'kind': "op_and_ptr", 'name': name})
        elif kind == "REQUEST":
            do_handle_ptr = 1
            if RE.match(r'mpi_start', func_name, re.I):
                if RE.search(r'length=', p['t']):
                    validation_list.append({'kind': "request-array-start", 'name': name})
                else:
                    validation_list.append({'kind': "request-start", 'name': name})
            elif RE.match(r'mpi_(wait|test)', func_name, re.I):
                if RE.search(r'length=', p['t']):
                    pass
                    do_handle_ptr = 0
                    validation_list.append({'kind': "request-array-wait", 'name': name})
        elif kind == "MESSAGE" and RE.search(r'direction=inout', p['t']):
            do_handle_ptr = 1
        elif RE.match(r'(COMMUNICATOR|GROUP|ERRHANDLER|OPERATION|INFO|WINDOW|KEYVAL|MESSAGE|SESSION|GREQ_CLASS)$', kind) and not RE.search(r'direction=out', p['t'], re.I):
            do_handle_ptr = 1
        elif kind == "RANK" and name == "root":
            validation_list.insert(0, {'kind': "ROOT", 'name': name})
        elif RE.match(r'(COUNT|RANK|TAG)$', kind):
            a = RE.m.group(1)
            validation_list.append({'kind': a, 'name': name})
        elif RE.match(r'(POLY)?(XFER_NUM_ELEM|DTYPE_NUM_ELEM_NNI)', kind):
            validation_list.append({'kind': "COUNT", 'name': name})
        elif RE.match(r'(RMA_DISPLACEMENT)', kind):
            validation_list.append({'kind': "DISP", 'name': name})
        elif RE.match(r'(.*_NNI)', kind):
            validation_list.append({'kind': "ARGNEG", 'name': name})
        elif RE.match(r'(.*_PI)', kind):
            validation_list.append({'kind': "ARGNONPOS", 'name': name})
        elif is_pointer_type(kind, p['t']):
            validation_list.append({'kind': "ARGNULL", 'name': name})
        elif 1:
            print("Missing error checking: %s" % kind, file=sys.stderr)

        if do_handle_ptr:
            handle_ptr_list.append(p)
            impl_arg_list.append(p['name'] + "_ptr")
        else:
            impl_arg_list.append(p['name'])
        i += 1

    func['need_validation'] = 0
    if len(validation_list):
        func['validation_list'] = validation_list
        func['need_validation'] = 1
    if len(handle_ptr_list):
        func['handle_ptr_list'] = handle_ptr_list
        func['need_validation'] = 1
    func['impl_arg_list'] = impl_arg_list

# ---- simple parts ----

def dump_copy_right():
    G.out.append("/*")
    G.out.append(" * Copyright (C) by Argonne National Laboratory")
    G.out.append(" *     See COPYRIGHT in top-level directory")
    G.out.append(" */")
    G.out.append("")

def dump_profiling(func, map_name):
    mapping = G.MAPS[map_name]
    func_name = func['name']
    if RE.search(r'BIG', map_name):
        func_name += "_l"
    G.out.append("/* -- Begin Profiling Symbol Block for routine %s */" % func_name)
    G.out.append("#if defined(HAVE_PRAGMA_WEAK)")
    G.out.append("#pragma weak %s = P%s" % (func_name, func_name))
    G.out.append("#elif defined(HAVE_PRAGMA_HP_SEC_DEF)")
    G.out.append("#pragma _HP_SECONDARY_DEF P%s  %s" % (func_name, func_name))
    G.out.append("#elif defined(HAVE_PRAGMA_CRI_DUP)")
    G.out.append("#pragma _CRI duplicate %s as P%s" % (func_name, func_name))
    G.out.append("#elif defined(HAVE_WEAK_ATTRIBUTE)")
    declare_function(func, mapping, " __attribute__ ((weak, alias(\"P%s\")));" % (func_name))
    G.out.append("#endif")
    G.out.append("/* -- End Profiling Symbol Block */")

    G.out.append("")
    G.out.append("/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build")
    G.out.append("   the MPI routines */")
    G.out.append("#ifndef MPICH_MPI_FROM_PMPI")
    G.out.append("#undef %s" % func_name)
    G.out.append("#define %s P%s" % (func_name, func_name))
    G.out.append("\n#endif")

def dump_manpage(func):
    func_name = func['name']
    G.out.append("/*@")
    G.out.append("   %s - %s" % (func_name, func['desc']))
    G.out.append("")
    lis_map = G.MAPS['LIS_KIND_MAP']
    for p in func['params']:
        lis_type = lis_map[p['kind']]
        if 'desc' not in p:
            if p['kind'] in G.default_descriptions:
                p['desc'] = G.default_descriptions[p['kind']]
            else:
                p['desc'] = p['name']
        p['desc'] += " (%s)" % (lis_type)

    input_list, output_list, inout_list = [], [], []
    for p in func['params']:
        if RE.search(r'direction=out', p['t']):
            output_list.append(p)
        elif RE.search(r'direction=inout', p['t']):
            inout_list.append(p)
        else:
            input_list.append(p)
    dump_manpage_list(inout_list, "Input/Output Parameters")
    dump_manpage_list(input_list, "Input Parameters")
    dump_manpage_list(output_list, "Output Parameters")

    # Add the custom notes (specified in e.g. pt2pt_api.txt) as is.
    if 'notes' in func:
        for l in func['notes']:
            G.out.append(l)

    (skip_thread_safe, skip_fortran) = (0, 0)
    if 'skip' in func:
        if RE.search(r'ThreadSafe', func['skip']):
            skip_thread_safe = 1
        if RE.search(r'Fortran', func['skip']):
            skip_fortran = 1

    if not skip_thread_safe:
        G.out.append(".N ThreadSafe")
        G.out.append("")
    if not skip_fortran:
        G.out.append(".N Fortran")
        has = {}
        for p in func['params']:
            if p['kind'] == "status":
                if RE.search(r'length=', p['t']):
                    has['FortStatusArray'] = 1
                else:
                    has['FortranStatus'] = 1
        for k in has:
            G.out.append(".N %s" % k)
        G.out.append("")

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
def dump_function(func, map_name):
    """Appends to G.out array the MPI function implementation."""
    mapping = G.MAPS[map_name]
    func_name = func['name']
    if RE.search(r'BIG', map_name):
        func_name += "_l"
    declare_function(func, mapping, "")
    G.out.append("{")
    G.out.append("INDENT")
    func['exit_routines'] = []
    G.out.append("int mpi_errno = MPI_SUCCESS;")
    if 'handle_ptr_list' in func:
        for p in func['handle_ptr_list']:
            dump_handle_ptr_var(func, p)

    state_name = "MPID_STATE_" + func_name.upper()
    G.out.append("MPIR_FUNC_TERSE_STATE_DECL(%s);" % state_name)

    G.out.append("")
    G.out.append("MPIR_ERRTEST_INITIALIZED_ORDIE();")
    G.out.append("")
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
    else:
        impl = func_name + "_impl"
        impl = re.sub(r'^MPIX?_', 'MPIR_', impl)
        args = ", ".join(func['impl_arg_list'])
        G.out.append("mpi_errno = %s(%s);" % (impl, args))
        dump_error_check("")

    G.out.append("/* ... end of body of routine ... */")
    G.out.append("")
    G.out.append("fn_exit:")
    for l in func['exit_routines']:
        G.out.append(l)
    G.out.append("MPIR_FUNC_TERSE_EXIT(%s);" % state_name)
    G.out.append("MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);")
    G.out.append("return mpi_errno;")
    G.out.append("")
    G.out.append("fn_fail:")
    dump_mpi_fn_fail(func, mapping)
    G.out.append("goto fn_exit;")
    G.out.append("DEDENT")
    G.out.append("}")

# -- fn_fail ----
def dump_mpi_fn_fail(func, mapping):
    G.out.append("/* --BEGIN ERROR HANDLINE-- */")
    G.out.append("#ifdef HAVE_ERROR_CHECKING")
    s = "mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,"

    err_name = func['name'].lower()
    if RE.search(r'BIG', mapping['_name']):
        err_name += "_l"

    (fmts, args) = ("", "")
    fmt_codes = {'RANK': "%i", 'TAG': "%t", 'COMMUNICATOR': "%C", 'ASSERT': "%A", 'DATATYPE': "%D", 'ERRHANDLER': "%E", 'FILE': "%F", 'GROUP': "%G", 'INFO': "%I", 'OPERATION': "%O", 'REQUEST': "%R", 'WINDOW': "%W", 'SESSION': "%S", 'KEYVAL': "%K"}
    for p in func['params']:
        kind = p['kind']
        name = p['name']
        if kind == "STRING" and not RE.search(r'direction=out', p['t']):
            fmts += " %s"
            args += ", %s" % (name)
        elif is_pointer_type(kind, p['t']):
            fmts += " %p"
            args += ", %s" % (name)
        elif kind in fmt_codes:
            fmts += " %s" % (fmt_codes[kind])
            args += ", %s" % (name)
        elif mapping[kind] == "int":
            fmts += " %d"
            args += ", %s" % (name)
        elif mapping[kind] == "MPI_Count":
            fmts += " %c"
            args += ", %s" % (name)
        else:
            print("Error format [%s] not found" % kind, file=sys.stderr)

    s += " \"**%s\", \"**%s%s\"%s);" % (err_name, err_name, fmts, args)

    G.out.append(s)
    G.out.append("#endif")
    comm = "0"
    if '_has_comm' in func:
        comm = "comm_ptr"
    G.out.append("mpi_errno = MPIR_Err_return_comm(%s, __func__, mpi_errno);" % comm)
    G.out.append("/* --END ERROR HANDLING-- */")

# -- early returns ----
def check_early_returns(func):
    if 'earlyreturn' in func:
        early_returns = func['earlyreturn'].split(',\s*')
        for kind in early_returns:
            if RE.search(r'pt2pt_proc_null', kind, re.I):
                dump_early_return_pt2pt_proc_null(func)

def dump_early_return_pt2pt_proc_null(func):
    check_rank, has_request, has_message, has_status, has_flag = '', '', '', '', ''
    for p in func['params']:
        kind = p['kind']
        name = p['name']
        if kind == "RANK":
            check_rank = name
        elif RE.search(r'direction=out', p['t']):
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
        if RE.search(r'mpi_.*(send|recv|probe)$', func['name'], re.I):
            a = RE.m.group(1)
            request_kind = "MPIR_REQUEST_KIND__" + a.upper()
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
    if kind == "REQUEST" and RE.search(r'length=(\w+)', p['t']):
        count = RE.m.group(1)
        calllen(request_array_var)
    elif kind == "COMMUNICATOR":
        # need init to NULL since MPIR_Err_return_comm will use it
        G.out.append("MPIR_Comm *comm_ptr = NULL;\n")
    else:
        mpir = G.handle_mpir_types[kind]
        G.out.append("%s *%s_ptr;" % (mpir, name))

def dump_validate_handle(func, p):
    func_name = func['name']
    (kind, name) = (p['kind'], p['name'])
    if RE.search(r'direction=inout', p['t']) and not RE.search(r'length=', p['t']):
        name = "*" + p['name']

    if kind == "COMMUNICATOR":
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
        if RE.search(r'length=(\w+)', p['t']):
            count = RE.m.group(1)
            G.out.append("if (count > 0) {")
            G.out.append("INDENT")
            G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
            G.out.append("for (int i = 0; i < %s; i++) {" % count)
            if RE.match(r'mpi_(wait|test)(all|any|some)$', func_name, re.I):
                G.out.append("    MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(%s[i], i, mpi_errno);" % name)
            else:
                G.out.append("    MPIR_ERRTEST_REQUEST(%s[i], mpi_errno);" % name)
            G.out.append("}")
            G.out.append("DEDENT")
            G.out.append("}")
        else:
            if RE.match(r'mpi_(request_get_status|wait|test)$', func_name, re.I):
                G.out.append("MPIR_ERRTEST_REQUEST_OR_NULL(%s, mpi_errno);" % name)
            else:
                G.out.append("MPIR_ERRTEST_REQUEST(%s, mpi_errno);" % name)
    elif kind == "MESSAGE":
        G.err_codes['MPI_ERR_REQUEST'] = 1
        G.out.append("MPIR_ERRTEST_REQUEST(%s, mpi_errno);" % name)
    elif kind == "DATATYPE":
        G.err_codes['MPI_ERR_TYPE'] = 1
        G.out.append("MPIR_ERRTEST_DATATYPE(%s, mpi_errno);" % name)
    elif kind == "OP":
        G.err_codes['MPI_ERR_OP'] = 1
        G.out.append("MPIR_ERRTEST_OP(%s, mpi_errno);" % name)
    elif kind == "INFO":
        G.err_codes['MPI_ERR_ARG'] = 1
        if RE.match(r'mpi_(info_.*|.*_set_info)$', func_name, re.I):
            G.out.append("MPIR_ERRTEST_INFO(%s, mpi_errno);" % name)
        else:
            G.out.append("MPIR_ERRTEST_INFO_OR_NULL(%s, mpi_errno);" % name)
    elif kind == "KEYVAL":
        G.err_codes['MPI_ERR_KEYVAL'] = 1
        if RE.match(r'mpi_comm_', func_name, re.I):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_COMM, \"communicator\", mpi_errno);" % name)
        elif RE.match(r'mpi_type_', func_name, re.I):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_DATATYPE, \"datatype\", mpi_errno);" % name)
        elif RE.match(r'mpi_win_', func_name, re.I):
            G.out.append("MPIR_ERRTEST_KEYVAL(%s, MPIR_WIN, \"window\", mpi_errno);" % name)

        if not RE.match(r'\w+_(get_attr)', func_name, re.I):
            G.out.append("MPIR_ERRTEST_KEYVAL_PERM(%s, mpi_errno);" % name)

def dump_convert_handle(func, p):
    (kind, name) = (p['kind'], p['name'])
    ptr_name = name + "_ptr"
    if RE.search(r'direction=inout', p['t']) and not RE.search(r'length=', p['t']):
        name = "*" + p['name']

    if kind == "REQUEST" and RE.search(r'length=(\w+)', p['t']):
        count = RE.m.group(1)
        G.out.append("if (%s > MPIR_REQUEST_PTR_ARRAY_SIZE) {" % count)
        G.out.append("    int nbytes = %s * sizeof(MPIR_Request *);" % count)
        G.out.append("    request_ptrs = (MPIR_Request **) MPL_malloc(nbytes, MPL_MEM_OBJECT);")
        G.out.append("    if (request_ptrs == NULL) {")
        G.out.append("        MPIR_CHKMEM_SETERR(mpi_errno, nbytes_, \"request pointers\");")
        G.out.append("        goto fn_fail;")
        G.out.append("    }")
        G.out.append("}")
        func['exit_routines'].append("if (%s > MPIR_REQUEST_PTR_ARRAY_SIZE) {" % (count))
        func['exit_routines'].append("    MPL_free(request_ptrs);")
        func['exit_routines'].append("}")

        G.out.append("\n")
        G.out.append("for (int i = 0; i < %s; i++) {" % count)
        G.out.append("    MPIR_Request_get_ptr(%s[i], request_ptrs[i]);" % name)
        G.out.append("}")
    else:
        mpir = G.handle_mpir_types[kind]
        G.out.append("%s_get_ptr(%s, %s);" % (mpir, name, ptr_name))

def dump_validate_handle_ptr(func, p):
    func_name = func['name']
    (kind, name) = (p['kind'], p['name'])
    mpir = G.handle_mpir_types[kind]
    if kind == "REQUEST" and RE.search(r'length=(\w+)', p['t']):
        count = RE.m.group(1)
        G.out.append("for (int i = 0; i < %s; i++) {" % count)
        G.out.append("    MPIR_Request_valid_ptr(request_ptrs[i], mpi_errno);")
        dump_error_check("    ")
        G.out.append("}")
    elif p['kind'] == "MESSAGE":
        G.out.append("if (*%s != MPI_MESSAGE_NO_PROC) {" % name)
        G.out.append("    MPIR_Request_valid_ptr(%s_ptr, mpi_errno);" % name)
        dump_error_check("    ")
        G.out.append("    MPIR_ERR_CHKANDJUMP((%s_ptr->kind != MPIR_REQUEST_KIND__MPROBE)," % name)
        G.out.append("                        mpi_errno, MPI_ERR_ARG, \"**reqnotmsg\");")
        G.out.append("}")
    elif kind == "COMMUNICATOR":
        ignore_revoke = 0
        if RE.match(r'mpi_comm_(create.*|i?dup.*|split.*|accept|connect|spawn.*|test_inter)$', func_name, re.I):
            ignore_revoke = 0
        elif RE.match(r'mpi_cart_(create.*|sub)$', func_name, re.I):
            ignore_revoke = 0
        elif RE.match(r'mpi_(comm_|cart_|cartdim_get|graph_get|graphdims_get|(dist_)?graph_neighbors_count|topo_test)$', func_name, re.I):
            ignore_revoke = 1

        if ignore_revoke:
            G.out.append("MPIR_Comm_valid_ptr(%s_ptr, mpi_errno, TRUE);" % name)
        else:
            G.out.append("MPIR_Comm_valid_ptr(%s_ptr, mpi_errno, FALSE);" % name)
        dump_error_check("")

        if RE.match(r'mpi_(i?exscan|i?scan|comm_spawn.*|.*_create)$', func_name, re.I):
            G.out.append("MPIR_ERRTEST_COMM_INTRA(%s_ptr, mpi_errno);" % name)
    else:
        G.out.append("%s_valid_ptr(%s_ptr, mpi_errno);" % (mpir, name))
        dump_error_check("")

        if kind == "WINDOW" and RE.match(r'mpi_win_shared_query', func_name, re.I):
            G.out.append("MPIR_ERRTEST_WIN_FLAVOR(win_ptr, MPI_WIN_FLAVOR_SHARED, mpi_errno);")

def dump_validation(func, t):
    func_name = func['name']
    (kind, name) = (t['kind'], t['name'])
    if kind == "RANK":
        G.err_codes['MPI_ERR_RANK'] = 1
        if RE.match(r'mpi_(i?m?probe|i?recv|recv_init)$', func_name, re.I) or name == "recvtag":
            G.out.append("MPIR_ERRTEST_RECV_RANK(comm_ptr, %s, mpi_errno);" % name)
        elif RE.match(r'(dest|target_rank)$', name) or RE.match(r'mpi_win_', func_name):
            G.out.append("MPIR_ERRTEST_SEND_RANK(comm_ptr, %s, mpi_errno);" % name)
        else:
            G.out.append("MPIR_ERRTEST_RANK(comm_ptr, %s, mpi_errno);" % name)
    elif kind == "TAG":
        G.err_codes['MPI_ERR_TAG'] = 1
        if RE.match(r'mpi_(i?m?probe|i?recv|recv_init)$', func_name, re.I) or name == "recvtag":
            G.out.append("MPIR_ERRTEST_RECV_TAG(%s, mpi_errno);" % name)
        else:
            G.out.append("MPIR_ERRTEST_SEND_TAG(%s, mpi_errno);" % name)
    elif kind == "ROOT":
        if RE.search(r'mpi_comm_', func_name, re.I):
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
    elif RE.search(r'DISPLACEMENT', kind):
        G.err_codes['MPI_ERR_DISP'] = 1
        G.out.append("MPIR_ERRTEST_DISP(%s, mpi_errno);" % name)
    elif RE.search(r'USERBUFFER', kind):
        p = name.split(',')
        if kind == "USERBUFFER-simple":
            dump_validate_userbuffer_simple(func, p[0], p[1], p[2])
        elif kind == "USERBUFFER-reduce":
            dump_validate_userbuffer_reduce(func, p[0], p[1], p[2], p[3], p[4])
        elif RE.search(r'-[vw]$', kind):
            dump_validate_userbuffer_coll(func, kind, p[0], p[1], p[3], p[2])
        else:
            dump_validate_userbuffer_coll(func, kind, p[0], p[1], p[2], "")
    elif RE.match(r'(ARGNULL)$', kind):
        G.err_codes['MPI_ERR_ARG'] = 1
        if name == "array_of_statuses":
            G.out.append("if (count > 0) {")
            G.out.append("INDENT")
            G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
            G.out.append("DEDENT")
            G.out.append("}")
        else:
            G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
    elif RE.match(r'(ARGNEG)$', kind):
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
        dump_validate_datatype(name)
    elif kind == "op_and_ptr":
        G.err_codes['MPI_ERR_OP'] = 1
        dump_validate_op(name, "")

    elif kind == "request-start":
        ptr = name + "_ptr"
        G.out.append("MPIR_ERRTEST_PERSISTENT(%s, mpi_errno);" % ptr)
        G.out.append("MPIR_ERRTEST_PERSISTENT_ACTIVE(%s, mpi_errno);" % ptr)

    elif kind == "request-array-start":
        G.out.append("for (int i = 0; i < count; i++) {")
        G.out.append("INDENT")
        ptr = name + "_ptrs[i]"
        G.out.append("MPIR_ERRTEST_PERSISTENT(%s, mpi_errno);" % ptr)
        G.out.append("MPIR_ERRTEST_PERSISTENT_ACTIVE(%s, mpi_errno);" % ptr)
        G.out.append("DEDENT")
        G.out.append("}")

    elif kind == "request-wait":
        G.out.append("MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
        G.out.append("MPIR_ERRTEST_REQUEST_OR_NULL(*%s, mpi_errno);" % name)

    elif kind == "request-array-wait":
        G.out.append("if (count > 0) {")
        G.out.append("    MPIR_ERRTEST_ARGNULL(%s, \"%s\", mpi_errno);" % (name, name))
        G.out.append("    for (int i = 0; i < count; i++) {")
        G.out.append("        MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(%s[i], i, mpi_errno);" % name)
        G.out.append("    }")
        G.out.append("}")

def dump_validate_userbuffer_simple(func, buf, ct, dt):
    G.out.append("MPIR_ERRTEST_COUNT(%s, mpi_errno);" % ct)
    G.out.append("if (%s > 0) {" % ct)
    G.out.append("INDENT")
    dump_validate_datatype(dt)
    G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (buf, ct, dt))
    G.out.append("DEDENT")
    G.out.append("}")

def dump_validate_userbuffer_reduce(func, sbuf, rbuf, ct, dt, op):
    dump_validate_datatype(dt)
    dump_validate_op(op, dt)
    (sct, rct) = (ct, ct)
    if RE.search(r'reduce_scatter$', func['name'], re.I):
        dump_validate_get_comm_size(func)
        G.out.append("int sum = 0;")
        G.out.append("for (int i = 0; i < comm_size; i++) {")
        G.out.append("    MPIR_ERRTEST_COUNT(%s[i], mpi_errno);" % ct)
        G.out.append("    sum += %s[i];" % ct)
        G.out.append("}")
        sct = "sum"
        rct = ct + "[comm_ptr->rank]"
    G.out.append("MPIR_ERRTEST_RECVBUF_INPLACE(%s, %s, mpi_errno);" % (rbuf, rct))
    G.out.append("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {")
    G.out.append("    MPIR_ERRTEST_SENDBUF_INPLACE(%s, %s, mpi_errno);" % (sbuf, sct))
    G.out.append("} else if (%s != MPI_IN_PLACE && %s != 0) {" % (sbuf, sct))
    G.out.append("    MPIR_ERRTEST_ALIAS_COLL(%s, %s, mpi_errno);" % (sbuf, rbuf))
    G.out.append("}")
    G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (rbuf, rct, dt))
    G.out.append("MPIR_ERRTEST_USERBUFFER(%s, sum, %s, mpi_errno);" % (sbuf, dt))

def dump_validate_userbuffer_coll(func, kind, buf, ct, dt, op):
    func_name = func['name']
    if RE.search(r'(gather|scatter)', func_name, re.I) and RE.search(r'-noinplace', kind):
        G.out.append("if (comm_ptr->rank == root) {")
        G.out.append("INDENT")

    if not RE.search(r'-w$', kind):
        dump_validate_datatype(dt)

    if RE.search(r'-[vw]$', kind):
        dump_validate_get_comm_size(func)
        G.out.append("for (int i = 0; i < comm_size; i++) {")
        G.out.append("INDENT")
        ct += '[i]'
        if RE.search(r'-w$', kind):
            dt += '[i]'
            dump_validate_datatype(dt)

    G.out.append("MPIR_ERRTEST_COUNT(%s, mpi_errno);" % ct)

    SEND = "SEND"
    if RE.search(r'recv', buf):
        SEND = "RECV"
    if RE.search(r'-noinplace', kind):
        G.out.append("MPIR_ERRTEST_%sBUF_INPLACE(%s, %s, mpi_errno);" % (SEND, buf, ct))
    else:
        G.out.append("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {")
        G.out.append("    MPIR_ERRTEST_%sBUF_INPLACE(%s, %s, mpi_errno);" % (SEND, buf, ct))
        G.out.append("}")
    G.out.append("MPIR_ERRTEST_USERBUFFER(%s, %s, %s, mpi_errno);" % (buf, ct, dt))

    if RE.search(r'-[vw]$', kind):
        G.out.append("DEDENT")
        G.out.append("}")

    if RE.search(r'-inplace', kind):
        cond = "comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM && %s != MPI_IN_PLACE" % (buf)
        if RE.search(r'(gather|scatter)', func_name, re.I):
            cond += " && comm_ptr->rank == root"
        G.out.append("if (%s) {" % cond)
        G.out.append("INDENT")
        if RE.search(r'i?(alltoall)', func_name, re.I):
            cond = "sendtype == recvtype && sendcount == recvcount && sendcount != 0"
            if RE.search(r'-v$', kind):
                cond = "sendtype == recvtype && sendcounts == recvcounts"
            elif RE.search(r'-w$', kind):
                cond = "sendtypes == recvtypes && sendcounts == recvcounts"
            G.out.append("if (%s) {" % cond)
            G.out.append("    MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);")
            G.out.append("}")
        elif RE.search(r'i?(allgather|gather|scatter)(v?)$', func_name, re.I):
            t1, t2 = RE.m.group(1, 2)
            cond = "sendtype == recvtype && sendcount == recvcount && sendcount != 0"
            (a, b) = ("send", "recv")
            if RE.match(r'scatter', t1, re.I):
                (a, b) = ("recv", "send")

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

    if RE.search(r'(gather|scatter)', func_name, re.I) and RE.search(r'-noinplace', kind):
        G.out.append("DEDENT")
        G.out.append("}")


def dump_validate_datatype(dt):
    G.out.append("MPIR_ERRTEST_DATATYPE(%s, \"datatype\", mpi_errno);" % dt)
    G.out.append("if (!HANDLE_IS_BUILTIN(%s)) {" % dt)
    G.out.append("    MPIR_Datatype *datatype_ptr = NULL;")
    G.out.append("    MPIR_Datatype_get_ptr(%s, datatype_ptr);" % dt)
    G.out.append("    MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);")
    dump_error_check("    ")
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
    if 'got_comm_size' not in func:
        G.out.append("int comm_size;")
        G.out.append("if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {")
        G.out.append("    comm_size = comm_ptr->remote_size;")
        G.out.append("} else {")
        G.out.append("    comm_size = comm_ptr->local_size;")
        G.out.append("}")
        func['got_comm_size'] = 1

# ---- supporting routines (reusable) ----

def declare_function(func, mapping, tail):
    name = func['name']
    if RE.search(r'BIG', mapping['_name']):
        name += "_l"

    ret = "int"
    if 'return' in func:
        ret = mapping[func['return']]

    params = get_C_params(func, mapping)
    s_param = ', '.join(params)
    s = "%s %s(%s)" % (ret, name, s_param)
    if tail:
        s += tail

    G.out.append(s)

def get_C_params(func, mapping):
    param_list = []
    for p in func['params']:
        if not RE.search(r'suppress=c_parameter', p['t']):
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

    if RE.search(r'func_type=(\w+)', param['t']):
        func_type = RE.m.group(1)
        param_type = func_type

    if not param_type:
        raise Exception("Type mapping [%s] %s not found!" % (mapping, kind))
    if not want_star:
        t = param['t']
        if is_pointer_type(kind, t):
            if kind == "STRING_ARRAY":
                want_star = 1
                want_bracket = 1
            elif kind == "STRING_2DARRAY":
                want_star = 2
                want_bracket = 1
            elif kind == "ARGUMENT_LIST":
                want_star = 3
            elif RE.search(r'pointer=False', t, re.I):
                want_bracket = 1
            elif RE.search(r'length=', t) and kind != "STRING":
                want_bracket = 1
            else:
                want_star = 1

    s = ''
    if RE.search(r'constant=True', param['t']):
        s += "const "
    s += param_type

    if want_star:
        s += " " + "*" * want_star
    else:
        s += " "
    s += param['name']

    if want_bracket:
        s += "[]"
    if RE.search(r'length=\[.*?, (\d+)\]', param['t']):
        n = RE.m.group(1)
        s += "[%s]" % (n)

    return s

def is_pointer_type(kind, t):
    if RE.match(r'(STRING\w*)$', kind):
        return 1
    elif RE.match(r'(ATTRIBUTE_VAL\w*|(C_)?BUFFER\d?|STATUS|EXTRA_STATE\d*|TOOL_MPI_OBJ|FUNCTION\w*)$', kind):
        return 1
    elif RE.search(r'direction=(in)?out', t, re.I):
        return 1
    elif RE.search(r'length=', t):
        return 1
    elif RE.search(r'pointer=True', t):
        return 1
    else:
        return 0

def dump_error_check(sp):
    G.out.append("%sif (mpi_errno) {" % sp)
    G.out.append("%s    goto fn_fail;" % sp)
    G.out.append("%s}" % sp)
