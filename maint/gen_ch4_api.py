##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import re
import os

class G:
    apis = []
    name_types = {}

class RE:
    m = None
    def match(pat, str, flags=0):
        RE.m = re.match(pat, str, flags)
        return RE.m
    def search(pat, str, flags=0):
        RE.m = re.search(pat, str, flags)
        return RE.m

def main():

    load_ch4_api("./src/mpid/ch4/ch4_api.txt")

    os.system("mkdir -p src/mpid/ch4/shm/include")
    dump_netmod_h("src/mpid/ch4/netmod/include/netmod.h")
    dump_shm_h("src/mpid/ch4/shm/include/shm.h")
    dump_netmod_impl_h("src/mpid/ch4/netmod/include/netmod_impl.h")
    dump_netmod_impl_c("src/mpid/ch4/netmod/src/netmod_impl.c")

    for m in "ofi", "ucx", "stubnm":
        dump_func_table_c("src/mpid/ch4/netmod/%s/func_table.c" % (m), m)

    for m in "ofi", "ucx", "stubnm":
        dump_noinline_h("src/mpid/ch4/netmod/%s/%s_noinline.h" % (m, m), m, is_nm=True)
    dump_noinline_h("src/mpid/ch4/shm/src/shm_noinline.h", 'shm', is_nm=False)
    dump_noinline_h("src/mpid/ch4/shm/posix/posix_noinline.h", 'posix', is_nm=False)
    dump_noinline_h("src/mpid/ch4/shm/stubshm/shm_noinline.h", 'stubshm', is_nm=False)

    dump_stub("src/mpid/ch4/netmod/stubnm", 'stubnm', is_nm=True)
    dump_stub("src/mpid/ch4/shm/stubshm", 'stubshm', is_nm=False)

# ---- subroutines --------------------------------------------
def load_ch4_api(ch4_api_txt):
    cur_api, flag = '', ''
    with open(ch4_api_txt, "r") as In:
        for line in In:
            if RE.match(r'(.*API|PARAM):', line):
                flag = RE.m.group(1)
            elif re.search(r'API$', flag):
                if RE.match(r'\s+(NM|SHM)(\*?)\s*:\s*(.+)', line):
                    nm, inline, t = RE.m.group(1, 2, 3)
                    tlist = re.split(r',\s*', t)
                    if nm == "NM":
                        cur_api['nm_params'] = tlist
                        cur_api['nm_inline'] = inline
                    elif nm == 'SHM':
                        cur_api['shm_params'] = tlist
                        cur_api['shm_inline'] = inline
                elif RE.match(r'\s+(\w+)\s*:\s*(.+)', line):
                    name, ret = RE.m.group(1, 2)
                    cur_api = {'name': name, 'ret': ret}
                    G.apis.append(cur_api)

                    if re.match(r'Native API', flag):
                        cur_api['native'] = 1
                    else:
                        cur_api['native'] = 0
            elif flag == "PARAM":
                if RE.match(r'\s+(\S+):\s*(.+)', line):
                    k, v = RE.m.group(1, 2)
                    G.name_types[k] = v

def dump_netmod_h(h_file):
    """Dumps netmod.h, which includes native and non-native api function table struct,
    as well as prototypes for all netmod api functions"""
    api_list = [a for a in G.apis if 'nm_params' in a]
    print("  --> [%s]" % h_file)
    with open(h_file, "w") as Out:
        dump_copyright(Out)
        INC = get_include_guard(h_file)
        print("#ifndef %s" % INC, file=Out)
        print("#define %s" % INC, file=Out)
        print("", file=Out)
        print("#include <mpidimpl.h>", file=Out)
        print("", file=Out)
        print("#define MPIDI_MAX_NETMOD_STRING_LEN 64", file=Out)
        print("", file=Out)
        print("typedef union {", file=Out)
        print("#ifdef HAVE_CH4_NETMOD_OFI", file=Out)
        print("    MPIDI_OFI_Global_t ofi;", file=Out)
        print("#endif", file=Out)
        print("#ifdef HAVE_CH4_NETMOD_UCX", file=Out)
        print("    MPIDI_UCX_Global_t ucx;", file=Out)
        print("#endif", file=Out)
        print("} MPIDI_NM_Global_t;", file=Out)
        print("", file=Out)
        for a in api_list:
            s = "typedef "
            s += a['ret']
            s += get_space_after_type(a['ret'])
            s += "(*MPIDI_NM_%s_t) (" % (a['name'])
            dump_s_param_tail(Out, s, a['nm_params'], ");")
        print("", file=Out)
        print("typedef struct MPIDI_NM_funcs {", file=Out)
        for a in api_list:
            if not a['native']:
                s = "    "
                s += "MPIDI_NM_%s_t %s;" % (a['name'], a['name'])
                print(s, file=Out)
        print("} MPIDI_NM_funcs_t;", file=Out)
        print("", file=Out)
        print("typedef struct MPIDI_NM_native_funcs {", file=Out)
        for a in api_list:
            if a['native']:
                s = "    "
                s += "MPIDI_NM_%s_t %s;" % (a['name'], a['name'])
                print(s, file=Out)
        print("} MPIDI_NM_native_funcs_t;", file=Out)
        print("", file=Out)
        print("extern MPIDI_NM_funcs_t *MPIDI_NM_funcs[];", file=Out)
        print("extern MPIDI_NM_funcs_t *MPIDI_NM_func;", file=Out)
        print("extern MPIDI_NM_native_funcs_t *MPIDI_NM_native_funcs[];", file=Out)
        print("extern MPIDI_NM_native_funcs_t *MPIDI_NM_native_func;", file=Out)
        print("extern int MPIDI_num_netmods;", file=Out)
        print("extern char MPIDI_NM_strings[][MPIDI_MAX_NETMOD_STRING_LEN];", file=Out)
        print("", file=Out)
        for a in api_list:
            s = ''
            s += a['ret']
            s += get_space_after_type(a['ret'])
            s += "MPIDI_NM_%s(" % (a['name'])
            tail = ");"
            if a['nm_inline']:
                s = "MPL_STATIC_INLINE_PREFIX %s" % (s)
                tail = ") MPL_STATIC_INLINE_SUFFIX;"
            dump_s_param_tail(Out, s, a['nm_params'], tail)
        print("", file=Out)
        print("#endif /* %s */" % INC, file=Out)

def dump_shm_h(h_file):
    """Dumps shm.h, which includes prototypes for all shmem api functions"""
    print("  --> [%s]" % h_file)
    with open(h_file, "w") as Out:
        dump_copyright(Out)
        INC = get_include_guard(h_file)
        print("#ifndef %s" % INC, file=Out)
        print("#define %s" % INC, file=Out)
        print("", file=Out)
        print("#include <mpidimpl.h>", file=Out)
        print("", file=Out)
        for a in G.apis:
            if 'shm_params' not in a:
                continue
            s = ''
            s += a['ret']
            s += get_space_after_type(a['ret'])
            s += "MPIDI_SHM_%s(" % (a['name'])
            tail = ");"
            if a['shm_inline']:
                s = "MPL_STATIC_INLINE_PREFIX %s" % (s)
                tail = ") MPL_STATIC_INLINE_SUFFIX;"
            dump_s_param_tail(Out, s, a['shm_params'], tail)
        print("", file=Out)
        print("#endif /* %s */" % INC, file=Out)

def dump_netmod_impl_h(h_file):
    """Dumps netmod_impl.h.
    If NETMOD_INLINE is not defined, this file defines inline wrapper functions that calls
    function tables entries. (Non-inline wrappers are defined in netmod_impl.c).
    If NETMOD_INLINE is defined, this file includes the inline headers."""
    api_list = [a for a in G.apis if ('nm_params' in a) and a['nm_inline']]
    print("  --> [%s]" % h_file)
    with open(h_file, "w") as Out:
        dump_copyright(Out)
        INC = get_include_guard(h_file)
        print("#ifndef %s" % INC, file=Out)
        print("#define %s" % INC, file=Out)
        print("", file=Out)
        print("#ifndef NETMOD_INLINE", file=Out)
        print("#ifndef NETMOD_DISABLE_INLINES", file=Out)
        for a in api_list:
            s = "MPL_STATIC_INLINE_PREFIX "
            s += a['ret'] + get_space_after_type(a['ret'])
            s += "MPIDI_NM_%s(" % (a['name'])
            dump_s_param_tail(Out, s, a['nm_params'], ")")

            print("{", file=Out)
            use_ret = ''
            if not re.match(r'void\s*$', a['ret']):
                use_ret = 1
                print("    %s ret;" % a['ret'], file=Out)
                print("", file=Out)
            NAME = a['name'].upper()
            print("    MPIR_FUNC_ENTER;", file=Out)
            print("", file=Out)
            s = "    "
            if use_ret:
                s += "ret = "
            if a['native']:
                s += "MPIDI_NM_native_func->%s(" % (a['name'])
            else:
                s += "MPIDI_NM_func->%s(" % (a['name'])
            dump_s_param_tail(Out, s, a['nm_params'], ");", 1)
            print("", file=Out)

            print("    MPIR_FUNC_EXIT;", file=Out)
            if use_ret:
                print("    return ret;", file=Out)
            print("}", file=Out)
            print("", file=Out)
        print("#endif /* NETMOD_DISABLE_INLINES */", file=Out)
        print("", file=Out)
        print("#else", file=Out)
        print("#define __netmod_inline_stubnm__   0", file=Out)
        print("#define __netmod_inline_ofi__   1", file=Out)
        print("#define __netmod_inline_ucx__   2", file=Out)
        print("", file=Out)
        print("#if NETMOD_INLINE==__netmod_inline_stubnm__", file=Out)
        print("#include \"../stubnm/netmod_inline.h\"", file=Out)
        print("#elif NETMOD_INLINE==__netmod_inline_ofi__", file=Out)
        print("#include \"../ofi/netmod_inline.h\"", file=Out)
        print("#elif NETMOD_INLINE==__netmod_inline_ucx__", file=Out)
        print("#include \"../ucx/netmod_inline.h\"", file=Out)
        print("#else", file=Out)
        print("#error \"No direct netmod included\"", file=Out)
        print("#endif", file=Out)
        print("#endif /* NETMOD_INLINE */", file=Out)
        print("", file=Out)
        print("#endif /* %s */" % INC, file=Out)

def dump_netmod_impl_c(c_file):
    """Dump netmod_impl.c, which supply non-inlined wrapper functions that calls function table
    entries"""
    api_list = [a for a in G.apis if ('nm_params' in a) and not a['nm_inline']]
    print("  --> [%s]" % c_file)
    with open(c_file, "w") as Out:
        dump_copyright(Out)
        print("", file=Out)
        print("#include \"mpidimpl.h\"", file=Out)
        print("", file=Out)
        print("#ifndef NETMOD_INLINE", file=Out)
        for a in api_list:
            s = a['ret'] + get_space_after_type(a['ret'])
            s += "MPIDI_NM_%s(" % (a['name'])
            dump_s_param_tail(Out, s, a['nm_params'], ")")

            print("{", file=Out)
            use_ret = ''
            if not re.match(r'void\s*$', a['ret']):
                use_ret = 1
                print("    %s ret;" % a['ret'], file=Out)
                print("", file=Out)
            NAME = a['name'].upper()
            print("    MPIR_FUNC_ENTER;", file=Out)
            print("", file=Out)
            s = "    "
            if use_ret:
                s += "ret = "
            if a['native']:
                s += "MPIDI_NM_native_func->%s(" % (a['name'])
            else:
                s += "MPIDI_NM_func->%s(" % (a['name'])
            dump_s_param_tail(Out, s, a['nm_params'], ");", 1)
            print("", file=Out)

            print("    MPIR_FUNC_EXIT;", file=Out)
            if use_ret:
                print("    return ret;", file=Out)
            print("}", file=Out)
            print("", file=Out)
        print("#endif /* NETMOD_INLINE */", file=Out)

def dump_func_table_c(c_file, mod):
    """Dumps func_table.c. When NETMOD_INLINE is not defined, this file defines function table
    by filling entries. NETMOD_DISABLE_INLINES is a hack to prevent wrapers in netmod_impl.h
    gets included"""
    MOD = mod.upper()
    print("  --> [%s]" % c_file)
    with open(c_file, "w") as Out:
        dump_copyright(Out)
        print("#ifndef NETMOD_INLINE", file=Out)
        print("#define NETMOD_DISABLE_INLINES", file=Out)
        print("#include <mpidimpl.h>", file=Out)
        print("MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;", file=Out)
        print("", file=Out)
        print("#include \"netmod_inline.h\"", file=Out)
        print("MPIDI_NM_funcs_t MPIDI_NM_%s_funcs = {" % mod, file=Out)
        for a in G.apis:
            if 'nm_params' not in a:
                continue
            if not a['native']:
                if a['nm_inline']:
                    print("    .%s = MPIDI_NM_%s," % (a['name'], a['name']), file=Out)
                else:
                    print("    .%s = MPIDI_%s_%s," % (a['name'], MOD, a['name']), file=Out)
        print("};", file=Out)
        print("", file=Out)
        print("MPIDI_NM_native_funcs_t MPIDI_NM_native_%s_funcs = {" % mod, file=Out)
        for a in G.apis:
            if 'nm_params' not in a:
                continue
            if a['native']:
                if a['nm_inline']:
                    print("    .%s = MPIDI_NM_%s," % (a['name'], a['name']), file=Out)
                else:
                    print("    .%s = MPIDI_%s_%s," % (a['name'], MOD, a['name']), file=Out)
        print("};", file=Out)
        print("#endif", file=Out)

def dump_noinline_h(h_file, mod, is_nm=True):
    """Dumps noinline header files e.g. ofi_noinline.h. "noinline" header files includes
    prototypes of non-inline functions."""
    api_list = []
    for a in G.apis:
        if is_nm and ('nm_params' in a) and not a['nm_inline']:
            api_list.append(a)
        elif ('shm_params' in a) and not a['shm_inline']:
            api_list.append(a)

    MOD = mod.upper()
    print("  --> [%s]" % h_file)
    with open(h_file, "w") as Out:
        dump_copyright(Out)
        INC = get_include_guard(h_file)
        print("#ifndef %s" % INC, file=Out)
        print("#define %s" % INC, file=Out)
        print("", file=Out)
        print("#include \"%s_impl.h\"" % mod, file=Out)
        print("", file=Out)
        for a in api_list:
            s = a['ret'] + get_space_after_type(a['ret'])
            s += "MPIDI_%s_%s(" % (MOD, a['name'])
            if is_nm:
                params = a['nm_params']
            else:
                params = a['shm_params']
            dump_s_param_tail(Out, s, params, ");")
        print("", file=Out)

        if is_nm:
            # if netmod is inlined, the api is called by `MPIDI_NM_xxx`,
            # define macros to redirect.
            print("#ifdef NETMOD_INLINE", file=Out)
            for a in api_list:
                print("#define MPIDI_NM_%s MPIDI_%s_%s" % (a['name'], MOD, a['name']), file=Out)
            print("#endif /* NETMOD_INLINE */", file=Out)
            print("", file=Out)

        print("#endif /* %s */" % INC, file=Out)

def dump_stub_file(stub_file, mod, is_inline, is_nm):
    """Dumps boilerplate stub implementations"""
    api_list = []
    for a in G.apis:
        if is_nm:
            if 'nm_params' not in a:
                continue
            a_is_inline = a['nm_inline']
        else:
            if 'shm_params' not in a:
                continue
            a_is_inline = a['shm_inline']
        if is_inline and (not a_is_inline):
            continue
        elif (not is_inline) and a_is_inline:
            continue

        api_list.append(a)

    MOD = mod.upper()
    print("  --> [%s]" % stub_file)
    with open(stub_file, "w") as Out:
        dump_copyright(Out)
        if is_inline:
            INC = get_include_guard(stub_file)
            print("#ifndef %s" % INC, file=Out)
            print("#define %s" % INC, file=Out)
            print("", file=Out)
        print("#include <mpidimpl.h>", file=Out)

        for a in api_list:
            print("", file=Out)
            s = ""
            if is_inline:
                s += "MPL_STATIC_INLINE_PREFIX "
            s += a['ret'] + get_space_after_type(a['ret'])
            if is_inline:
                if is_nm:
                    s += "MPIDI_NM_%s(" % (a['name'])
                else:
                    s += "MPIDI_SHM_%s(" % (a['name'])
            else:
                s += "MPIDI_%s_%s(" % (MOD, a['name'])
            if is_nm:
                params = a['nm_params']
            else:
                params = a['shm_params']
            dump_s_param_tail(Out, s, params, ")")
            print("{", file=Out)
            if a['ret'] == 'int':
                print("    int mpi_errno = MPI_SUCCESS;", file=Out)
                print("    MPIR_Assert(0);", file=Out)
                print("    return mpi_errno;", file=Out)
            elif a['ret'] == 'void':
                print("    MPIR_Assert(0);", file=Out)
                print("    return;", file=Out)
            else:
                print("    MPIR_Assert(0);", file=Out)
                print("    return 0;", file=Out)
            print("}", file=Out)
        if is_inline:
            print("", file=Out)
            print("/* Not-inlined %s netmod functions */" % MOD, file=Out)
            print("#include \"%s_noinline.h\"" % mod, file=Out)
            print("", file=Out)
            print("#endif /* %s */" % INC, file=Out)

def dump_stub(stub_dir, mod, is_nm):
    if is_nm:
        dump_stub_file("%s/netmod_inline.h" % stub_dir, mod, True, is_nm)
    else:
        dump_stub_file("%s/shm_inline.h" % stub_dir, mod, True, is_nm)
    dump_stub_file("%s/%s_noinline.c" % (stub_dir, mod), mod, False, is_nm)

# ---- utils ------------------------------------------------------------
def dump_copyright(out):
    print("/*", file=out)
    print(" * Copyright (C) by Argonne National Laboratory", file=out)
    print(" *     See COPYRIGHT in top-level directory", file=out)
    print(" */", file=out)
    print("", file=out)
    print("/* ** This file is auto-generated, do not edit ** */", file=out)
    print("", file=out)

def get_include_guard(h_file):
    h_file = re.sub(r'.*\/', '', h_file)
    h_file = re.sub(r'\.', '_', h_file)
    return h_file.upper() + "_INCLUDED"

def get_space_after_type(type):
    if re.search(r'\b(void|char|int|short|long|float|double) \*+$', type) or re.match(r'struct .* \*+$', type):
        return ''
    else:
        return ' '

def dump_s_param_tail(out, s, params, tail, is_arg=False):
    """Print to file out a function declaration or a function call. 
    Essentially it will print a string as a concatenation of s, params, and tail.
    It will expand params to the correct type depend on whether it is a declaration
    or function call, and it will break the long line at 100 character limit.
    """
    count = len(params)
    n_lead = len(s)
    n = n_lead
    space = ""

    if count == 1:
        t = get_param_phrase(params[0], is_arg)
        if n + len(t) + len(tail) <= 100:
            print("%s%s%s" % (s, t, tail), file=out)
        elif re.search(r'(.*) MPL_STATIC_INLINE_SUFFIX;', tail):
            print("%s%s)" % (s, t), file=out)
            print("    MPL_STATIC_INLINE_SUFFIX;", file=out)
        else:
            print("%s%s%s" % (s, t, tail), file=out)
        return

    for i, p in enumerate(params):
        t = get_param_phrase(p, is_arg)
        if i < count - 1:
            t += ","
        else:
            t += "%s" % (tail)

        if n + len(space) + len(t) <= 100:
            s += "%s%s" % (space, t)
        else:
            print(s, file=out)
            s = ' ' * n_lead + t
        n = len(s)
        space = ' '

    if len(s) > 100 and RE.search(r'(.*) MPL_STATIC_INLINE_SUFFIX;', s):
        t = RE.m.group(1)
        print(t, file=out)
        print("    MPL_STATIC_INLINE_SUFFIX;", file=out)
    else:
        print(s, file=out)

def get_param_phrase(name, is_arg):
    p = name
    p = re.sub(r'-\d+$', '', p)
    if is_arg:
        if name == "void":
            return ""
        else:
            return p

    t = G.name_types[name]
    if RE.search(r'(.*)\[\]', t):
        t = RE.m.group(1)
        p += '[]'

    if name == "void":
        return "void"
    else:
        return t + get_space_after_type(t) + p

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
