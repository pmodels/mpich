##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.binding_common import *
from local_python import RE

import re

def get_cdesc_name(func, is_large):
    name = re.sub(r'^MPIX?_', 'MPIR_', func['name'] + "_cdesc")
    if is_large:
        name += "_large"
    return name

def get_f08_c_name(func, is_large):
    name = re.sub(r'MPIX?_', r'MPIR_', func['name'] + '_c')
    if is_large:
        name += "_large"
    return name

def get_f08ts_name(func, is_large):
    name = func['name'] + "_f08ts"
    if is_large:
        name += "_large"
    return name

def get_f08_name(func, is_large):
    name = func['name'] + "_f08"
    if is_large:
        name += "_large"
    return name

def dump_f08_wrappers_c(func, is_large):
    c_mapping = get_kind_map('C', is_large)

    c_param_list = []
    c_arg_list = []
    vardecl_list = []
    code_list = []
    end_list = []

    def dump_buf(buf, check_in_place):
        c_param_list.append("CFI_cdesc_t *%s" % buf)
        vardecl_list.append("void *%s_i = %s->base_addr;" % (buf, buf))
        c_arg_list.append(buf + "_i")
        code_list.append("if (%s_i == &MPIR_F08_MPI_BOTTOM) {" % buf)
        code_list.append("    %s_i = MPI_BOTTOM;" % buf)
        if check_in_place:
            code_list.append("} else if (%s_i == &MPIR_F08_MPI_IN_PLACE) {" % buf)
            code_list.append("    %s_i = MPI_IN_PLACE;" % buf)
        code_list.append("}")
        code_list.append("")

    def dump_ct_dt(buf, ct, dt):
        ct_type = "int"
        if is_large:
            ct_type = "MPI_Count"
        c_param_list.append("%s %s" % (ct_type, ct))
        c_param_list.append("MPI_Datatype %s" % dt)
        vardecl_list.append("%s %s_i = %s;" % (ct_type, ct, ct))
        vardecl_list.append("MPI_Datatype %s_i = %s;" % (dt, dt))
        c_arg_list.append(ct + "_i")
        c_arg_list.append(dt + "_i")

        code_list.append("if (%s->rank != 0 && !CFI_is_contiguous(%s)) {" % (buf, buf))
        code_list.append("    err = cdesc_create_datatype(%s, %s, %s, &%s_i);" % (buf, ct, dt, dt))
        code_list.append("    %s_i = 1;" % ct)
        code_list.append("}")
        code_list.append("")

        end_list.append("if (%s_i != %s) PMPI_Type_free(&%s_i);" % (dt, dt, dt))

    def dump_p(p):
        c_param_list.append(get_C_param(p, func, c_mapping))
        c_arg_list.append(p['name'])

    n = len(func['parameters'])
    i = 0
    while i < n:
        p = func['parameters'][i]
        (group_kind, group_count) = ("", 0)
        if i + 3 <= n and RE.search(r'BUFFER', p['kind']):
            group_kind, group_count = get_userbuffer_group(func['name'], func['parameters'], i)

        if group_count > 0:
            p2 = func['parameters'][i + 1]
            p3 = func['parameters'][i + 2]
            if group_count == 3:
                dump_buf(p['name'], re.search('-inplace', group_kind))
                dump_ct_dt(p['name'], p2['name'], p3['name'])
            elif group_count == 4:
                p4 = func['parameters'][i + 3]
                # assume no sub-array buffer
                dump_buf(p['name'], re.search('-inplace', group_kind))
                dump_p(p2)
                dump_p(p3)
                dump_p(p4)
            elif group_count == 5:
                # reduce
                p4 = func['parameters'][i + 3]
                p5 = func['parameters'][i + 4]
                dump_buf(p['name'], True)
                dump_buf(p2['name'], False)
                # arbitrary: only recvbuf may be sub-array
                if RE.match(r'mpi_i?reduce_scatter', func['name'], re.IGNORECASE):
                    dump_p(p3)
                    dump_p(p4)
                else:
                    dump_ct_dt(p2['name'], p3['name'], p4['name'])
                dump_p(p5)

            i += group_count
            continue
        elif p['kind'] == "BUFFER":
            dump_buf(p['name'], False)
            i += 1
        else:
            dump_p(p)
            i += 1

    cdesc_name = get_cdesc_name(func, is_large)
    s = "int %s(%s)" % (cdesc_name, ', '.join(c_param_list))
    G.decls.append(s)
    tlist = split_line_with_break(s, "", 80)
    G.out.extend(tlist)
    G.out.append("{")
    if re.match(r'MPI_File_', func['name']):
        if is_large:
            # File large functions are not there yet
            G.out.append("    return MPI_ERR_INTERN;")
            G.out.append("}")
            return
        G.out.append("#ifndef MPI_MODE_RDONLY")
        G.out.append("    return MPI_ERR_INTERN;")
        G.out.append("#else")
    G.out.append("INDENT");
    G.out.append("int err = MPI_SUCCESS;")
    if re.match(r'MPI_F_sync_reg', func['name'], re.IGNORECASE):
        # dummy
        pass
    else:
        for l in vardecl_list:
            G.out.append(l)
        G.out.append("")
        for l in code_list:
            G.out.append(l)
        G.out.append("err = %s(%s);" % (get_function_name(func, is_large), ', '.join(c_arg_list)))
        G.out.append("")
        for l in end_list:
            G.out.append(l)
    G.out.append("return err;")
    G.out.append("DEDENT")
    if re.match(r'MPI_File_', func['name']):
        G.out.append("#endif")
    G.out.append("}")

def dump_f08_wrappers_f(func, is_large):
    c_mapping = get_kind_map('C', is_large)
    f08_mapping = get_kind_map('F08', is_large)

    f_param_list = []
    uses = {}
    f_decl_list = []
    c_decl_list = []
    arg_list_1 = []  # used if (c_int == kind(0))
    arg_list_2 = []  # used otherwise
    code_list = []
    convert_list_pre = []  # conversions always needed
    convert_list_post = []
    convert_list_1 = []    # conversions only if c_int != kind(0)
    convert_list_2 = []
    need_check_int_kind = False
    need_check_status_ignore = None # or p (the status parameter)
    has_comm_size = False  # arrays of length = comm_size
    status_var = ""
    status_count = ""
    is_alltoallvw = False

    if need_cdesc(func):
        f08ts_name = get_f08ts_name(func, is_large)
        c_func_name = get_cdesc_name(func, is_large)
    else:
        f08ts_name = get_f08_name(func, is_large)
        c_func_name = get_f08_c_name(func, is_large)
    uses[c_func_name] = 1

    if RE.match(r'MPI_(Init|Init_thread)$', func['name'], re.IGNORECASE):
        arg_list_1.append("c_null_ptr")
        arg_list_1.append("c_null_ptr")
        arg_list_2.append("c_null_ptr")
        arg_list_2.append("c_null_ptr")
        uses['c_null_ptr'] = 1
    elif RE.match(r'mpi_i?alltoall[vw]', func['name'], re.IGNORECASE):
        # Need check MPI_IN_PLACE in order to skip accessing sender arrays
        is_alltoallvw = True
        uses['c_loc'] = 1
        uses['c_associated'] = 1
        uses['MPI_IN_PLACE'] = 1

    # alltoallw inplace hack (since it is a corner case)
    def dump_alltoallvw_inplace(arg_list_1, arg_list_2, convert_list_2):
        # cannot use like sendcounts(1:length)
        if G.opts['fint-size'] == G.opts['cint-size']:
            if re.match(r'mpi_i?alltoallw', func['name'], re.IGNORECASE):
                send_args = "sendbuf, sendcounts, sdispls, sendtypes(1:1)%MPI_VAL"
                args1 = send_args + ", " + ', '.join(arg_list_1[4:])
            else:
                # alltoallv is fine
                args1 = ', '.join(arg_list_1)
            dump_fortran_line("ierror_c = %s(%s)" % (c_func_name, args1))
        else:
            args2 = ', '.join(arg_list_2)
            G.out.append("sendcounts_c = sendcounts(1:1)")
            G.out.append("sdispls_c = sdispls(1:1)")
            G.out.append("recvcounts_c = recvcounts(1:length)")
            G.out.append("rdispls_c = rdispls(1:length)")
            if re.match(r'mpi_i?alltoallw', func['name'], re.IGNORECASE):
                G.out.append("sendtypes_c = sendtypes(1:1)%MPI_VAL")
                G.out.append("recvtypes_c = recvtypes(1:length)%MPI_VAL")
            dump_fortran_line("ierror_c = %s(%s)" % (c_func_name, args2))
            G.out.extend(convert_list_2)

    # ----
    def process_integer(p):
        nonlocal need_check_int_kind

        # deal with user callbacks
        def set_grequest_lang(arg):
            # assume need_check_int_kind is False
            uses['MPI_SUCCESS'] = 1
            uses['MPIR_Grequest_set_lang_fortran'] = 1
            convert_list_2.append("IF (ierror_c == MPI_SUCCESS) THEN")
            convert_list_2.append("    call MPIR_Grequest_set_lang_fortran(%s)" % arg)
            convert_list_2.append("END IF")

        def set_attr_proxy(arg):
            # assume need_check_int_kind is False
            uses['MPI_SUCCESS'] = 1
            uses['MPII_Keyval_set_f90_proxy'] = 1
            convert_list_2.append("IF (ierror_c == MPI_SUCCESS) THEN")
            convert_list_2.append("    call MPII_Keyval_set_f90_proxy(%s)" % arg)
            convert_list_2.append("END IF")

        def check_proxy_requirement(func_name, p):
            if p['kind'] == "REQUEST" and RE.match(r'mpi_grequest_start', func_name, re.IGNORECASE):
                set_grequest_lang(arg_2)
                return True
            elif p['kind'] == "KEYVAL" and RE.match(r'mpi_(.*)_create_keyval', func_name, re.IGNORECASE):
                set_attr_proxy(arg_2)
                return True
            return False

        def info_get_string_buflen():
            convert_list_pre.append("IF (buflen > 0) THEN")
            convert_list_pre.append("    buflen_c = buflen + 1")
            convert_list_pre.append("ELSE")
            convert_list_pre.append("    buflen_c = 0")
            convert_list_pre.append("END IF")

        def info_get_valuelen():
            convert_list_2.append("IF (flag_c /= 0) THEN")
            convert_list_2.append("    valuelen = valuelen_c")
            convert_list_2.append("END IF")

        # ----
        if RE.match(r'TYPE\(MPIX?_\w+\)', f08_mapping[p['kind']], re.IGNORECASE):
            arg_1 = p['name'] + "%MPI_VAL"
        else:
            arg_1 = p['name']
        arg_2 = "%s_c" % p['name']
        if p['name'] == 'comm' and (has_comm_size or RE.match(r'mpi_cart_(rank|sub)', func['name'], re.IGNORECASE)):
            # already processed
            pass
        elif p['name'] == 'buflen' and func['name'] == "MPI_Info_get_string":
            # always use "buflen_c"
            arg_1 = arg_2
            info_get_string_buflen()
        elif p['name'] == 'valuelen' and func['name'] == "MPI_Info_get_valuelen":
            info_get_valuelen()
        else:
            need_check_int_kind = True
            if p['param_direction'] == 'in' or p['param_direction'] == 'inout':
                convert_list_1.append("%s = %s" % (arg_2, arg_1))
            if p['param_direction'] == 'out' or p['param_direction'] == 'inout':
                if check_proxy_requirement(func['name'], p):
                    # proxy doesn't work with the branches
                    need_check_int_kind = False

                convert_list_2.append("%s = %s" % (arg_1, arg_2))
        return (arg_1, arg_2)

    def process_mpi_file(p):
        arg = "%s_c" % p['name']
        if p['param_direction'] == 'in' or p['param_direction'] == 'inout':
            uses['MPI_File_f2c'] = 1
            convert_list_pre.append("%s = MPI_File_f2c(%s%%MPI_VAL)" % (arg, p['name']))
        if p['param_direction'] == 'out' or p['param_direction'] == 'inout':
            uses['MPI_File_c2f'] = 1
            convert_list_post.append("%s%%MPI_VAL = MPI_File_c2f(%s)" % (p['name'], arg))
        return (arg, arg)

    def process_logical(p):
        arg = "%s_c" % p['name']
        if p['param_direction'] == 'in' or p['param_direction'] == 'inout':
            convert_list_pre.append("IF (%s) THEN" % p['name'])
            convert_list_pre.append("    %s = 1" % arg)
            convert_list_pre.append("ELSE")
            convert_list_pre.append("    %s = 0" % arg)
            convert_list_pre.append("END IF")
        if p['param_direction'] == 'out' or p['param_direction'] == 'inout':
            convert_list_post.append("%s = (%s /= 0)" % (p['name'], arg))
        return (arg, arg)

    def process_index(p):
        arg = "%s_c" % p['name']
        if p['param_direction'] == 'in' or p['param_direction'] == 'inout':
            convert_list_pre.append("%s = %s - 1" % (arg, p['name']))
        if p['param_direction'] == 'out' or p['param_direction'] == 'inout':
            convert_list_post.append("%s = %s + 1" % (p['name'], arg))
        return (arg, arg)

    def process_string(p):
        arg = "%s_c" % p['name']

        def info_get_string_post():
            convert_list_post.append("IF (flag_c /= 0) THEN")
            # only update value when buflen /= 0
            convert_list_post.append("    IF (buflen /= 0) THEN")
            convert_list_post.append("        call MPIR_Fortran_string_c2f(%s, %s)" % (arg, p['name']))
            convert_list_post.append("    END IF")
            convert_list_post.append("    buflen = buflen_c - 1")
            convert_list_post.append("END IF")

        def info_get_post():
            convert_list_post.append("IF (flag_c /= 0) THEN")
            convert_list_post.append("    call MPIR_Fortran_string_c2f(%s, %s)" % (arg, p['name']))
            convert_list_post.append("END IF")

	# ----
        if p['param_direction'] == 'in':
            convert_list_pre.append("call MPIR_Fortran_string_f2c(%s, %s)" % (p['name'], arg))
            uses["MPIR_Fortran_string_f2c"] = 1
        else:
            if func['name'] == "MPI_Info_get_string":
                info_get_string_post()
            elif func['name'] == "MPI_Info_get":
                info_get_post()
            else:
                convert_list_post.append("call MPIR_Fortran_string_c2f(%s, %s)" % (arg, p['name']))
            uses["MPIR_Fortran_string_c2f"] = 1
        return (arg, arg)

    def process_status(p):
        nonlocal need_check_int_kind, need_check_status_ignore
        need_check_int_kind = True
        uses['c_loc'] = 1
        uses['c_associated'] = 1
        uses['assignment(=)'] = 1
        if p['length'] is not None: 
            # always output parameter
            uses['MPI_STATUSES_IGNORE'] = 1
            uses['MPIR_F08_get_MPI_STATUSES_IGNORE_c'] = 1
            need_check_status_ignore = p
            arg_1 = ":STATUS:"
            arg_2 = ":STATUS:"
            length = p['_array_length']
            if RE.match(r'mpix?_(test|wait)some', func['name'], re.IGNORECASE):
                length = "outcount_c"
            p['_status_convert'] = "%s(1:%s) = %s_c(1:%s)" % (p['name'], length, p['name'], length)
        else:
            arg_1 = "c_loc(status)"
            arg_2 = "c_loc(status_c)"
            if p['param_direction'] == 'out':
                need_check_status_ignore = p
                uses['MPI_STATUS_IGNORE'] = 1
                uses['MPIR_F08_get_MPI_STATUS_IGNORE_c'] = 1
                arg_1 = ":STATUS:"
                arg_2 = ":STATUS:"
                p['_status_convert'] = "status = status_c"
            elif p['param_direction'] == 'inout':
                convert_list_1.append("status_c = status")
                convert_list_2.append("status = status_c")
            else:
                convert_list_1.append("status_c = status")
        return (arg_1, arg_2)

    def process_array_check(p):
        nonlocal need_check_int_kind
        uses['c_loc'] = 1
        uses['c_ptr'] = 1
        arg_1 = "%s_cptr" % p['name']
        arg_2 = arg_1
        convert_list_pre.append("%s = c_loc(%s)" % (arg_1, p['name']))

        check = None
        if p['name'] == "argv":
            check = "MPI_ARGV_NULL"
        elif p['name'] == "array_of_argv":
            check = "MPI_ARGVS_NULL"
        elif RE.match(r'\w*weights', p['name']):
            # weights are input int array
            c_decl_list.append("LOGICAL :: has_%s = .false." % p['name'])
            check = "MPI_UNWEIGHTED"
            need_check_int_kind = True
            if RE.match(r'mpi_dist_graph_create$', func['name'], re.IGNORECASE):
                length = "sum(degrees)"
            elif RE.match(r'mpi_dist_graph_create_adjacent$', func['name'], re.IGNORECASE):
                if p['name'] == "sourceweights":
                    length = "indegree"
                else:
                    length = "outdegree"
            elif RE.match(r'mpi_dist_graph_neighbors$', func['name'], re.IGNORECASE):
                if p['name'] == "sourceweights":
                    length = "maxindegree"
                else:
                    length = "maxoutdegree"
            else:
                print("process_array_check: Unhandled %s" % p['name'])

            c_decl_list.append("INTEGER(c_int), TARGET :: %s_c(%s)" % (p['name'], length))
            convert_list_1.append("IF (has_%s) THEN" % p['name'])
            convert_list_1.append("    %s_cptr = c_loc(%s_c)" % (p['name'], p['name']))
            convert_list_1.append("END IF")
            # output conversion for MPI_Dist_graph_neighbors
            if p['param_direction'] == 'out':
                convert_list_2.append("IF (has_%s) THEN" % p['name'])
                convert_list_2.append("    %s(1:%s) = %s_c(1:%s)" % (p['name'], length, p['name'], length))
                convert_list_2.append("END IF")

        elif p['name'] == "array_of_errcodes":
            # errcodes are output int array
            c_decl_list.append("LOGICAL :: has_errcodes_ignore = .false.")
            check = "MPI_ERRCODES_IGNORE"
            need_check_int_kind = True
            if RE.match(r'mpi_comm_spawn_multiple', func['name'], re.IGNORECASE):
                length = "sum(array_of_maxprocs(1:count))"
            else: # mpi_comm_spawn
                length = "maxprocs"
            c_decl_list.append("INTEGER(c_int), TARGET :: %s_c(%s)" % (p['name'], length))
            convert_list_1.append("IF (.not. has_errcodes_ignore) THEN")
            convert_list_1.append("    %s_cptr = c_loc(%s_c)" % (p['name'], p['name']))
            convert_list_1.append("END IF")
            convert_list_2.append("IF (.not. has_errcodes_ignore) THEN")
            convert_list_2.append("    %s(1:%s) = %s_c" % (p['name'], length, p['name']))
            convert_list_2.append("END IF")
        else:
            print("Unhandled process_array_check")

        if check:
            uses['c_associated'] = 1
            uses[check] = 1
            uses['MPIR_F08_get_%s_c' % check] = 1
            convert_list_pre.append("IF (c_associated(%s, c_loc(%s))) THEN" % (arg_1, check))
            convert_list_pre.append("    %s = MPIR_F08_get_%s_c()" % (arg_1, check))
            if check == "MPI_ERRCODES_IGNORE":
                convert_list_pre.append("    has_errcodes_ignore = .true.")
            elif check == "MPI_UNWEIGHTED":
                # also need check MPI_WEIGHTS_EMPTY
                check = "MPI_WEIGHTS_EMPTY"
                uses[check] = 1
                uses['MPIR_F08_get_%s_c' % check] = 1
                convert_list_pre.append("ELSE IF (c_associated(%s, c_loc(%s))) THEN" % (arg_1, check))
                convert_list_pre.append("    %s = MPIR_F08_get_%s_c()" % (arg_1, check))
                convert_list_pre.append("ELSE")
                convert_list_pre.append("    %s = c_loc(%s)" % (arg_1, p['name']))
                convert_list_pre.append("    has_%s = .true." % p['name'])
            convert_list_pre.append("END IF")

        return (arg_1, arg_2)

    def process_array(p):
        nonlocal need_check_int_kind, has_comm_size

        if p['_array_convert'] == "MPI_VAL":
            need_check_int_kind = True
            if p['kind'] == "DATATYPE" and has_comm_size:
                # alltoallw types array
                arg_1 = "%s(1:length)%%MPI_VAL" % p['name']
            else:
                arg_1 = "%s%%MPI_VAL" % p['name']
            arg_2 = "%s_c" % p['name']
            if RE.match(r'in|inout', p['param_direction']):
                convert_list_1.append("%s = %s" % (arg_2, arg_1))
            if RE.match(r'out|inout', p['param_direction']):
                convert_list_2.append("%s = %s" % (arg_1, arg_2))
        elif p['_array_convert'] == "LOGICAL":
            arg_1 = "%s_c" % p['name']
            arg_2 = "%s_c" % p['name']
            if RE.match(r'in|inout', p['param_direction']):
                convert_list_pre.append("%s = merge(1, 0, %s)" % (arg_2, p['name']))
            if RE.match(r'out|inout', p['param_direction']):
                convert_list_post.append("%s = (%s /= 0)" % (p['name'], arg_2))
        elif p['_array_convert'] == "INDEX":
            arg_1 = "%s_c" % p['name']
            arg_2 = "%s_c" % p['name']
            if RE.match(r'MPI_(Wait|Test)some', func['name'], re.IGNORECASE):
                convert_list_post.append("%s(1:outcount) = %s(1:outcount) + 1" % (p['name'], arg_2))
            else:
                raise Exception("Unexpected function encountered in process_array: %s" % func['name'])
        elif RE.match(r'allocate:(.+)', p['_array_convert']):
            # The length variable name
            is_MPI_VAL = (RE.m.group(1) == 'MPI_VAL')
            length = "length"
            # get array length
            if p['_array_length'] == 'comm_size':
                need_check_int_kind = True
                if not has_comm_size:
                    if RE.search(r'alltoallw', func['name'], re.IGNORECASE):
                        # always need the length for types array
                        use_list = convert_list_pre
                    else:
                        use_list = convert_list_1
                    use_list.append("comm_c = comm%MPI_VAL")
                    if RE.search(r'neighbor', func['name'], re.IGNORECASE):
                        c_decl_list.append("INTEGER(c_int) :: err, indegree, outdegree, weighted")
                        use_list.append("err = MPIR_Dist_graph_neighbors_count_c(comm_c, indegree, outdegree, weighted)")
                        uses['MPIR_Dist_graph_neighbors_count_c'] = 1
                    else:
                        c_decl_list.append("INTEGER(c_int) :: err, length")
                        use_list.append("err = MPIR_Comm_size_c(comm_c, length)")
                        uses['MPIR_Comm_size_c'] = 1
                    has_comm_size = True
                if RE.search(r'neighbor', func['name'], re.IGNORECASE):
                    if RE.match(r'sendcounts|sdispls|sendtypes', p['name']):
                        length = "outdegree"
                    else:
                        length = "indegree"
            elif p['_array_length'] == 'cart_dim':
                # MPI_Cart_rank, only 1 allocatable array
                c_decl_list.append("INTEGER(c_int) :: err, length")
                use_list = convert_list_pre
                if RE.match(r'mpi_cart_rank', func['name'], re.IGNORECASE):
                    use_list = convert_list_2
                use_list.append("comm_c = comm%MPI_VAL")
                use_list.append("err = MPIR_Cartdim_get_c(comm_c, length)")
                uses['MPIR_Cartdim_get_c'] = 1
            else:
                print("process_array: Unhandled assumed array length")

            # set args
            if is_MPI_VAL:
                if p['kind'] == "DATATYPE":
                    arg_1 = "%s(1:%s)%%MPI_VAL" % (p['name'], length)
                else:
                    arg_1 = "%s%%MPI_VAL" % p['name']
                args_1 = "%s(1:%s)%%MPI_VAL" % (p['name'], length)
            else:
                arg_1 = p['name']
                args_1 = "%s(1:%s)" % (p['name'], length)
            arg_2 = "%s_c" % p['name']

            # convert
            if p['kind'] == "LOGICAL":
                convert_list_pre.append("%s = merge(1, 0, %s)" % (arg_2, args_1))
                arg_1 = arg_2
            else:
                convert_list_1.append("%s = %s" % (arg_2, args_1))
        elif p['_array_convert'] == "c_int":
            need_check_int_kind = True
            arg_1 = "%s" % p['name']
            arg_2 = "%s_c" % p['name']
            if p['_array_length']:
                argv_1 = arg_1 + "(1:%s)" % p['_array_length']
                argv_2 = arg_2 + "(1:%s)" % p['_array_length']
            else:
                argv_1 = arg_1
                argv_2 = arg_2
            if RE.match(r'in|inout', p['param_direction']):
                convert_list_1.append("%s = %s" % (argv_2, argv_1))
            if RE.match(r'out|inout', p['param_direction']):
                if RE.match(r'mpix?_(test|wait)some', func['name'], re.IGNORECASE) and p['name'] == "array_of_indices":
                    argv_1 = "array_of_indices(1:outcount_c)"
                    argv_2 = "array_of_indices_c(1:outcount_c)"
                convert_list_2.append("%s = %s" % (argv_1, argv_2))
        else:
            print("Unhandled process_array")

        return (arg_1, arg_2)

    def process_procedure(p):
        uses['c_funptr'] = 1
        uses['c_funloc'] = 1
        FN = get_F_procedure_type(p, is_large)
        uses[FN] = 1
        convert_list_pre.append("%s_c = c_funloc(%s)" % (p['name'], p['name']))
        if RE.match(r'mpi_register_datarep', func['name'], re.IGNORECASE) and RE.match(r'(read|write)_conversion_fn', p['name']):
            FN_NULL="MPI_CONVERSION_FN_NULL"
            if is_large:
                FN_NULL="MPI_CONVERSION_FN_NULL_C"
            convert_list_pre.append("IF (c_associated(%s_c, c_funloc(%s))) then" % (p['name'], FN_NULL))
            convert_list_pre.append("    %s_c = c_null_funptr" % p['name'])
            convert_list_pre.append("END IF")
            uses['c_associated'] = 1
            uses['c_null_funptr'] = 1
            uses[FN_NULL] = 1
        arg = "%s_c" % p['name']
        return (arg, arg)

    def post_string_len(v):
        c_decl_list.append("INTEGER(c_int) :: %s_len" % v)
        convert_list_pre.append("%s_len = len(%s)" % (v, v))
        arg_list_1.append("%s_len" % v)
        arg_list_2.append("%s_len" % v)

    # ----
    has_attribute_val = False
    for p in func['parameters']:
        if f08_param_need_skip(p, f08_mapping):
            continue
        if p['kind'] == "ATTRIBUTE_VAL":
            has_attribute_val = True
        f_param_list.append(p['name'])
        f_decl = get_F_decl(p, f08_mapping)
        if is_alltoallvw and p['name'] == 'sendbuf':
            f_decl = re.sub(r' ::', ', TARGET ::', f_decl)
        f_decl_list.append(f_decl)
        check_decl_uses(f_decl, uses)

        c_decl = get_F_c_decl(func, p, f08_mapping, c_mapping)
        if not c_decl:
            if p['kind'] == "STRING_ARRAY":
                arg_1 = "c_loc(%s)" % p['name']
                uses['c_loc'] = 1
            elif p['kind'] == "INFO":
                arg_1 = "%s(1:%s)%%MPI_VAL" % (p['name'], p['_array_length'])
            else:
                # no conversion needed, e.g. choice buffer, MPI_Aint, etc.
                arg_1 = p['name']
            arg_2 = arg_1
        else:
            c_decl_list.append(c_decl)
            check_decl_uses(c_decl, uses)
            if p['kind'] == "STRING":
                (arg_1, arg_2) = process_string(p)
            elif p['kind'] == "STATUS":
                (arg_1, arg_2) = process_status(p)
            elif '_array_length' in p: # set by get_F_c_decl(p)
                if p['_array_convert'] == 'c_ptr_check':
                    (arg_1, arg_2) = process_array_check(p)
                else:
                    (arg_1, arg_2) = process_array(p)
            elif p['kind'] == "LOGICAL" or p['kind'] == "LOGICAL_BOOLEAN":
                (arg_1, arg_2) = process_logical(p)
            elif p['kind'] == "INDEX" and re.match(r'MPI_(Test|Wait)any', func['name'], re.IGNORECASE):
                (arg_1, arg_2) = process_index(p)
            elif f08_mapping[p['kind']] == "PROCEDURE":
                (arg_1, arg_2) = process_procedure(p)
            elif p['kind'] == 'FILE':
                (arg_1, arg_2) = process_mpi_file(p)
            else:
                (arg_1, arg_2) = process_integer(p)
        arg_list_1.append(arg_1)
        arg_list_2.append(arg_2)

        if isinstance(p['length'], str) and RE.match(r'(MPI_\w+)', p['length'], re.IGNORECASE):
            uses[RE.m.group(1)] = 1

    # -- extra args for wrapper functions
    if has_attribute_val:
        uses["MPIR_ATTR_AINT"] = 1
        arg_list_1.append("MPIR_ATTR_AINT")
        arg_list_2.append("MPIR_ATTR_AINT")
    elif func['name'] == "MPI_Comm_spawn":
        post_string_len("argv")
    elif func['name'] == "MPI_Comm_spawn_multiple":
        post_string_len("array_of_commands")
        post_string_len("array_of_argv")

    # -- return
    if 'return' not in func:
        f_param_list.append("ierror")
        f_decl_list.append("INTEGER, OPTIONAL, INTENT(out) :: ierror")
        c_decl_list.append("INTEGER(c_int) :: ierror_c")
        uses['c_int'] = 1
    else:
        f_decl_list.append("%s :: res" % f08_mapping[func['return']])

    if need_check_int_kind:
        uses['c_int'] = 1

    # -- dump to G.out
    G.out.append("")
    if 'return' not in func:
        dump_fortran_line("SUBROUTINE %s(%s)" % (f08ts_name, ', '.join(f_param_list)))
    else:
        dump_fortran_line("FUNCTION %s(%s) result(res)" % (f08ts_name, ', '.join(f_param_list)))
    G.out.append("INDENT")
    dump_F_uses(uses)
    G.out.append("")
    G.out.append("IMPLICIT NONE")
    G.out.append("")
    G.out.extend(f_decl_list)
    G.out.append("")
    G.out.extend(c_decl_list)
    G.out.append("")
    if convert_list_pre:
        G.out.extend(convert_list_pre)
        G.out.append("")

    # ----
    def dump_call(s, check_int_kind):
        if need_check_status_ignore:
            p = need_check_status_ignore # the status parameter
            if p['length'] is None:
                ignore = 'MPI_STATUS_IGNORE'
            else:
                ignore = 'MPI_STATUSES_IGNORE'
            dump_F_if_open("c_associated(c_loc(%s), c_loc(%s))" % (p['name'], ignore))
            s2 = re.sub(r':STATUS:', "MPIR_F08_get_%s_c()" % ignore, s)
            dump_fortran_line(s2)
            dump_F_else()
            if check_int_kind:
                s2 = re.sub(r':STATUS:', "c_loc(%s_c)" % p['name'], s)
            else:
                s2 = re.sub(r':STATUS:', "c_loc(%s)" % p['name'], s)
            dump_fortran_line(s2)
            if check_int_kind:
                G.out.append(p['_status_convert'])
            dump_F_if_close()
        else:
            dump_fortran_line(s)

    # ----
    if 'return' not in func:
        ret = 'ierror_c'
    else:
        ret = 'res'

    if is_alltoallvw:
        dump_F_if_open("c_associated(c_loc(sendbuf), c_loc(MPI_IN_PLACE))")
        dump_alltoallvw_inplace(arg_list_1, arg_list_2, convert_list_2)
        dump_F_else()
    if need_check_int_kind and G.opts['fint-size'] == G.opts['cint-size']:
        dump_call("%s = %s(%s)" % (ret, c_func_name, ', '.join(arg_list_1)), False)
    else:
        G.out.extend(convert_list_1)
        dump_call("%s = %s(%s)" % (ret, c_func_name, ', '.join(arg_list_2)), True)
        G.out.extend(convert_list_2)

    if is_alltoallvw:
        dump_F_if_close()
    G.out.append("")

    if convert_list_post:
        G.out.extend(convert_list_post)
        G.out.append("")
    if 'return' not in func:
        G.out.append("IF (present(ierror)) ierror = ierror_c")
        G.out.append("DEDENT")
        G.out.append("END SUBROUTINE %s" % f08ts_name)
    else:
        G.out.append("DEDENT")
        G.out.append("END FUNCTION %s" % f08ts_name)

# -----
def dump_interface_module_open(module):
    G.out.append("module %s" % module)
    G.out.append("")
    G.out.append("IMPLICIT NONE")
    G.out.append("")
    G.out.append("INTERFACE")

def dump_interface_module_close(module):
    G.out.append("")
    G.out.append("END INTERFACE")
    G.out.append("END module %s" % module)

def dump_mpi_c_interface_cdesc(func, is_large):
    name = get_cdesc_name(func, is_large)
    dump_interface_function(func, name, name, is_large)

def dump_mpi_c_interface_nobuf(func, is_large):
    name = get_f08_c_name(func, is_large)
    if RE.match(r'mpi_(comm|type|win)_(set|get)_attr', func['name'], re.IGNORECASE):
        # use C wrapper functions exposed by C binding
        c_name = re.sub(r'MPIX?_', r'MPII_', func['name'])
        if is_large:
            c_name += "_large"
    elif RE.match(r'mpi_comm_spawn(_multiple)?$', func['name'], re.IGNORECASE):
        # use wrapper c functions
        c_name = name
    else:
        # uses PMPI c binding directly
        c_name = 'P' + get_function_name(func, is_large)
    dump_interface_function(func, name, c_name, is_large)

def dump_interface_function(func, name, c_name, is_large):
    c_mapping = get_kind_map('C', is_large)
    f08_mapping = get_kind_map('F08', is_large)

    uses = {}
    f_param_list = []
    decl_list = []
    # nearly always uses c_int
    uses['c_int'] = 1

    if RE.match(r'MPI_(Init|Init_thread)$', func['name'], re.IGNORECASE):
        # special, just special treat it
        f_param_list.append("argc")
        f_param_list.append("argv")
        decl_list.append("TYPE(c_ptr), VALUE, INTENT(in) :: argc")
        decl_list.append("TYPE(c_ptr), VALUE, INTENT(in) :: argv")
        uses['c_ptr'] = 1

    # ----
    for p in func['parameters']:
        if f08_param_need_skip(p, f08_mapping):
            continue
        f_param_list.append(p['name'])
        c_decl = get_F_c_interface_decl(func, p, f08_mapping, c_mapping)
        decl_list.append(c_decl)
        check_decl_uses(c_decl, uses)

    # -- extra parameters for wrapper functions
    if RE.match(r'MPII_\w+_(get|set)_attr', c_name):
        # MPII attribute wrapper functions
        f_param_list.append("attr_type")
        decl_list.append("INTEGER(kind(MPIR_ATTR_AINT)), VALUE, INTENT(in) :: attr_type")
        uses['MPIR_ATTR_AINT'] = 1
    elif func['name'] == "MPI_Comm_spawn":
        f_param_list.append("argv_elem_len")
        decl_list.append("INTEGER(c_int), VALUE, INTENT(in) :: argv_elem_len")
    elif func['name'] == "MPI_Comm_spawn_multiple":
        f_param_list.append("commands_elem_len")
        f_param_list.append("argv_elem_len")
        decl_list.append("INTEGER(c_int), VALUE, INTENT(in) :: commands_elem_len")
        decl_list.append("INTEGER(c_int), VALUE, INTENT(in) :: argv_elem_len")

    # -- return
    if 'return' not in func:
        ret = "ierror"
        decl_list.append("INTEGER(c_int) :: ierror")
    else:
        ret = "res"
        decl_list.append("%s :: res" % f08_mapping[func['return']])

    # ----
    G.out.append("")
    dump_fortran_line("FUNCTION %s(%s) &" % (name, ', '.join(f_param_list)))
    G.out.append("    bind(C, name=\"%s\") result(%s)" % (c_name, ret))
    G.out.append("INDENT")
    dump_F_uses(uses)
    G.out.append("IMPLICIT NONE")
    G.out.extend(decl_list)
    G.out.append("DEDENT")
    G.out.append("END FUNCTION %s" % name)

# dump the interface block in `mpi_f08.f90`
def dump_mpi_f08(func, is_large):
    f08_mapping = get_kind_map('F08', is_large)

    uses = {}
    f_param_list = []
    decl_list = []

    for p in func['parameters']:
        if f08_param_need_skip(p, f08_mapping):
            continue
        f_param_list.append(p['name'])
        decl = get_F_decl(p, f08_mapping)
        decl_list.append(decl)
        check_decl_uses(decl, uses)
    if 'return' not in func:
        f_param_list.append("ierror")
        decl_list.append("INTEGER, OPTIONAL, INTENT(out) :: ierror")
    else:
        decl_list.append("%s :: res" % f08_mapping[func['return']])

    # ----
    if need_cdesc(func):
        name = get_f08ts_name(func, is_large)
    else:
        name = get_f08_name(func, is_large)
    if 'return' not in func:
        dump_fortran_line("SUBROUTINE %s(%s)" % (name, ', '.join(f_param_list)))
    else:
        dump_fortran_line("FUNCTION %s(%s) result(res)" % (name, ', '.join(f_param_list)))
    G.out.append("INDENT")
    dump_F_uses(uses)
    G.out.append("IMPLICIT NONE")
    G.out.extend(decl_list)
    G.out.append("DEDENT")
    if 'return' not in func:
        G.out.append("END SUBROUTINE %s" % name)
    else:
        G.out.append("END FUNCTION %s" % name)

# -------------------------------
def f08_param_need_skip(p, mapping):
    if RE.search(r'suppress=.*f08_parameter', p['t']):
        return True
    if p['kind'] == 'VARARGS':
        return True
    if p['large_only'] and not mapping['_name'].startswith("BIG_"):
        return True
    return False

# check a type declaration' module requirement, sets keys in uses dictionary 
def check_decl_uses(decl, uses):
    if RE.match(r'TYPE\((\w+)\)', decl, re.IGNORECASE):
        uses[RE.m.group(1)] = 1
    elif RE.match(r'INTEGER\(KIND=(\w+)\)', decl, re.IGNORECASE):
        uses[RE.m.group(1)] = 1
    elif RE.match(r'INTEGER\((\w+)\)', decl, re.IGNORECASE):
        uses[RE.m.group(1)] = 1
    elif RE.match(r'CHARACTER\(KIND=(\w+)\)', decl, re.IGNORECASE):
        uses[RE.m.group(1)] = 1
    elif RE.match(r'CHARACTER\(len=(MPI_\w+)\)', decl, re.IGNORECASE):
        uses[RE.m.group(1)] = 1
    elif RE.match(r'procedure\((MPI_\w+)\)', decl, re.IGNORECASE):
        uses[RE.m.group(1)] = 1

def dump_F_uses(uses):
    iso_c_binding_list = []
    mpi_f08_list_1 = []  # mpi_f08_types
    mpi_f08_list_2 = []  # mpi_f08_compile_constants
    mpi_f08_list_3 = []  # mpi_f08_link_constants
    mpi_f08_list_4 = []  # mpi_f08_callbacks
    mpi_c_list_1 = []    # mpi_c_interface_types
    mpi_c_list_2 = []    # mpi_c_interfaces_{nobuf,cdesc}
    mpi_c_list_3 = []    # mpi_c_interface_glue
    for a in uses:
        if re.match(r'c_(int|char|ptr|loc|associated|null_ptr|null_funptr|funptr|funloc)', a, re.IGNORECASE):
            iso_c_binding_list.append(a)
        elif re.match(r'MPIR_ATTR_AINT|MPII_.*_proxy|MPIR.*set_lang|MPIR_.*string_(f2c|c2f)', a):
            mpi_c_list_3.append(a)
        elif re.match(r'MPI_\w+_(function|FN|FN_NULL)(_c)?$', a, re.IGNORECASE):
            mpi_f08_list_4.append(a)
        elif re.search(r'(STATUS.*IGNORE|ARGV.*NULL|ERRCODES_IGNORE|_UNWEIGHTED|WEIGHTS_EMPTY|MPI_IN_PLACE|MPI_BOTTOM)', a, re.IGNORECASE):
            mpi_f08_list_3.append(a)
        elif re.match(r'MPI_[A-Z_]+$', a):
            mpi_f08_list_2.append(a)
        elif re.match(r'MPIX?_\w+', a):
            mpi_f08_list_1.append(a)
        elif re.match(r'assignment', a):
            mpi_f08_list_1.append(a)
        elif re.match(r'c_\w+', a):
            mpi_c_list_1.append(a)
        else:
            mpi_c_list_2.append(a)
    if iso_c_binding_list:
        dump_fortran_line("USE, intrinsic :: iso_c_binding, ONLY : %s" % ', '.join(iso_c_binding_list))
    if mpi_f08_list_1:
        dump_fortran_line("USE :: mpi_f08_types, ONLY : %s" % ', '.join(mpi_f08_list_1))
    if mpi_f08_list_2:
        dump_fortran_line("USE :: mpi_f08_compile_constants, ONLY : %s" % ', '.join(mpi_f08_list_2))
    if mpi_f08_list_3:
        dump_fortran_line("USE :: mpi_f08_link_constants, ONLY : %s" % ', '.join(mpi_f08_list_3))
    if mpi_f08_list_4:
        dump_fortran_line("USE :: mpi_f08_callbacks, ONLY : %s" % ', '.join(mpi_f08_list_4))
    if mpi_c_list_1:
        dump_fortran_line("USE :: mpi_c_interface_types, ONLY : %s" % ', '.join(mpi_c_list_1))
    if mpi_c_list_2:
        dump_fortran_line("USE :: mpi_c_interface, ONLY : %s" % ', '.join(mpi_c_list_2))
    if mpi_c_list_3:
        dump_fortran_line("USE :: mpi_c_interface_glue, ONLY : %s" % ', '.join(mpi_c_list_3))

def dump_F_if_open(cond):
    G.out.append("IF (%s) THEN" % cond)
    G.out.append("INDENT")

def dump_F_else():
    G.out.append("DEDENT")
    G.out.append("ELSE")
    G.out.append("INDENT")

def dump_F_if_close():
    G.out.append("DEDENT")
    G.out.append("END IF")

def dump_F_module_open(name):
    G.out.append("MODULE %s" % name)
    G.out.append("INDENT")

def dump_F_module_close(name):
    G.out.append("DEDENT")
    G.out.append("END MODULE %s" % name)

def dump_fortran_line(s):
    tlist = split_line_with_break(s, '', 100)
    n = len(tlist)
    if n > 1:
        for i in range(n-1):
            tlist[i] = tlist[i] + ' &'
    G.out.extend(tlist)

# ---- mpi_f08_types.f90 -------------------------
G.f08_sizeof_list = ["character", "logical", "xint8", "xint16", "xint32", "xint64", "xreal32", "xreal64", "xreal128", "xcomplex32", "xcomplex64", "xcomplex128"]

def dump_mpi_f08_types():
    def dump_status_type():
        # Status need be consistent with mpi.h
        G.out.append("")
        G.out.append("TYPE, bind(C) :: MPI_Status")
        for field in G.status_fields:
            G.out.append("    INTEGER :: %s" % field)
        G.out.append("END TYPE MPI_Status")
        G.out.append("")
        G.out.append("INTEGER, parameter :: MPI_SOURCE = 3")
        G.out.append("INTEGER, parameter :: MPI_TAG    = 4")
        G.out.append("INTEGER, parameter :: MPI_ERROR  = 5")
        G.out.append("INTEGER, parameter :: MPI_STATUS_SIZE = 5")

    def dump_status_interface():
        G.out.append("")
        G.out.append("INTERFACE assignment(=)")
        G.out.append("    module procedure MPI_Status_f08_assign_c")
        G.out.append("    module procedure MPI_Status_c_assign_f08")
        G.out.append("END INTERFACE")
        G.out.append("")
        G.out.append("private :: MPI_Status_f08_assign_c")
        G.out.append("private :: MPI_Status_c_assign_f08")
        G.out.append("private :: MPI_Status_f_assign_c")
        G.out.append("private :: MPI_Status_c_assign_f")

    def dump_status_routines():
        # declare variable name of status in f, c, or f08
        def dump_decl(intent, t, name):
            if t == 'f':
                G.out.append("INTEGER, INTENT(%s) :: %s(MPI_STATUS_SIZE)" % (intent, name))
            elif t == 'c':
                G.out.append("TYPE(c_Status), INTENT(%s) :: %s" % (intent, name))
            else:
                G.out.append("TYPE(MPI_Status), INTENT(%s) :: %s" % (intent, name))

        # phrase of individual status field
        def field(t, name, idx):
            if t == 'f':
                if idx < 2:
                    return "%s(%d)" % (name, idx + 1)
                else:
                    return "%s(%s)" % (name, G.status_fields[idx])
            else:
                return "%s%%%s" % (name, G.status_fields[idx])

        # body of the status conversion routines
        def dump_convert(in_type, in_name, out_type, out_name, res):
            dump_decl("in", in_type, in_name)
            dump_decl("out", out_type, out_name)
            if res == "ierror":
                G.out.append("INTEGER, OPTIONAL, INTENT(out) :: ierror")
            elif res == "res":
                G.out.append("INTEGER(c_int) :: res")

            G.out.append("")
            if in_type == "f" or out_type == "f" or res is None:
                for i in range(5):
                    G.out.append("%s = %s" % (field(out_type, out_name, i), field(in_type, in_name, i)))
            else:
                G.out.append("%s = %s" % (out_name, in_name))

            if res == "ierror":
                G.out.append("IF (present(ierror)) ierror = 0")
            elif res == "res":
                G.out.append("res = 0")

        # e.g. MPI_Status_f08_assign_c
        def dump_convert_assign(in_type, out_type):
            G.out.append("")
            in_name = "status_%s" % in_type
            out_name = "status_%s" % out_type

            if in_type != 'f' and out_type != 'f':
                G.out.append("elemental SUBROUTINE MPI_Status_%s_assign_%s(%s, %s)" % (out_type, in_type, out_name, in_name))
            else:
                G.out.append("SUBROUTINE MPI_Status_%s_assign_%s(%s, %s)" % (out_type, in_type, out_name, in_name))
            G.out.append("INDENT")
            dump_convert(in_type, in_name, out_type, out_name, None)
            G.out.append("DEDENT")
            G.out.append("END SUBROUTINE")

        # e.g. MPI_Status_f082f
        def dump_convert_2(in_type, out_type):
            G.out.append("")
            in_name = "%s_status" % in_type
            out_name = "%s_status" % out_type

            G.out.append("SUBROUTINE MPI_Status_%s2%s(%s, %s, ierror)" % (in_type, out_type, in_name, out_name))
            G.out.append("INDENT")
            dump_convert(in_type, in_name, out_type, out_name, "ierror")
            G.out.append("DEDENT")
            G.out.append("END SUBROUTINE")

        # e.g. MPI_Status_f082c
        def dump_convert_mpi(in_type, out_type, prefix):
            G.out.append("")
            # open
            mpi_name = "%s_Status_%s2%s" % (prefix, in_type, out_type)
            in_name = "status_%s" % in_type
            out_name = "status_%s" % out_type

            G.out.append("FUNCTION %s(%s, %s) &" % (mpi_name, in_name, out_name))
            G.out.append("        bind(C, name=\"%s\") result (res)" % mpi_name)
            G.out.append("INDENT")
            G.out.append("USE, intrinsic :: iso_c_binding, ONLY: c_int")
            dump_convert(in_type, in_name, out_type, out_name, "res")
            G.out.append("DEDENT")
            G.out.append("END FUNCTION %s" % mpi_name)

        # ----
        dump_convert_2("f08", "f")
        dump_convert_2("f", "f08")
        dump_convert_assign("f08", "c")
        dump_convert_assign("c", "f08")
        dump_convert_assign("f", "c")
        dump_convert_assign("c", "f")
        for prefix in ["MPI", "PMPI"]:
            dump_convert_mpi("f08", "c", prefix)
            dump_convert_mpi("c", "f08", prefix)

    def dump_handle_types():
        for a in G.handle_list:
            G.out.append("")
            G.out.append("TYPE, bind(C) :: %s" % a)
            G.out.append("    INTEGER :: MPI_VAL")
            G.out.append("END TYPE %s" % a)

    def dump_handle_interface():
        for op in ["eq", "neq"]:
            if op == "eq":
                op_sym = "=="
            else:
                op_sym = "/="
            G.out.append("")
            G.out.append("INTERFACE operator(%s)" % op_sym)
            for a in G.handle_list:
                G.out.append("    module procedure %s_%s" % (a, op))
                G.out.append("    module procedure %s_f08_%s_f" % (a, op))
                G.out.append("    module procedure %s_f_%s_f08" % (a, op))
            G.out.append("END INTERFACE")
            G.out.append("")
            for a in G.handle_list:
                G.out.append("private :: %s_%s" % (a, op))
                G.out.append("private :: %s_f08_%s_f" % (a, op))
                G.out.append("private :: %s_f_%s_f08" % (a, op))

    def dump_handle_routines():
        for op in ["eq", "neq"]:
            G.out.append("")
            for a in G.handle_list:
                # e.g. MPI_Comm_eq
                G.out.append("")
                G.out.append("elemental FUNCTION %s_%s(x, y) result(res)" % (a, op))
                G.out.append("    TYPE(%s), INTENT(in) :: x, y" % a)
                G.out.append("    LOGICAL :: res")
                if op == "eq":
                    G.out.append("    res = (x%MPI_VAL == y%MPI_VAL)")
                else:
                    G.out.append("    res = (x%MPI_VAL /= y%MPI_VAL)")
                G.out.append("END FUNCTION %s_%s" % (a, op))
                # e.g. MPI_Comm_f08_eq_f, MPI_Comm_f_eq_f08
                G.out.append("")
                for p in [("f08", "f"), ("f", "f08")]:
                    func_name = "%s_%s_%s_%s" % (a, p[0], op, p[1])
                    G.out.append("")
                    G.out.append("elemental FUNCTION %s(%s, %s) result(res)" % (func_name, p[0], p[1]))
                    G.out.append("    TYPE(%s), INTENT(in) :: f08" % a)
                    G.out.append("    INTEGER, INTENT(in) :: f")
                    G.out.append("    LOGICAL :: res")
                    if op == "eq":
                        G.out.append("    res = (f08%MPI_VAL == f)")
                    else:
                        G.out.append("    res = (f08%MPI_VAL /= f)")
                    G.out.append("END FUNCTION %s" % func_name)
        # e.g. MPI_Comm_f2c
        for a in G.handle_list:
            if a == "MPI_File":
                continue
            if RE.match(r'MPIX?_(\w+)', a):
                c_name = "c_" + RE.m.group(1)
            for p in [("f", "c"), ("c", "f")]:
                func_name = "%s_%s2%s" % (a, p[0], p[1])
                G.out.append("")
                G.out.append("FUNCTION %s(x) result(res)" % func_name)
                G.out.append("    USE mpi_c_interface_types, ONLY: %s" % c_name)
                if p[0] == "f":
                    G.out.append("    INTEGER, VALUE :: x")
                    G.out.append("    INTEGER(%s) :: res" % c_name)
                else:
                    G.out.append("    INTEGER(%s), VALUE :: x" % c_name)
                    G.out.append("    INTEGER :: res")
                G.out.append("    res = x")
                G.out.append("END FUNCTION %s" % func_name)

    def dump_file_interface():
        G.out.append("")
        G.out.append("INTERFACE")
        G.out.append("INDENT")
        for p in [("f", "c"), ("c", "f")]:
            func_name = "MPI_File_%s2%s" % (p[0], p[1])
            G.out.append("")
            G.out.append("FUNCTION %s(x) bind(C, name=\"%s\") result(res)" % (func_name, func_name))
            G.out.append("    USE mpi_c_interface_types, ONLY: c_File")
            if p[0] == "f":
                G.out.append("    INTEGER, VALUE :: x")
                G.out.append("    INTEGER(c_File) :: res")
            else:
                G.out.append("    INTEGER(c_File), VALUE :: x")
                G.out.append("    INTEGER :: res")
            G.out.append("END FUNCTION MPI_File_%s2%s" % (p[0], p[1]))
        G.out.append("DEDENT")
        G.out.append("END INTERFACE")

    def filter_f08_sizeof_list():
        if "no-real128" in G.opts:
            G.f08_sizeof_list = [a for a in G.f08_sizeof_list if not a.endswith("128")]

    def dump_sizeof_interface():
        G.out.append("")
        G.out.append("INTERFACE MPI_Sizeof")
        for a in G.f08_sizeof_list:
            G.out.append("    module procedure MPI_Sizeof_%s" % a)
        G.out.append("END INTERFACE")
        G.out.append("")
        for a in G.f08_sizeof_list:
            G.out.append("private :: MPI_Sizeof_%s" % a)

    def dump_sizeof_routines():
        for a in G.f08_sizeof_list:
            G.out.append("")
            G.out.append("SUBROUTINE MPI_Sizeof_%s(x, size, ierror)" % a)
            G.out.append("INDENT")
            if RE.match(r'x(\w+?)(\d+)', a):
                if RE.m.group(1) == 'int':
                    t = 'int%s' % RE.m.group(2)
                    T = "INTEGER(%s)" % t
                else:
                    t = 'real%s' % RE.m.group(2)
                    T = "%s(%s)" % (RE.m.group(1), t)
                G.out.append("USE, intrinsic :: iso_fortran_env, ONLY: %s" % t)
                G.out.append("%s, dimension(..) :: x" % T)
            else:
                G.out.append("%s, dimension(..) :: x" % a)
            G.out.append("INTEGER, INTENT(out) :: size")
            G.out.append("INTEGER, OPTIONAL, INTENT(out) :: ierror")
            G.out.append("")
            G.out.append("size = storage_size(x)/8")
            G.out.append("IF (present(ierror)) ierror = 0")
            G.out.append("DEDENT")
            G.out.append("END SUBROUTINE")

    # ----
    # optionally filter real128
    filter_f08_sizeof_list()

    dump_F_module_open("mpi_f08_types")
    G.out.append("USE, intrinsic :: iso_c_binding, ONLY: c_int")
    G.out.append("USE :: mpi_c_interface_types, ONLY: c_Count, c_Status")
    G.out.append("IMPLICIT NONE")
    G.out.append("")
    G.out.append("private :: c_int, c_Count, c_Status")
    dump_handle_types()
    if "no-mpiio" not in G.opts:
        dump_file_interface()
    dump_status_type()
    dump_status_interface()
    dump_handle_interface()
    dump_sizeof_interface()
    G.out.append("")
    G.out.append("contains")
    G.out.append("")
    dump_sizeof_routines()
    dump_status_routines()
    dump_handle_routines()
    G.out.append("")
    dump_F_module_close("mpi_f08_types")

# -----------------------------
def dump_cdesc_c(f, lines):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)
        print("#include \"cdesc.h\"", file=Out)
        indent = 0
        for l in lines:
            if RE.match(r'INDENT', l):
                indent += 1
            elif RE.match(r'DEDENT', l):
                indent -= 1
            else:
                if indent > 0:
                    print("    " * indent, end='', file=Out)
                print(l, file=Out)

def dump_cdesc_proto_h(f, lines):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)
        for l in lines:
            tlist = split_line_with_break(l, ';', 80)
            for t in tlist:
                print(t, file=Out)

def dump_f90_file(f, lines):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_f90:
            print(l, file=Out)
        indent = 0
        for l in lines:
            if RE.match(r'INDENT', l):
                indent += 1
            elif RE.match(r'DEDENT', l):
                indent -= 1
            else:
                if indent > 0:
                    print("    " * indent, end='', file=Out)
                print(l, file=Out)

# -------------------------------
def need_cdesc(func):
    if '_need_cdesc' in func:
        return True
    else:
        return False

def process_func_parameters(func):
    for p in func['parameters']:
        if p['kind'] == 'BUFFER':
            func['_need_cdesc'] = True
            return

def check_func_directives(func):
    if 'dir' in func and func['dir'] == "mpit":
        func['_skip_fortran'] = 1
    elif RE.match(r'mpix_(grequest_|type_iov)', func['name'], re.IGNORECASE):
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_attr_', func['name'], re.IGNORECASE):
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_\w+_(f|f08|c)2(f|f08|c)$', func['name'], re.IGNORECASE):
        # implemented in mpi_f08_types.f90
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_.*_function$', func['name'], re.IGNORECASE):
        # defined in mpi_f08_callbacks.f90
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_type_(lb|ub|extent|hindexed|hvector|struct)$', func['name'], re.IGNORECASE):
        # removed in MPI-1 and not defined in mpi_f08
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_(address|errhandler_(create|get|set))$', func['name'], re.IGNORECASE):
        # removed in MPI-1 and not defined in mpi_f08
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_keyval_(create|free)$', func['name'], re.IGNORECASE):
        # deprecated and not defined in mpi_f08
        func['_skip_fortran'] = 1

# -------------------------------
def need_ptr_check(p):
    # Array parameter that may have special constant values
    if RE.match(r'(argv|array_of_argv|array_of_errcodes|\w*weights)', p['name']):
        return True
    else:
        return False

def get_F_procedure_type(p, is_large):
    if re.match(r'MPI_(Datarep_conversion|User)_function', p['func_type']) and is_large:
        return p['func_type'] + "_c"
    else:
        return p['func_type']
    return s

def get_F_decl(p, mapping):
    if p['kind'] == 'STRING':
        if p['length']:
            s = "CHARACTER(len=%s)" % p['length']
        else:
            s = "CHARACTER(len=*)"
    elif RE.match(r'STRING_(2D)?ARRAY', p['kind']):
        s = "CHARACTER(len=*)"
    elif RE.match(r'(PROCEDURE)', mapping[p['kind']]):
        s = "PROCEDURE(%s)" % get_F_procedure_type(p, re.match(r'BIG', mapping['_name']))
    else:
        s = mapping[p['kind']]

    # intent
    if p['kind'] == 'BUFFER' and p['param_direction'] == 'out':
        # assumed-type may not have the INTENT(OUT) attribute
        pass
    elif RE.match(r'procedure', s, re.IGNORECASE):
        pass
    elif not RE.search(r'suppress=f08_intent', p['t']):
        s += ", INTENT(%s)" % p['param_direction']

    # asynchronous
    if p['asynchronous']:
        s += ", ASYNCHRONOUS"

    # length
    length = get_F_decl_length(p)
    if p['name'] == "array_of_argv":
        # MPI_Comm_spawn_multiple
        length = "count, *"
    if need_ptr_check(p) or p['kind'] == "STATUS" or p['kind'] == "STRING_ARRAY":
        s += ', TARGET'
    s += ' :: ' + p['name']
    if length:
        s += '(%s)' % length

    return s

def get_F_decl_length(p):
    if p['kind'] == "STRING_ARRAY" or p['kind'] == "STRING_2DARRAY":
        return '*'
    elif p['length'] is None or p['kind'] == 'STRING':
        return None
    elif isinstance(p['length'], list):
        return ', '.join(p['length'])
    elif p['length'] == '': # array with assumed length
        return '*'
    else:
        return p['length']

def get_F_c_interface_decl(func, p, f_mapping, c_mapping):
    t_c = c_mapping[p['kind']]
    t_f = f_mapping[p['kind']]
    c_ptr = "TYPE(c_ptr), VALUE, INTENT(in) :: %s" % p['name']
    intent = "INTENT(%s)" % p['param_direction']

    def get_array():
        if t_c == 'int' and not RE.match(r'(array_of_errcodes|\w*weights)', p['name']):
            s = "INTEGER(c_int), %s :: %s" % (intent, p['name'])
        elif RE.match(r'MPI_(Fint|Aint|Count|Offset)', t_c):
            s = "%s, %s :: %s" % (t_f, intent, p['name'])
        else:
            return "TYPE(c_ptr), VALUE, INTENT(in) :: %s" % p['name']
        length = get_F_decl_length(p)
        if length:
            s += '(%s)' % length
        return s

    # ----
    if RE.match(r'TYPE\(MPIX?_(\w+)\)', t_f, re.IGNORECASE):
        if RE.m.group(1) == 'Status':
            return c_ptr
        elif p['param_direction'] == 'in' and p['length'] is None and func['name'] != "MPI_Cancel":
            # note: MPI_Cancel passed in not as value due to MPI specification oversight
            return "INTEGER(c_%s), VALUE, INTENT(in) :: %s" % (RE.m.group(1), p['name'])
        elif p['length'] is not None:
            return "INTEGER(c_%s), %s :: %s(%s)" % (RE.m.group(1), intent, p['name'], p['length'])
        else:
            return "INTEGER(c_%s), %s :: %s" % (RE.m.group(1), intent, p['name'])
    elif p['kind'] == 'STRING':
        return "character(kind=c_char), %s :: %s(*)" % (intent, p['name'])
    elif RE.match(r'(BUFFER)', p['kind']):
        if p['param_direction'] == 'in':
            return "%s, INTENT(in) :: %s" % (t_f, p['name'])
        else:
            # assumed-type may not have the INTENT(OUT) attribute
            return "%s :: %s" % (t_f, p['name'])
    elif RE.match(r'(EXTRA_STATE)', p['kind']):
        return "%s, %s :: %s" % (t_f, intent, p['name'])
    elif RE.match(r'TYPE\((c_ptr)\)', t_f, re.IGNORECASE):
        return "%s, %s :: %s" % (t_f, intent, p['name'])
    elif p['length'] is not None or RE.match(r'STRING_(2D)?ARRAY', p['kind']):
        return get_array()
    elif RE.match(r'(out|inout)', p['param_direction'], re.IGNORECASE):
        if t_c == 'int':
            return "INTEGER(c_int), %s :: %s" % (intent, p['name'])
        elif p['kind'] == 'ATTRIBUTE_VAL':
            return "%s, %s :: %s" % (t_f, intent, p['name'])
        elif RE.match(r'MPI_(Fint|Aint|Count|Offset)', t_c):
            return "%s, %s :: %s" % (t_f, intent, p['name'])
    else: # intent(in)
        if t_f == 'PROCEDURE':
            return "TYPE(c_funptr), VALUE :: %s" % p['name']
        elif t_c == 'int':
            return "INTEGER(c_int), VALUE, INTENT(in) :: %s" % p['name']
        elif p['kind'] == 'ATTRIBUTE_VAL':
            return "%s, VALUE, INTENT(in) :: %s" % (t_f, p['name'])
        elif RE.match(r'MPI_(Fint|Aint|Count|Offset)', t_c):
            return "%s, VALUE, INTENT(in) :: %s" % (t_f, p['name'])

    print("get_F_c_interface_decl: unhandled type %s - %s" % (func['name'], p['name']))
    return None

def get_F_c_decl(func, p, f_mapping, c_mapping):
    t_c = c_mapping[p['kind']]
    t_f = f_mapping[p['kind']]

    def get_string():
        t = "character(kind=c_char)"
        if p['param_direction'] == "in":
            return "%s :: %s_c(len_trim(%s) + 1)" % (t, p['name'], p['name'])
        elif func['name'] == "MPI_Info_get_string":
            return "%s :: %s_c(buflen + 1)" % (t, p['name'])
        elif p['length'] is None:
            return "%s :: %s_c(len(%s) + 1)" % (t, p['name'], p['name'])
        else:
            return "%s :: %s_c(%s + 1)" % (t, p['name'], p['length'])

    def get_array_decl():
        # Arrays: we'll use assumptions (since only with limited num of functions)
        length = get_F_decl_length(p)
        if RE.match(r'mpix?_(Test|Wait)(all|any)', func['name'], re.IGNORECASE):
            length = 'count'
        elif RE.match(r'mpix?_(Test|Wait)(some)', func['name'], re.IGNORECASE):
            length = 'incount'
        elif RE.match(r'mpi_cart_(rank|sub)', func['name'], re.IGNORECASE):
            length = 'cart_dim'
        elif RE.match(r'mpi_graph_(create|map)$', func['name'], re.IGNORECASE) and length != 'n':
            if p['name'] == 'indx':
                length = 'nnodes'
            else:
                length = 'indx(nnodes)'
        elif RE.match(r'mpi_dist_graph_create$', func['name'], re.IGNORECASE) and length != 'n':
            length = 'sum(degrees)'
        elif 'dir' in func and func['dir'] == 'coll':
            length = 'comm_size'
        # store the length
        p['_array_length'] = length

        if need_ptr_check(p):
            p['_array_convert'] = "c_ptr_check"
            return "TYPE(c_ptr) :: %s_cptr" % p['name']
        elif length == 'comm_size':
            if p['kind'] == "DATATYPE":
                p['_array_convert'] = "allocate:MPI_VAL"
                return "INTEGER(c_Datatype), allocatable, dimension(:) :: %s_c" % p['name']
            elif RE.match(r'MPI_(Count|Aint)', t_c):
                # t_f is something like INTEGER(KIND=MPI_COUNT_KIND)
                p['_array_convert'] = "allocate:%s" % RE.m.group(1)
                return "%s, allocatable, dimension(:) :: %s_c" % (t_f, p['name'])
            else:
                # assume t_c == 'c_int'
                p['_array_convert'] = "allocate:c_int"
                return "INTEGER(c_int), allocatable, dimension(:) :: %s_c" % p['name']
        elif length == 'cart_dim':
            if p['kind'] == 'LOGICAL':
                p['_array_convert'] = "allocate:logical"
                return "INTEGER(c_int), allocatable, dimension(:) :: %s_c" % p['name']
            else:
                p['_array_convert'] = "allocate:c_int"
                return "INTEGER(c_int), allocatable, dimension(:) :: %s_c" % p['name']
        elif p['kind'] == 'STATUS':
            p['_array_convert'] = "STATUS"
            return "TYPE(c_Status), TARGET :: %s_c(%s)" % (p['name'], length)
        elif RE.match(r'(REQUEST|DATATYPE|INFO|STREAM)', p['kind']):
            t = RE.m.group(1)
            c_type = "c_" + t[0].upper() + t[1:].lower()
            p['_array_convert'] = "MPI_VAL"
            return "INTEGER(%s) :: %s_c(%s)" % (c_type, p['name'], length)
        elif p['kind'] == "INDEX" and re.match(r'MPI_(Test|Wait)some', func['name'], re.IGNORECASE):
            p['_array_convert'] = "INDEX"
            return "INTEGER(c_int) :: %s_c(%s)" % (p['name'], length)
        elif p['kind'] == "LOGICAL":
            p['_array_convert'] = "LOGICAL"
            return "INTEGER(c_int) :: %s_c(%s)" % (p['name'], length)
        elif t_c == 'int' and t_f == 'INTEGER':
            p['_array_convert'] = "c_int"
            return "INTEGER(c_int) :: %s_c(%s)" % (p['name'], length)
        elif RE.match(r'MPI_(Fint|Aint|Count|Offset)', t_c):
            # no conversion needed
            return None
        elif RE.match(r'STRING_ARRAY', p['kind']):
            # direct conversion, handle it in dump_f08_wrappers_f
            return None
        else:
            print("Unhandled array parameter: %s - %s" % (p['name'], p['kind']))
            return None

    # ----
    if p['kind'] == 'STRING':
        return get_string()
    elif t_f == 'PROCEDURE':
        return "TYPE(c_funptr) :: %s_c" % p['name']
    elif p['length'] is not None or RE.match(r'STRING_(2D)?ARRAY', p['kind']):
        return get_array_decl()
    elif t_c == 'int':
        return "INTEGER(c_int) :: %s_c" % p['name']
    elif RE.match(r'TYPE\(MPIX?_(\w+)\)', t_f, re.IGNORECASE):
        if RE.m.group(1) == 'Status':
            return "TYPE(c_%s), TARGET :: %s_c" % (RE.m.group(1), p['name'])
        else:
            return "INTEGER(c_%s) :: %s_c" % (RE.m.group(1), p['name'])
    elif RE.match(r'(BUFFER|EXTRA_STATE|ATTRIBUTE_VAL)', p['kind']):
        return None
    elif RE.match(r'MPI_(Fint|Aint|Count|Offset)', t_c):
        return None
    elif RE.match(r'TYPE\((c_ptr)\)', t_f, re.IGNORECASE):
        return None
    else:
        print("get_F_c_decl: unhandled type %s: %s - %s" % (p['name'], t_f, t_c))
        return None

#----------------------------------------
# Depend on integer size, POLY parameters don't always end up with different interfaces
def get_real_POLY_kinds():
    G.real_poly_kinds = {}

    def get_int_type(fortran_type):
        if fortran_type == "INTEGER":
            return "fint"
        elif "MPI_ADDRESS_KIND" in fortran_type:
            return "aint"
        elif "MPI_COUNT_KIND" in fortran_type:
            return "count"
        else:
            raise Exception("Unrecognized POLY int type: %s" % fortran_type)

    small_map = G.MAPS['SMALL_F08_KIND_MAP']
    large_map = G.MAPS['BIG_F08_KIND_MAP']
    for kind in small_map:
        if kind == 'POLYFUNCTION':
            # Currently there are only two: MPI_User_function, MPI_Datarep_conversion_function,
            # both contains parameter POLYXFER_NUM_ELEM. However, Fortran is not able to
            # differentiate generic interface based on different function signature. Disable
            # for now.
            pass
        elif kind.startswith('POLY'):
            a = get_int_type(small_map[kind]) + "-size"
            b = get_int_type(large_map[kind]) + "-size"
            if G.opts[a] != G.opts[b]:
                G.real_poly_kinds[kind] = 1

def function_has_real_POLY_parameters(func):
    for p in func['parameters']:
        if p['kind'] in G.real_poly_kinds:
            return True
    return False
