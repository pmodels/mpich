##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.binding_common import *
from local_python import RE

import re

def dump_f77_c_func(func, is_cptr=False):
    func_name = get_function_name(func)
    f_mapping = get_kind_map('F90')
    c_mapping = get_kind_map('C')

    c_param_list = []
    c_param_list_end = []  # for string args, ref: FORT_END_LEN
    code_list_common = []
    end_list_common = []

    # code for HAVE_FINT_IS_INT
    c_arg_list_A = []
    code_list_A = []
    end_list_A = []

    # code for #!defined HAVE_FINT_IS_INT
    c_arg_list_B = []
    code_list_B = []
    end_list_B = []

    need_ATTR_AINT = False
    is_custom_fn = False  # custom function body
    need_skip_ierr = False

    if re.match(r'MPI_((\w+)_(get|set)_attr|Attr_(put|get))', func['name'], re.IGNORECASE):
        need_ATTR_AINT = True
    elif re.match(r'MPI.*_(DUP|DELETE|COPY)_FN|MPI_CONVERSION_FN_NULL', func['name'], re.IGNORECASE):
        is_custom_fn = True
    elif re.match(r'MPI_F_sync_reg', func['name'], re.IGNORECASE):
        is_custom_fn = True
    
    if len(func['parameters']) > 0:
        last_p = func['parameters'][-1]
        if last_p['kind'] == 'ERROR_CODE' and re.search(r'c_parameter', last_p['suppress']):
            need_skip_ierr = True
    if re.match(r'MPI_Pcontrol', func['name'], re.IGNORECASE):
        need_skip_ierr = True

    # ---- internal functions --------------------
    # input as MPI_Fint
    def dump_p(p):
        c_type = c_mapping[p['kind']]
        if "length" in p and p['length']:
            if re.match(r'mpi_i?neighbor_', func['name'], re.IGNORECASE):
                if p['kind'] == "DISPLACEMENT":
                    # neighbor_alltoallw - sdispls and rdispls
                    dump_direct_pointer(p['name'], "MPI_Aint")
                elif re.search(r'_allgatherv', func['name'], re.IGNORECASE):
                    dump_array_in(p['name'], c_type, "indegree")
                elif re.match(r'(sendcounts|sdispls|sendtypes)', p['name'], re.IGNORECASE):
                    dump_array_in(p['name'], c_type, "indegree")
                else:
                    dump_array_in(p['name'], c_type, "outdegree")
            elif re.match(r'mpi_.*(gatherv|scatterv|alltoall[vw]|reduce_scatter)', func['name'], re.IGNORECASE):
                dump_array_in(p['name'], c_type, "comm_size")
            else:
                raise Exception("Unhandled: %s - %s, length=%s" % (func['name'], p['name'], p['length']))
        else:
            c_param_list.append("MPI_Fint *" + p['name'])
            c_arg_list_A.append("(%s) (*%s)" % (c_type, p['name']))
            c_arg_list_B.append("(%s) (*%s)" % (c_type, p['name']))

    def dump_scalar_out(v, f_type, c_type):
        c_param_list.append("%s *%s" % (f_type, v))
        c_arg_list_A.append("&%s_i" % v)
        c_arg_list_B.append("&%s_i" % v)
        code_list_common.append("%s %s_i;" % (c_type, v))
        end_list_common.append("*%s = (%s) %s_i;" % (v, f_type, v))

    # void *
    def dump_buf(buf, check_in_place):
        # void *
        c_param_list.append("void *%s" % buf)

        c_arg_list_A.append(buf)
        c_arg_list_B.append(buf)
        code_list_common.append("if (%s == MPIR_F_MPI_BOTTOM) {" % buf)
        code_list_common.append("    %s = MPI_BOTTOM;" % buf)
        if check_in_place:
            code_list_common.append("} else if (%s == MPIR_F_MPI_IN_PLACE) {" % buf)
            code_list_common.append("    %s = MPI_IN_PLACE;" % buf)
        code_list_common.append("}")
        code_list_common.append("")

    # MPI_Buffer_attach and family
    def dump_buf_attach(buf):
        # void *
        c_param_list.append("void *%s" % buf)
        c_arg_list_A.append(buf)
        c_arg_list_B.append(buf)
        code_list_common.append("if (%s == MPIR_F_MPI_BUFFER_AUTOMATIC) {" % buf)
        code_list_common.append("    %s = MPI_BUFFER_AUTOMATIC;" % buf)
        code_list_common.append("}")
        code_list_common.append("")

    # MPI_Status
    def dump_status(v, is_in, is_out):
        c_param_list.append("MPI_Fint *%s" % v)

        def dump_FINT_is_INT_pre():
            c_arg_list_A.append("(MPI_Status *) %s" % v)
            if is_out:
                code_list_A.append("if (%s == MPI_F_STATUS_IGNORE) {" % v)
                code_list_A.append("    %s = (MPI_Fint *) MPI_STATUS_IGNORE;" % v)
                code_list_A.append("}")

        def dump_FINT_not_INT_pre():
            c_arg_list_B.append("status_arg")
            code_list_B.append("MPI_Status * status_arg;")
            code_list_B.append("int status_i[MPI_F_STATUS_SIZE];")
            if is_out:
                code_list_B.append("if (%s == MPI_F_STATUS_IGNORE) {" % v)
                code_list_B.append("    status_arg = MPI_STATUS_IGNORE;")
                code_list_B.append("} else {")
                # NOTE: always copy in because some fields (e.g. MPI_ERROR) may need be untouched
                code_list_B.append("INDENT")
                copy_status_in()
                code_list_B.append("DEDENT")
                code_list_B.append("}")
            if is_in:
                copy_status_in()
        def dump_FINT_not_INT_post():
            if is_out:
                end_list_B.append("if (%s != MPI_F_STATUS_IGNORE) {" % v)
                code_list_B.append("INDENT")
                copy_status_out()
                code_list_B.append("DEDENT")
                end_list_B.append("}")
        def copy_status_in():
            code_list_B.append("status_arg = (MPI_Status *) status_i;")
            code_list_B.append("for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {")
            code_list_B.append("    status_i[i] = (int) %s[i];" % v)
            code_list_B.append("}")
        def copy_status_out():
            end_list_B.append("for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {")
            end_list_B.append("    %s[i] = (MPI_Fint) status_i[i];" % v)
            end_list_B.append("}")

        dump_FINT_is_INT_pre()
        dump_FINT_not_INT_pre()
        dump_FINT_not_INT_post()

    # e.g. MPI_Status array_of_statuses[]
    def dump_statuses(v, incount, outcount, is_in, is_out):
        c_param_list.append("MPI_Fint *%s" % v)

        def dump_FINT_is_INT_pre():
            c_arg_list_A.append("(MPI_Status *) %s" % v)
            if is_out:
                code_list_A.append("if (%s == MPI_F_STATUSES_IGNORE) {" % v)
                code_list_A.append("    %s = (MPI_Fint *) MPI_STATUSES_IGNORE;" % v)
                code_list_A.append("}")

        def dump_FINT_not_INT_pre():
            c_arg_list_B.append("statuses_i")
            code_list_B.append("MPI_Status *statuses_i;")
            if is_out:
                code_list_B.append("if (%s == MPI_F_STATUSES_IGNORE) {" % v)
                code_list_B.append("    statuses_i = MPI_STATUS_IGNORE;")
                code_list_B.append("} else {")
            else:
                code_list_B.append("{")
            code_list_B.append("INDENT")
            copy_statuses_in()
            code_list_B.append("DEDENT")
            code_list_B.append("}")
        def dump_FINT_not_INT_post():
            if is_out:
                end_list_B.append("if (%s != MPI_F_STATUSES_IGNORE) {" % v)
                end_list_B.append("INDENT")
                copy_statuses_out()
                end_list_B.append("DEDENT")
                end_list_B.append("}")
            else:
                end_list_B.append("free(statuses_i);")

        def copy_statuses_in():
            code_list_B.append("statuses_i = malloc(%s * sizeof(MPI_Status));" % incount)
            code_list_B.append("int *p = (int *) statuses_i;")
            code_list_B.append("for (int i = 0; i < %s * MPI_F_STATUS_SIZE; i++) {" % incount)
            code_list_B.append("    p[i] = (int) %s[i];" % v)
            code_list_B.append("}")
        def copy_statuses_out():
            end_list_B.append("int *p = (int *) statuses_i;")
            end_list_B.append("for (int i = 0; i < %s * MPI_F_STATUS_SIZE; i++) {" % outcount)
            end_list_B.append("    %s[i] = (MPI_Fint) p[i];" % v)
            end_list_B.append("}")
            end_list_B.append("free(statuses_i);")


        dump_FINT_is_INT_pre()
        dump_FINT_not_INT_pre()
        dump_FINT_not_INT_post()

    # scalar int output, e.g. MPI_Request *req_out
    def dump_int_out(v, c_type, is_inout):
        c_param_list.append("MPI_Fint *%s" % v)

        def dump_FINT_is_INT_pre():
            c_arg_list_A.append("(%s *) %s" % (c_type, v))

        def dump_FINT_not_INT_pre():
            c_arg_list_B.append("&%s_i" % v)
            code_list_B.append("%s %s_i;" % (c_type, v))
            if is_inout:
                code_list_B.append("%s_i = (%s) *%s;" % (v, c_type, v))
        def dump_FINT_not_INT_post():
            end_list_B.append("*%s = (MPI_Fint) %s_i;" % (v, v))

        dump_FINT_is_INT_pre()
        dump_FINT_not_INT_pre()
        dump_FINT_not_INT_post()

    # scalar integer input, but we assume Fortran use the same c_type
    def dump_c_type_in(v, c_type):
        c_param_list.append("%s *%s" % (c_type, v))
        c_arg_list_A.append('*' + v)
        c_arg_list_B.append('*' + v)

    # e.g. MPI_Address
    def dump_aint_out_check(v):
        c_param_list.append("MPI_Fint *%s" % v)
        c_arg_list_A.append("&%s_i" % v)
        c_arg_list_B.append("&%s_i" % v)
        code_list_common.append("MPI_Aint %s_i;" % v)
        end_list_common.append("*%s = (MPI_Fint) %s_i;" % (v, v))
        end_list_common.append("if ((MPI_Aint) (*%s) != %s_i) {" % (v, v))
        end_list_common.append("    /* Unfortunately, there isn't an easy way to invoke error handler */")
        end_list_common.append("    *ierr = MPI_ERR_OTHER;")
        end_list_common.append("}")

    # arrays, e.g. MPI_Request array_of_reqs[]
    # NOTE: we also handle MPI_UNWEIGHTED, MPI_WEIGHTS_EMPTY

    def dump_array_in(v, c_type, count):
        dump_array(v, c_type, count, count, True, False)

    def dump_array_out(v, c_type, count):
        dump_array(v, c_type, count, count, False, True)

    def dump_array_special_in(v, c_type, count):
        dump_array(v, c_type, count, count, True, False, True)

    def dump_array_special_out(v, c_type, count):
        dump_array(v, c_type, count, count, False, True, True)

    def dump_array(v, c_type, incount, outcount, is_in=True, is_out=False, has_special=False):
        c_param_list.append("MPI_Fint *%s" % v)

        def dump_FINT_is_INT_pre():
            c_arg_list_A.append("(int *) %s" % v)
            dump_check_special(True)

        def dump_FINT_not_INT_pre():
            c_arg_list_B.append("%s_i" % v)
            code_list_B.append("%s *%s_i;" % (c_type, v))
            dump_check_special(False)

            dump_check_special_begin(code_list_B)
            code_list_B.append("%s_i = malloc(sizeof(%s) * %s);" % (v, c_type, incount))
            if is_in:
                code_list_B.append("for (int i = 0; i < %s; i++) {" % incount)
                code_list_B.append("    %s_i[i] = (%s) %s[i];" % (v, c_type, v))
                code_list_B.append("}")
            dump_check_special_end(code_list_B)
        def dump_FINT_not_INT_post():
            dump_check_special_begin(end_list_B)
            if is_out:
                end_list_B.append("for (int i = 0; i < %s; i++) {" % outcount)
                if v == 'periods':
                    # hack for logical
                    end_list_B.append("    %s[i] = MPII_TO_FLOG(%s_i[i]);" % (v, v))
                else:
                    end_list_B.append("    %s[i] = (MPI_Fint) %s_i[i];" % (v, v))
                end_list_B.append("}")
            end_list_B.append("free(%s_i);" % v)
            dump_check_special_end(end_list_B)

        def dump_check_special(FINT_is_INT):
            def check_weights():
                if FINT_is_INT:
                    code_list_A.append("if (%s == MPIR_F_MPI_UNWEIGHTED) {" % v)
                    code_list_A.append("    %s = (MPI_Fint *) MPI_UNWEIGHTED;" % v)
                    code_list_A.append("} else if (%s == MPIR_F_MPI_WEIGHTS_EMPTY) {" % v)
                    code_list_A.append("    %s = (MPI_Fint *) MPI_WEIGHTS_EMPTY;" % v)
                    code_list_A.append("}")
                else:
                    code_list_B.append("int %s_is_special = 0;" % v)
                    code_list_B.append("if (%s == MPIR_F_MPI_UNWEIGHTED) {" % v)
                    code_list_B.append("    %s_i = MPI_UNWEIGHTED;" % v)
                    code_list_B.append("    %s_is_special = 1;" % v)
                    code_list_B.append("} else if (%s == MPIR_F_MPI_WEIGHTS_EMPTY) {" % v)
                    code_list_B.append("    %s_i = MPI_WEIGHTS_EMPTY;" % v)
                    code_list_B.append("    %s_is_special = 1;" % v)
                    code_list_B.append("}")
            def check_errcodes():
                if FINT_is_INT:
                    code_list_A.append("if (%s == MPI_F_ERRCODES_IGNORE) {" % v)
                    code_list_A.append("    %s = (MPI_Fint *) MPI_ERRCODES_IGNORE;" % v)
                    code_list_A.append("}")
                else:
                    code_list_B.append("int %s_is_special = 0;" % v)
                    code_list_B.append("if (%s == MPI_F_ERRCODES_IGNORE) {" % v)
                    code_list_B.append("    %s_i = MPI_ERRCODES_IGNORE;" % v)
                    code_list_B.append("    %s_is_special = 1;" % v)
                    code_list_B.append("}")

            if has_special:
                if is_in and re.match(r'(source|dest)?weights$', v):
                    check_weights()
                elif is_out and re.match(r'.*_errcodes$', v):
                    check_errcodes()
                else:
                    raise Exception("Unhandled special parameter: %s - %s" % (func['name'], v))

        def dump_check_special_begin(code_list):
            if has_special:
                code_list.append("if (!%s_is_special) {" % v)
                code_list.append("INDENT")
        def dump_check_special_end(code_list):
            if has_special:
                code_list.append("DEDENT")
                code_list.append("}")

        dump_FINT_is_INT_pre()
        dump_FINT_not_INT_pre()
        dump_FINT_not_INT_post()

    def dump_array_int_to_aint(v, count):
        c_param_list.append("MPI_Fint *%s" % v)
        c_arg_list_A.append("%s_i" % v)
        c_arg_list_B.append("%s_i" % v)
        code_list_common.append("MPI_Aint *%s_i;" % v)
        code_list_common.append("#ifdef HAVE_AINT_DIFFERENT_THAN_FINT")
        code_list_common.append("%s_i = malloc(%s * sizeof(MPI_Aint));" % (v, count))
        code_list_common.append("for (int i = 0; i < %s; i++) {" % count)
        code_list_common.append("    %s_i[i] = (MPI_Aint) %s[i];" % (v, v))
        code_list_common.append("}")
        code_list_common.append("#else")
        code_list_common.append("%s_i = %s;" % (v, v))
        code_list_common.append("#endif")
        end_list_common.append("#ifdef HAVE_AINT_DIFFERENT_THAN_FINT")
        end_list_common.append("free(%s_i);" % v)
        end_list_common.append("#endif")

    def dump_string_in(v):
        c_param_list.append("char *%s FORT_MIXED_LEN(%s_len)" % (v, v))
        c_param_list_end.append("FORT_END_LEN(%s_len)" % v)
        c_arg_list_A.append("%s_i" % v)
        c_arg_list_B.append("%s_i" % v)
        code_list_common.append("char *%s_i = MPIR_fort_dup_str(%s, %s_len);" % (v, v, v))
        end_list_common.append("free(%s_i);" % (v))

    def dump_string_out(v, flag=None):
        c_param_list.append("char *%s FORT_MIXED_LEN(%s_len)" % (v, v))
        c_param_list_end.append("FORT_END_LEN(%s_len)" % v)
        c_arg_list_A.append("%s_i" % v)
        c_arg_list_B.append("%s_i" % v)
        code_list_common.append("char *%s_i = malloc(%s_len + 1);" % (v, v))

        flag_cond = "*ierr == MPI_SUCCESS"
        if flag:
            flag_cond += " && " + flag
        end_list_common.append("if (%s) {" % flag_cond)
        end_list_common.append("INDENT")
        end_list_common.append("MPIR_fort_copy_str_from_c(%s, %s_len, %s_i);" % (v, v, v))
        end_list_common.append("DEDENT")
        end_list_common.append("}")
        end_list_common.append("free(%s_i);" % v)

    def dump_string_array(v, count="0"):
        c_param_list.append("char *%s FORT_MIXED_LEN(%s_len)" % (v, v))
        c_param_list_end.append("FORT_END_LEN(%s_len)" % v)
        c_arg_list_A.append("%s_i" % v)
        c_arg_list_B.append("%s_i" % v)
        code_list_common.append("char **%s_i;" % v)
        code_list_common.append("if (%s == MPI_F_ARGV_NULL) {" % v)
        code_list_common.append("    %s_i = MPI_ARGV_NULL;" % v)
        code_list_common.append("} else {")
        code_list_common.append("    %s_i = MPIR_fort_dup_str_array(%s, %s_len, %s_len, %s);" % (v, v, v, v, count))
        code_list_common.append("}")
        end_list_common.append("if (%s != MPI_F_ARGV_NULL) {" % v)
        end_list_common.append("    MPIR_fort_free_str_array(%s_i);" % v)
        end_list_common.append("}")

    def dump_string_2darray(v, count):
        c_param_list.append("char *%s FORT_MIXED_LEN(%s_len)" % (v, v))
        c_param_list_end.append("FORT_END_LEN(%s_len)" % v)
        c_arg_list_A.append("%s_i" % v)
        c_arg_list_B.append("%s_i" % v)
        code_list_common.append("char ***%s_i;" % v)
        code_list_common.append("if (%s == MPI_F_ARGVS_NULL) {" % v)
        code_list_common.append("    %s_i = MPI_ARGVS_NULL;" % v)
        code_list_common.append("} else {")
        code_list_common.append("    %s_i = MPIR_fort_dup_str_2d_array(%s, %s_len, %s);" % (v, v, v, count))
        code_list_common.append("}")
        end_list_common.append("if (%s != MPI_F_ARGVS_NULL) {" % v)
        end_list_common.append("    MPIR_fort_free_str_2d_array(%s_i, %s);" % (v, count))
        end_list_common.append("}")

    def dump_file_in(v):
        c_param_list.append("MPI_Fint *%s" % v)
        c_arg_list_A.append("MPI_File_f2c(*%s)" % v)
        c_arg_list_B.append("MPI_File_f2c(*%s)" % v)

    def dump_file_out(v, is_inout):
        c_param_list.append("MPI_Fint *%s" % v)
        c_arg_list_A.append("&%s_i" % v)
        c_arg_list_B.append("&%s_i" % v)
        if is_inout:
            code_list_common.append("MPI_File %s_i = MPI_File_f2c(*%s);" % (v, v))
        else:
            code_list_common.append("MPI_File %s_i;" % v)
        end_list_common.append("*%s = MPI_File_c2f(%s_i);" % (v, v))

    def dump_logical_in(v):
        c_param_list.append("MPI_Fint *%s" % v)
        c_arg_list_A.append("*%s" % v)
        c_arg_list_B.append("*%s" % v)

    def dump_logical_out(v):
        c_param_list.append("MPI_Fint *%s" % v)
        c_arg_list_A.append("&%s_i" % v)
        c_arg_list_B.append("&%s_i" % v)
        code_list_common.append("int %s_i;" % v)
        end_list_common.append("if (*ierr == MPI_SUCCESS) {")
        end_list_common.append("    *%s = MPII_TO_FLOG(%s_i);" % (v, v))
        end_list_common.append("}")

    def dump_index_in(v):
        c_param_list.append("MPI_Fint *%s" % v)
        c_arg_list_A.append("(int) (*%s - 1)" % v)
        c_arg_list_B.append("(int) (*%s - 1)" % v)

    def dump_index_out(v):
        c_param_list.append("MPI_Fint *%s" % v)
        c_arg_list_A.append("&%s_i" % v)
        c_arg_list_B.append("&%s_i" % v)
        code_list_common.append("int %s_i;" % v)
        end_list_common.append("if (*ierr == MPI_SUCCESS) {")
        end_list_common.append("    if (%s_i == MPI_UNDEFINED) {" % v)
        end_list_common.append("        *%s = %s_i;" % (v, v))
        end_list_common.append("    } else {")
        end_list_common.append("        *%s = %s_i + 1;" % (v, v))
        end_list_common.append("    }")
        end_list_common.append("}")

    def dump_string_len_inout(v):
        # only used in MPI_Info_get_string
        c_param_list.append("MPI_Fint *%s" % v)
        c_arg_list_A.append("&%s_i" % v)
        c_arg_list_B.append("&%s_i" % v)
        code_list_common.append("int %s_i = (*%s > 0) ? (int) (*%s + 1) : 0;" % (v, v, v))
        end_list_common.append("*%s = (%s_i > 0) ? (MPI_Fint) (%s_i - 1) : 0;" % (v, v, v))

    def dump_index_array_out(v, incount, outcount):
        dump_array(v, 'int', incount, outcount, False, True)
        end_list_common.append("for (int i = 0; i < %s; i++) {" % outcount)
        end_list_common.append("    %s[i] += 1;" % v)
        end_list_common.append("}")

    def dump_function(v, func_type):
        if re.match(r'MPI_\w+_errhandler_function', func_type, re.IGNORECASE):
            # Fortran errhandler does not do variadic (...); use MPI_Fint for handle
            # F77_ErrFunction is defined in mpi_fortimpl.h
            c_param_list.append("F77_ErrFunction %s" % v)
        else:
            c_param_list.append("%s %s" % (func_type, v))
        c_arg_list_A.append(v)
        c_arg_list_B.append(v)
        if func_type == "MPI_Datarep_conversion_function":
            # FIXME: check name mangling
            code_list_common.append("if (%s == (MPI_Datarep_conversion_function *) mpi_conversion_fn_null_) {" % v)
            code_list_common.append("    %s = NULL;" % v)
            code_list_common.append("}")

    def dump_direct_pointer(v, c_type):
        c_param_list.append("%s *%s" % (c_type, v))
        c_arg_list_A.append(v)
        c_arg_list_B.append(v)

    def dump_attr_in(v, c_type):
        c_param_list.append("%s *%s" % (c_type, v))
        c_arg_list_A.append("(void *) (intptr_t) (*%s)" % v)
        c_arg_list_B.append("(void *) (intptr_t) (*%s)" % v)

    def dump_attr_out(v, c_type, flag):
        c_param_list.append("%s *%s" % (c_type, v))
        c_arg_list_A.append("&%s_i" % v)
        c_arg_list_B.append("&%s_i" % v)
        code_list_common.append("void *%s_i;" % v)
        end_list_common.append("if (*ierr || !%s) {" % flag)
        end_list_common.append("    *%s = 0;" % v)
        end_list_common.append("} else {")
        end_list_common.append("    *%s = (MPI_Aint) %s_i;" % (v, v))
        end_list_common.append("}")

    def dump_sum(codelist, sum_v, sum_n, array):
        codelist.append("int %s = 0;" % sum_v)
        codelist.append("for (int i = 0; i < *%s; i++) {" % sum_n)
        codelist.append("    %s += (int) %s[i];" % (sum_v, array))
        codelist.append("}")

    def dump_mpi_decl_begin(name, param_str, return_type):
        G.out.append("FORT_DLL_SPEC %s FORT_CALL %s(%s) {" % (return_type, name, param_str))
        G.out.append("INDENT")
        if re.match(r'MPI_File_|MPI_Register_datarep', func['name'], re.IGNORECASE):
            G.out.append("#ifndef HAVE_ROMIO")
            G.out.append("*ierr = MPI_ERR_INTERN;")
            G.out.append("#else")

    def dump_mpi_decl_end():
        if re.match(r'MPI_File_|MPI_Register_datarep', func['name'], re.IGNORECASE):
            G.out.append("#endif")
        G.out.append("DEDENT")
        G.out.append("}")

    def dump_custom_functions(func):
        # assume the last parameter is ierror -- forum don't name it consistently
        err = func['parameters'][-1]['name']
        if re.match(r'MPI_CONVERSION_FN_NULL', func['name'], re.IGNORECASE):
            G.out.append("/* dummy function, not callable */")
            G.out.append("return 0;")
        elif re.match(r'MPI.*_DUP_FN', func['name'], re.IGNORECASE):
            G.out.append("* (MPI_Aint *) attribute_val_out = * (MPI_Aint *) attribute_val_in;")
            G.out.append("*flag = MPII_TO_FLOG(1);")
            G.out.append("*%s = MPI_SUCCESS;" % err)
        elif re.match(r'MPI.*_NULL_COPY_FN', func['name'], re.IGNORECASE):
            G.out.append("*flag = MPII_TO_FLOG(0);")
            G.out.append("*%s = MPI_SUCCESS;" % err)
        elif re.match(r'MPI.*_NULL_DELETE_FN', func['name'], re.IGNORECASE):
            G.out.append("*%s = MPI_SUCCESS;" % err)
        elif re.match(r'MPI_F_sync_reg', func['name'], re.IGNORECASE):
            G.out.append("*ierr = MPI_SUCCESS;")
        else:
            raise Exception("Unhandled dummy function - %s" % func['name'])

    # ----------------------------------
    def process_func_parameters():
        n = len(func['parameters'])
        i = 0
        while i < n:
            p = func['parameters'][i]
            if p['large_only']:
                i += 1
                continue

            (group_kind, group_count) = ("", 0)
            if i + 3 <= n and RE.search(r'BUFFER', p['kind']):
                group_kind, group_count = get_userbuffer_group(func['name'], func['parameters'], i)

            if group_count > 0:
                p2 = func['parameters'][i + 1]
                p3 = func['parameters'][i + 2]
                if group_count == 3:
                    dump_buf(p['name'], re.search('-inplace', group_kind))
                    dump_p(p2)
                    dump_p(p3)
                elif group_count == 4:
                    # e.g. Alltoallv
                    p4 = func['parameters'][i + 3]
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
                    dump_p(p3)
                    dump_p(p4)
                    dump_p(p5)

                i += group_count
                continue
            else:
                i += 1

            if f90_param_need_skip(p):
                pass;

            elif p['kind'] == "BUFFER":
                if re.match(r'MPI_Buffer_attach|MPI_\w+_buffer_attach', func['name'], re.IGNORECASE):
                    dump_buf_attach(p['name'])
                else:
                    dump_buf(p['name'], False)

            elif p['kind'] == "STRING":
                if p['param_direction'] == 'out':
                    if re.match(r'MPI_Info_get$', func['name'], re.IGNORECASE):
                        dump_string_out(p['name'], '*flag')
                    elif re.match(r'MPI_Info_get_string$', func['name'], re.IGNORECASE):
                        code_list_common.append("int buflen_nonzero = (*buflen > 0);")
                        dump_string_out(p['name'], '(*flag && buflen_nonzero)')
                    else:
                        dump_string_out(p['name'])
                else:
                    dump_string_in(p['name'])

            elif p['kind'] == "STRING_ARRAY":
                if p['name'] == 'array_of_commands':
                    dump_string_array(p['name'], "*count")
                else:
                    dump_string_array(p['name'])

            elif p['kind'] == "STRING_2DARRAY":
                if re.match(r'MPI_Comm_spawn_multiple$', func['name'], re.IGNORECASE):
                    dump_string_2darray(p['name'], "*count")
                else:
                    raise Exception("Not expected: %s - %s" % (func['name'], p['name']))

            elif p['kind'] == "STATUS":
                if p['param_direction'] == 'out':
                    if p['length'] is None:
                        dump_status(p['name'], False, True)
                    elif RE.match(r'mpix?_(wait|test|request_get_status_|continue)all', func['name'], re.IGNORECASE):
                        dump_statuses(p['name'], "(*count)", "(*count)", False, True)
                    elif RE.match(r'mpix?_(wait|test|request_get_status_)some', func['name'], re.IGNORECASE):
                        dump_statuses(p['name'], "(*incount)", "(*outcount)", False, True)
                    else:
                        raise Exception("Unhandled: %s - %s" % (func['name'], p['name']))
                else:
                    if p['length'] is None:
                        dump_status(p['name'], True, False)
                    else:
                        raise Exception("Unhandled: %s - %s" % (func['name'], p['name']))

            elif RE.match(r'(source|dest)?weights$', p['name']):
                if RE.match(r'MPI_Dist_graph_create$', func['name'], re.IGNORECASE):
                    # NOTE: total_degrees is pre-calculated
                    dump_array_special_in(p['name'], "int", "total_degrees")
                elif RE.match(r'MPI_Dist_graph_create_adjacent$', func['name'], re.IGNORECASE):
                    if p['name'] == 'sourceweights':
                        dump_array_special_in(p['name'], "int", "(*indegree)")
                    else:
                        dump_array_special_in(p['name'], "int", "(*outdegree)")
                elif RE.match(r'mpi_dist_graph_neighbors$', func['name'], re.IGNORECASE):
                    if p['name'] == 'sourceweights':
                        dump_array_out(p['name'], "int", "(*maxindegree)")
                    else:
                        dump_array_out(p['name'], "int", "(*maxoutdegree)")
                else:
                    raise Exception("Not handled: %s - %s" % (func['name'], p['name']))

            elif RE.match(r'array_of_errcodes', p['name']):
                if re.match(r'MPI_Comm_spawn$', func['name'], re.IGNORECASE):
                    dump_array_special_out(p['name'], "int", "(*maxprocs)")
                elif re.match(r'MPI_Comm_spawn_multiple$', func['name'], re.IGNORECASE):
                    # NOTE: total_procs is pre-calculated
                    dump_array_special_out(p['name'], "int", "total_procs")
                else:
                    raise Exception("Not handled: %s - %s" % (func['name'], p['name']))

            elif p['kind'] == "FILE":
                if p['param_direction'] == 'out':
                    dump_file_out(p['name'], False)
                elif p['param_direction'] == 'inout':
                    dump_file_out(p['name'], True)
                else:
                    dump_file_in(p['name'])
            elif p['kind'] == "LOGICAL":
                
                if re.match(r'MPI_Cart_sub$', func['name'], re.IGNORECASE):
                    # remain_dims
                    dump_array_in(p['name'], 'int', "maxdims")
                elif p['length']:
                    # assume input (MPI_Cart_create - periods)
                    if p['length'] == '*':
                        raise Exception("Unhandled: %s - %s" % (func['name'], p['name']))
                    count = "(*%s)" % p['length']
                    dump_array(p['name'], 'int', count, count, param_is_in(p), param_is_out(p))
                elif p['param_direction'] == 'out':
                    dump_logical_out(p['name'])
                else:
                    dump_logical_in(p['name'])
            elif p['kind'] == "INDEX":
                # tricky, not all need to be 1-based
                if p['length']:
                    if re.match(r'MPI_(Test|Wait|Request_get_status_)some', func['name'], re.IGNORECASE):
                        dump_index_array_out(p['name'], '(*incount)', '(*outcount)')
                    elif re.match(r'MPI_Graph_get', func['name'], re.IGNORECASE):
                        dump_array_out(p['name'], 'int', '(*maxindex)')
                    elif re.match(r'MPI_Graph_(create|map)', func['name'], re.IGNORECASE):
                        dump_array_in(p['name'], 'int', "(*%s)" % p['length'])
                    else:
                        raise Exception("Unhandled: %s - %s" % (func['name'], p['name']))
                elif p['param_direction'] == 'out':
                    dump_index_out(p['name'])
                elif p['name'] == "direction":  # MPI_Cart_shift
                    dump_p(p)
                else:
                    dump_index_in(p['name'])
            elif re.match(r'FUNCTION|FUNCTION_SMALL|POLYFUNCTION', p['kind']):
                dump_function(p['name'], p['func_type'])
            elif is_cptr and p['kind'] == 'C_BUFFER' and p['param_direction'] == 'out':
                dump_scalar_out(p['name'], "void *", "void *")
            elif re.match(r'EXTRA_STATE|C_BUFFER2|C_BUFFER', p['kind']):
                if p['param_direction'] == 'out':
                    dump_scalar_out(p['name'], "MPI_Aint", "void *")
                elif p['param_direction'] != "inout":
                    dump_direct_pointer(p['name'], "void")
                else:
                    raise Exception("Unhandled: %s - %s" % (func['name'], p['name']))
            elif re.match(r'ATTRIBUTE_VAL', p['kind']):
                if re.match(r'MPI_((Comm|Type|Win)_get_attr)', func['name'], re.IGNORECASE):
                    dump_attr_out(p['name'], "MPI_Aint", "flag_i")
                elif re.match(r'MPI_((Comm|Type|Win)_set_attr)', func['name'], re.IGNORECASE):
                    dump_attr_in(p['name'], "MPI_Aint")
                elif re.match(r'MPI_Attr_get', func['name'], re.IGNORECASE):
                    dump_attr_out(p['name'], "MPI_Fint", "flag_i")
                elif re.match(r'MPI_Attr_put', func['name'], re.IGNORECASE):
                    dump_attr_in(p['name'], "MPI_Fint")
                elif is_custom_fn:
                    # Must be e.g. MPI_DUP_FN. We'll special treat them
                    c_param_list.append("void *%s" % p['name'])
                    pass
                else:
                    raise Exception("Unhandled: func %s - %s" % (func['name'], p['name']))
            elif re.match(r'MPI_(Aint|Count|Offset)', c_mapping[p['kind']]):
                c_type = c_mapping[p['kind']]
                if re.match(r'MPI_Type_(extent|lb|ub)', func['name'], re.IGNORECASE):
                    # old function, output as INTEGER
                    dump_scalar_out(p['name'], "MPI_Fint", "MPI_Aint")
                elif re.match(r'MPI_Type_hvector', func['name'], re.IGNORECASE):
                    # old function, input as INTEGER
                    dump_p(p)
                elif re.match(r'MPI_Type_(hindexed|struct)', func['name'], re.IGNORECASE):
                    # old function, input as INTEGER array
                    dump_array_int_to_aint(p['name'], '(*count)')
                elif p['length']:
                    dump_direct_pointer(p['name'], c_type)
                elif p['param_direction'] == 'out' or p['param_direction'] == 'inout':
                    if re.match(r'MPI_Address', func['name'], re.IGNORECASE):
                        dump_aint_out_check(p['name'])
                    else:
                        dump_direct_pointer(p['name'], c_type)
                else:
                    dump_c_type_in(p['name'], c_type)

            elif p['kind'] == "ERRHANDLER" and re.match(r'.*_(create_errhandler|errhandler_create)', func['name'], re.IGNORECASE):
                # use MPII_errhan_create(err_fn, errhan, type
                c_param_list.append("MPI_Fint *%s" % p['name'])
                c_arg_list_A.append(p['name'])
            elif p['kind'] == "OPERATION" and re.match(r'.*_op_create$', func['name'], re.IGNORECASE):
                # use MPII_op_create(opfn, *commute, op)
                c_param_list.append("MPI_Fint *%s" % p['name'])
                c_arg_list_A.append(p['name'])
            elif p['kind'] in G.handle_mpir_types or c_mapping[p['kind']] == "int":
                c_type = c_mapping[p['kind']]
                if p['length'] is None:
                    if p['param_direction'] == 'out':
                        dump_int_out(p['name'], c_type, False)
                        if p['kind'] == "KEYVAL":
                            end_list_common.append("if (!*ierr) {")
                            end_list_common.append("    MPII_Keyval_set_f90_proxy((int) *%s);" % p['name'])
                            end_list_common.append("}")
                        elif re.match(r'MPI_Grequest_start', func['name'], re.IGNORECASE):
                            end_list_common.append("if (!*ierr) {")
                            end_list_common.append("    MPII_Grequest_set_lang_f77((int) *%s);" % p['name'])
                            end_list_common.append("}")
                    elif p['param_direction'] == 'inout' or p['pointer']:
                        if re.match(r'MPI_Info_get_string$', func['name'], re.IGNORECASE):
                            dump_string_len_inout(p['name'])
                        else:
                            dump_int_out(p['name'], c_type, True)
                    else:    
                        dump_p(p)
                elif isinstance(p['length'], list):
                    if re.match(r'MPI_Group_range_(incl|excl)', func['name'], re.IGNORECASE):
                        c_param_list.append("MPI_Fint *%s" % p['name'])
                        c_arg_list_A.append("(int (*)[3]) %s" % p['name'])
                        c_arg_list_B.append("(int (*)[3]) %s" % p['name'])
                    else:
                        raise Exception("unhandled length: %s - %s" % (func['name'], p['name']))
                else:
                    # FIXME: find length
                    count = p['length']
                    if count == '*':
                        if re.match(r'MPI_Dist_graph_create$', func['name'], re.IGNORECASE):
                            count = "total_degrees"
                        elif re.match(r'MPI_Cart_rank$', func['name'], re.IGNORECASE):
                            count = "maxdims"
                        elif re.match(r'MPI_Graph_(create|map)$', func['name'], re.IGNORECASE):
                            count = "indx[*nnodes - 1]"
                        elif re.match(r'MPI_(Wait|Test)some$', func['name'], re.IGNORECASE):
                            count = "(*incount)"
                        else:
                            raise Exception("Unhandled array[*]: %s - %s" % (func['name'], p['name']))
                    else:
                        count = "(*%s)" % count

                    if p['param_direction'] == 'out':
                        dump_array_out(p['name'], c_type, count)
                    elif p['param_direction'] == 'inout':
                        dump_array(p['name'], c_type, count, count, True, True)
                    else:    
                        dump_array_in(p['name'], c_type, count)

            else:
                raise Exception("Unhandled: func %s - %s, kind = %s" % (func['name'], p['name'], p['kind']))

    # -----------------------------------------------------------
    # pre-process, such as pre-calculate total dimensions
    if re.match(r'MPI_Dist_graph_create$', func['name'], re.IGNORECASE):
        dump_sum(code_list_B, "total_degrees", "n", "degrees")
    elif re.match(r'MPI_Comm_spawn_multiple$', func['name'], re.IGNORECASE):
        dump_sum(code_list_B, "total_procs", "count", "array_of_maxprocs")
    elif re.match(r'MPI_Cart_(rank|sub)$', func['name'], re.IGNORECASE):
        code_list_B.append("int maxdims;")
        code_list_B.append("MPI_Cartdim_get((MPI_Comm) (*comm), &maxdims);")
    elif re.match(r'mpi_i?neighbor_(.*[vw])(_init)?$', func['name'], re.IGNORECASE):
        code_list_B.append("int indegree, outdegree, weighted;")
        code_list_B.append("MPI_Dist_graph_neighbors_count((MPI_Comm) (*comm), &indegree, &outdegree, &weighted);")
    elif re.match(r'mpi_.*(reduce_scatter)', func['name'], re.IGNORECASE):
        code_list_B.append("int comm_size;")
        code_list_B.append("MPI_Comm_size((MPI_Comm) (*comm), &comm_size);")
    elif re.match(r'mpi_.*(gatherv|scatterv|alltoall[vw])', func['name'], re.IGNORECASE):
        code_list_B.append("int is_inter;")
        code_list_B.append("int comm_size;")
        code_list_B.append("MPI_Comm_test_inter((MPI_Comm) (*comm), &is_inter);")
        code_list_B.append("if (is_inter) {")
        code_list_B.append("    MPI_Comm_remote_size((MPI_Comm) (*comm), &comm_size);")
        code_list_B.append("} else {")
        code_list_B.append("    MPI_Comm_size((MPI_Comm) (*comm), &comm_size);")
        code_list_B.append("}")

    process_func_parameters()

    c_func_name = func_name
    if need_ATTR_AINT:
        if RE.match(r'MPI_Attr_(get|put)', func['name'], re.IGNORECASE):
            if RE.m.group(1) == 'put':
                c_func_name = "MPII_Comm_set_attr"
            else:
                c_func_name = "MPII_Comm_get_attr"
            c_arg_list_A.append("MPIR_ATTR_INT")
            c_arg_list_B.append("MPIR_ATTR_INT")
        else:
            c_func_name = re.sub(r'MPI_', 'MPII_', func['name'])
            c_arg_list_A.append("MPIR_ATTR_AINT")
            c_arg_list_B.append("MPIR_ATTR_AINT")
    elif re.match(r'MPI_(Init|Init_thread|Info_create_env)$', func['name'], re.IGNORECASE):
        # argc, argv
        c_arg_list_A.insert(0, "0, 0")
        c_arg_list_B.insert(0, "0, 0")
    elif re.match(r'.*_op_create$', func['name'], re.IGNORECASE):
        c_func_name = "MPII_op_create"
    elif RE.match(r'MPI_(\w+)_create_errhandler$', func['name'], re.IGNORECASE):
        c_func_name = "MPII_errhan_create"
        c_arg_list_A.append("F77_" + RE.m.group(1).upper())
    elif RE.match(r'MPI_Errhandler_create$', func['name'], re.IGNORECASE):
        c_func_name = "MPII_errhan_create"
        c_arg_list_A.append("F77_COMM")

    if re.match(r'MPI_CONVERSION_FN_NULL', func['name'], re.IGNORECASE):
        param_str = "void *userbuf, MPI_Datatype datatype, int count, void *filebuf, MPI_Offset position, void *extra_state"
        return_type = "int"
    elif 'return' not in func:
        if not need_skip_ierr:
            if c_param_list:
                param_str = ', '.join(c_param_list) + ", MPI_Fint *ierr"
            else:
                param_str = "MPI_Fint *ierr"
        else:
            param_str = ', '.join(c_param_list)
        return_type = "void"
    else:
        if not c_param_list:
            param_str = "void"
        else:
            param_str = ', '.join(c_param_list)
        return_type = c_mapping[func['return']]

    if c_param_list_end:
        param_str += ' ' + ' '.join(c_param_list_end)


    use_name = dump_profiling(func_name, param_str, return_type, is_cptr)
    G.out.append("")
    dump_mpi_decl_begin(use_name, param_str, return_type)

    if is_custom_fn:
        dump_custom_functions(func)
    elif re.match(r'MPI_Pcontrol', func['name'], re.IGNORECASE):
        G.out.append("%s(%s);" % (c_func_name, ', '.join(c_arg_list_A)))
    elif return_type != "void":
        # these are all special functions, e.g. MPI_Wtime, MPI_Wtick, MPI_Aint_add, MPI_Aint_diff
        G.out.append("return %s(%s);" % (c_func_name, ', '.join(c_arg_list_A)))
    else:
        # FIXME: need check name mangling before call mpirinitf_()
        G.out.append("if (MPIR_F_NeedInit) {mpirinitf_(); MPIR_F_NeedInit = 0;}")
        G.out.append("")

        for l in code_list_common:
            G.out.append(l)

        if code_list_B:
            G.out.append("#ifdef HAVE_FINT_IS_INT")

        for l in code_list_A:
            G.out.append(l)
        G.out.append("*ierr = %s(%s);" % (c_func_name, ', '.join(c_arg_list_A)))
        for l in end_list_A:
            G.out.append(l)

        if code_list_B:
            G.out.append("")
            G.out.append("#else  /* ! HAVE_FINT_IS_INT */")

            for l in code_list_B:
                G.out.append(l)
            G.out.append("*ierr = %s(%s);" % (c_func_name, ', '.join(c_arg_list_B)))
            for l in end_list_B:
                G.out.append(l)

            G.out.append("#endif")

        for l in end_list_common:
            G.out.append(l)

    dump_mpi_decl_end()

def dump_f77_c_file(f, lines):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_c:
            print(l, file=Out)
        if f.endswith('.c'):
            print("#include \"mpi_fortimpl.h\"", file=Out)
            print("#include \"fortran_profile.h\"", file=Out)
        print("", file=Out)
        indent = 0
        for l in lines:
            if RE.match(r'#(if|elif|else|endif|define)', l):
                print(l, file=Out)
            elif RE.match(r'INDENT', l):
                indent += 1
            elif RE.match(r'DEDENT', l):
                indent -= 1
            else:
                if indent > 0:
                    print("    " * indent, end='', file=Out)
                print(l, file=Out)

def dump_mpif_h(f):
    print("  --> [%s]" % f)
    # note: fixed-form Fortran line is ignored after column 72
    with open(f, "w") as Out:
        for l in G.copyright_f77:
            print(l, file=Out)

        for a in ['INTEGER', 'ADDRESS', 'COUNT', 'OFFSET']:
            G.mpih_defines['MPI_%s_KIND' % a] = '@%s_KIND@' % a
        G.mpih_defines['MPI_STATUS_SIZE'] = G.mpih_defines['MPI_F_STATUS_SIZE']
        for a in ['SOURCE', 'TAG', 'ERROR']:
            G.mpih_defines['MPI_%s' % a] = int(G.mpih_defines['MPI_F_%s' % a]) + 1

        # -- all integer constants
        for name in G.mpih_defines:
            T = "INTEGER"
            if re.match(r'MPI_[TF]_', name):
                continue
            elif re.match(r'MPI_\w+_FN', name):
                continue
            elif re.match(r'MPI_\w+_FMT_(DEC|HEX)_SPEC', name):
                continue
            elif re.match(r'MPI_(UNWEIGHTED|WEIGHTS_EMPTY|BUFFER_AUTOMATIC|BOTTOM|IN_PLACE|STATUS_IGNORE|STATUSES_IGNORE|ERRCODES_IGNORE|ARGVS_NULL|ARGV_NULL)', name):
                continue
            elif re.match(r'MPI_DISPLACEMENT_CURRENT', name):
                T = '@FORTRAN_MPI_OFFSET@'
            print("       %s %s" % (T, name), file=Out)
            print("       PARAMETER (%s=%s)" % (name, G.mpih_defines[name]), file=Out)

        # -- Fortran08 capability
        for a in ['SUBARRAYS_SUPPORTED', 'ASYNC_PROTECTS_NONBLOCKING']:
            print("       LOGICAL MPI_%s" % a, file=Out)
            print("       PARAMETER(MPI_%s=.FALSE.)" % a, file=Out)

        # -- function constants
        print("       EXTERNAL MPI_DUP_FN, MPI_NULL_DELETE_FN, MPI_NULL_COPY_FN", file=Out)
        print("       EXTERNAL MPI_COMM_DUP_FN, MPI_COMM_NULL_DELETE_FN", file=Out)
        print("       EXTERNAL MPI_COMM_NULL_COPY_FN", file=Out)
        print("       EXTERNAL MPI_WIN_DUP_FN, MPI_WIN_NULL_DELETE_FN", file=Out)
        print("       EXTERNAL MPI_WIN_NULL_COPY_FN", file=Out)
        print("       EXTERNAL MPI_TYPE_DUP_FN, MPI_TYPE_NULL_DELETE_FN", file=Out)
        print("       EXTERNAL MPI_TYPE_NULL_COPY_FN", file=Out)
        print("       EXTERNAL MPI_CONVERSION_FN_NULL", file=Out)
        # -- MPI_Wtime, MPI_Wtick, MPI_Aint_add, MPI_Aint_diff
        for a in ['Wtime', 'Wtick', 'Aint_add', 'Aint_diff']:
            T = "DOUBLE PRECISION"
            if a.startswith("Aint"):
                T = "INTEGER(KIND=MPI_ADDRESS_KIND)"
            print("       EXTERNAL MPI_%s, PMPI_%s" % (a.upper(), a.upper()), file=Out)
            print("       %s MPI_%s, PMPI_%s" % (T, a.upper(), a.upper()), file=Out)
        # -- common blocks
        print("       INTEGER MPI_BOTTOM, MPI_IN_PLACE, MPI_UNWEIGHTED(1)", file=Out)
        print("       INTEGER MPI_WEIGHTS_EMPTY(1)", file=Out)
        print("       INTEGER MPI_BUFFER_AUTOMATIC(1)", file=Out)
        print("       INTEGER MPI_STATUS_IGNORE(MPI_STATUS_SIZE)", file=Out)
        print("       INTEGER MPI_STATUSES_IGNORE(MPI_STATUS_SIZE,1)", file=Out)
        print("       INTEGER MPI_ERRCODES_IGNORE(1)", file=Out)
        print("       CHARACTER*1 MPI_ARGVS_NULL(1,1)", file=Out)
        print("       CHARACTER*1 MPI_ARGV_NULL(1)", file=Out)
        print("@DLLIMPORT@", file=Out)
        print("       COMMON /MPIFCMB5/ MPI_UNWEIGHTED", file=Out)
        print("       COMMON /MPIFCMB9/ MPI_WEIGHTS_EMPTY", file=Out)
        print("       COMMON /MPIFCMBa/ MPI_BUFFER_AUTOMATIC", file=Out)
        print("       COMMON /MPIPRIV1/ MPI_BOTTOM, MPI_IN_PLACE, MPI_STATUS_IGNORE", file=Out)
        print("       COMMON /MPIPRIV2/ MPI_STATUSES_IGNORE, MPI_ERRCODES_IGNORE", file=Out)
        print("       COMMON /MPIPRIVC/ MPI_ARGVS_NULL, MPI_ARGV_NULL", file=Out)
        print("       SAVE /MPIFCMB5/, /MPIFCMB9/, /MPIFCMBa/", file=Out)
        print("       SAVE /MPIPRIV1/, /MPIPRIV2/, /MPIPRIVC/", file=Out)

def load_mpi_h_in(f):
    # load constants into G.mpih_defines
    with open(f, "r") as In:
        for line in In:
            # trim trailing comments
            line = re.sub(r'\s+\/\*.*', '', line)
            if RE.match(r'#define\s+(MPI_\w+)\s+(.+)', line):
                # direct macros
                (name, val) = RE.m.group(1, 2)
                if re.match(r'MPI_FILE_NULL', name):
                    val = 0
                elif re.match(r'MPI_(LONG_LONG|C_COMPLEX)', name):
                    # datatype aliases
                    val = "@F77_%s@" % name
                elif re.match(r'\(?\(MPI_Datatype\)\@(MPIR?_\w+)\@\)?', val):
                    # datatypes
                    if re.match(r'MPI_(AINT|OFFSET|COUNT)', name):
                        val = "@F77_%s_DATATYPE@" % name
                    elif RE.match(r'MPI_CXX_(\w+)', name):
                        val = "@F77_MPIR_CXX_%s@" % RE.m.group(1)
                    else:
                        val = "@F77_%s@" % name
                elif RE.match(r'\(+MPI_\w+\)\(?0x([0-9a-fA-F]+)', val):
                    # handle constants
                    val = int(RE.m.group(1), 16)
                elif RE.match(r'0x([0-9a-fA-F]+)', val):
                    # direct hex constants (KEYVAL constants)
                    val = int(RE.m.group(1), 16)
                    if RE.match(r'MPI_(TAG_UB|HOST|IO|WTIME_IS_GLOBAL|UNIVERSE_SIZE|LASTUSEDCODE|APPNUM|WIN_(BASE|SIZE|DISP_UNIT|CREATE_FLAVOR|MODEL))', name):
                        # KEYVAL, Fortran value is C-value + 1
                        val = val + 1
                elif RE.match(r'MPI_MAX_', name):
                    # Fortran string buffer limit need be 1-less
                    if re.match(r'@\w+@', val):
                        val += "-1"
                    else:
                        val = int(val) - 1
                elif RE.match(r'\(([-\d]+)\)', val):
                    # take off the extra parentheses
                    val = RE.m.group(1)

                G.mpih_defines[name] = val
            elif RE.match(r'\s+(MPI_\w+)\s*=\s*(\d+)', line):
                # enum values
                (name, val) = RE.m.group(1, 2)
                G.mpih_defines[name] = val

#---------------------------------------- 
def dump_profiling(name, param_str, return_type, is_cptr):
    if is_cptr:
        name = name + '_cptr'
    pname = "P" + name
    defines = ["F77_NAME_UPPER", "F77_NAME_LOWER", "F77_NAME_LOWER_USCORE", "F77_NAME_LOWER_2USCORE"]
    names = [name.upper(), name.lower(), name.lower() + "_", name.lower() + "__"]
    pnames = [pname.upper(), pname.lower(), pname.lower() + "_", pname.lower() + "__"]
    # use e.g. mpi_send_ to define the function
    use_idx = 2

    def dump_multiple_pragma_weak(use_only_mpi_names):
        for nm in names:
            dump_mpi_decl(nm, param_str)
        G.profile_out.append("")
        for i in range(len(names)):
            G.profile_out.append("#%s defined(%s)" % (get_if_or_elif(i), defines[i]))
            for j in range(len(names)):
                if use_only_mpi_names:
                    if i == j:
                        continue
                    G.profile_out.append("#pragma weak %s = %s" % (names[j], names[i]))
                else:    
                    G.profile_out.append("#pragma weak %s = %s" % (names[j], pnames[i]))
        G.profile_out.append("#else")
        G.profile_out.append("#error missing F77 name mangling")
        G.profile_out.append("#endif")

    def dump_elif_pragma_weak(have_pragma):
        G.profile_out.append("#elif defined(%s)" % have_pragma)

        for i in range(len(names)):
            G.profile_out.append("#%s defined(%s)" % (get_if_or_elif(i), defines[i]))
            if have_pragma == "HAVE_PRAGMA_WEAK":
                dump_mpi_decl(names[i], param_str)
                G.profile_out.append("#pragma weak %s = %s" % (names[i], pnames[i]))
            elif have_pragma == "HAVE_PRAGMA_HP_SEC_DEF":
                G.profile_out.append("#pragma _HP_SECONDARY_DEF %s %s" % (pnames[i], names[i]))
            elif have_pragma == "HAVE_PRAGMA_CRI_DUP":
                G.profile_out.append("#pragma _CRI duplicate %s as %s" % (names[i], pnames[i]))
        G.profile_out.append("#endif")
        G.profile_out.append("")

    def dump_weak_attribute(use_only_mpi_names):
        for i in range(len(names)):
            G.profile_out.append("#%s defined(%s)" % (get_if_or_elif(i), defines[i]))
            for j in range(len(names)):
                if use_only_mpi_names:
                    if i == j:
                        dump_mpi_decl(names[i], param_str)
                    else:
                        dump_mpi_decl_weak_attr(names[j], param_str, names[i])
                else:    
                    dump_mpi_decl_weak_attr(names[j], param_str, pnames[i])
        G.profile_out.append("#else")
        G.profile_out.append("#error missing F77 name mangling")
        G.profile_out.append("#endif")

    def dump_pmpi_decl_multiple_pragma_weak():
        for i in range(len(names)):
            G.profile_out.append("#ifndef %s" % defines[i])
            dump_mpi_decl(pnames[i], param_str)
            G.profile_out.append("#endif")

        for i in range(len(names)):
            G.profile_out.append("#%s defined(%s)" % (get_if_or_elif(i), defines[i]))
            for j in range(len(names)):
                if i != j:
                    G.profile_out.append("#pragma weak %s = %s" % (pnames[j], pnames[i]))
        G.profile_out.append("#else")
        G.profile_out.append("#error missing F77 name mangling")
        G.profile_out.append("#endif")

    def dump_pmpi_decl_weak_attribute():
        for i in range(len(names)):
            G.profile_out.append("#%s defined(%s)" % (get_if_or_elif(i), defines[i]))
            for j in range(len(names)):
                if i != j:
                    dump_mpi_decl_weak_attr(pnames[j], param_str, pnames[i])
        G.profile_out.append("#else")
        G.profile_out.append("#error missing F77 name mangling")
        G.profile_out.append("#endif")

    def dump_define_mpi_as_pmpi():
        for i in range(len(names)):
            G.profile_out.append("#%s defined(%s)" % (get_if_or_elif(i), defines[i]))
            G.profile_out.append("#define %s %s" % (names[use_idx], pnames[i]))
        G.profile_out.append("#endif")
        G.profile_out.append("")

        # This defines the routine that we call, which must be the PMPI version
        # since we're renaming the Fortran entry as the pmpi version.  The MPI name
        # must be undefined first to prevent any conflicts with previous renamings.
        G.profile_out.append("#ifdef F77_USE_PMPI")
        use_name = name[0:5].upper() + name[5:]
        G.profile_out.append("#undef %s" % use_name)
        G.profile_out.append("#define %s P%s" % (use_name, use_name))
        G.profile_out.append("#endif")

    def dump_define_mpi_as_mangle():
        for i in range(len(names)):
            if i != use_idx:
                G.profile_out.append("#%s defined(%s)" % (get_if_or_elif(i), defines[i]))
                G.profile_out.append("#define %s %s" % (names[use_idx], names[i]))
        G.profile_out.append("#endif")

    # ---- general util ----
    def get_if_or_elif(i):
        if i == 0:
            return "if"
        else:
            return "elif"
        
    def dump_mpi_decl(name, param_str):
        G.profile_out.append("extern FORT_DLL_SPEC %s FORT_CALL %s(%s);" % (return_type, name, param_str))

    def dump_mpi_decl_weak_attr(name, param_str, weak_name):
        G.profile_out.append("extern FORT_DLL_SPEC %s FORT_CALL %s(%s) __attribute__((weak,alias(\"%s\")));" % (return_type, name, param_str, weak_name))

    # ---------        
    G.profile_out.append("")
    G.profile_out.append("/* ---- %s ---- */" % name)
    G.profile_out.append("#if defined(USE_WEAK_SYMBOLS) && !defined(USE_ONLY_MPI_NAMES)")
    G.profile_out.append("#if defined(HAVE_MULTIPLE_PRAGMA_WEAK)")
    dump_multiple_pragma_weak(False)
    G.profile_out.append("")
    dump_elif_pragma_weak("HAVE_PRAGMA_WEAK")
    dump_elif_pragma_weak("HAVE_PRAGMA_HP_SEC_DEF")
    dump_elif_pragma_weak("HAVE_PRAGMA_CRI_DUP")
    G.profile_out.append("#elif defined(HAVE_WEAK_ATTRIBUTE)")
    dump_weak_attribute(False)
    G.profile_out.append("")
    G.profile_out.append("#endif  /* HAVE_MULTIPLE_PRAGMA_WEAK, HAVE_PRAGMA_WEAK, HAVE_WEAK_ATTRIBUTE */")
    G.profile_out.append("#endif  /* USE_WEAK_SYMBOLS && !USE_ONLY_MPI_NAMES */")
    G.profile_out.append("")

    G.profile_out.append("#if defined(USE_WEAK_SYMBOLS) && defined(USE_ONLY_MPI_NAMES)")
    G.profile_out.append("#if defined(HAVE_MULTIPLE_PRAGMA_WEAK)")
    dump_multiple_pragma_weak(True)
    # no weak pragma since the names will be identical
    G.profile_out.append("")
    G.profile_out.append("#elif defined(HAVE_WEAK_ATTRIBUTE)")
    dump_weak_attribute(True)
    G.profile_out.append("")
    G.profile_out.append("#endif  /* HAVE_MULTIPLE_PRAGMA_WEAK, HAVE_PRAGMA_WEAK, HAVE_WEAK_ATTRIBUTE */")
    G.profile_out.append("#endif  /* USE_WEAK_SYMBOLS && USE_ONLY_MPI_NAMES */")
    G.profile_out.append("")

    G.profile_out.append("#ifndef MPICH_MPI_FROM_PMPI")

    G.profile_out.append("#if defined(USE_WEAK_SYMBOLS)")
    G.profile_out.append("#if defined(HAVE_MULTIPLE_PRAGMA_WEAK)")
    G.profile_out.append("")
    dump_pmpi_decl_multiple_pragma_weak()
    G.profile_out.append("")
    G.profile_out.append("#elif defined(HAVE_WEAK_ATTRIBUTE)")
    dump_pmpi_decl_weak_attribute()
    G.profile_out.append("")
    G.profile_out.append("#endif")
    G.profile_out.append("#endif  /* USE_WEAK_SYMBOLS */")
    dump_define_mpi_as_pmpi()
    G.profile_out.append("")

    G.profile_out.append("#else  /* MPICH_MPI_FROM_PMPI */")
    dump_define_mpi_as_mangle()
    G.profile_out.append("")

    G.profile_out.append("#endif  /* MPICH_MPI_FROM_PMPI */")

    # prototype
    G.profile_out.append("")
    G.profile_out.append("FORT_DLL_SPEC %s FORT_CALL %s(%s);" % (return_type, names[use_idx], param_str))

    # return the name to be used to define the function
    return names[use_idx]

# -------------------------------
def dump_fortran_line(s):
    tlist = split_line_with_break(s, '', 100)
    n = len(tlist)
    if n > 1:
        for i in range(n-1):
            tlist[i] = tlist[i] + ' &'
    G.out.extend(tlist)

# -------------------------------
def check_func_directives(func):
    if 'dir' in func and func['dir'] == "mpit":
        func['_skip_fortran'] = 1
    elif RE.match(r'mpix_(grequest_|type_iov|async_)', func['name'], re.IGNORECASE):
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_\w+_(f|f08|c)2(f|f08|c)$', func['name'], re.IGNORECASE):
        # implemented in mpi_f08_types.f90
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_.*_function$', func['name'], re.IGNORECASE):
        # defined in mpi_f08_callbacks.f90
        func['_skip_fortran'] = 1

def f90_param_need_skip(p):
    if RE.search(r'suppress=.*f90_parameter', p['t']):
        return True
    if p['kind'] == 'VARARGS':
        return True
    return False
