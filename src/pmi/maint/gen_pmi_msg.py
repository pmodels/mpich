##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import re
import os

class G:
    pmi_vers = []
    cmd_list = []
    cmd_hash = {}

class RE:
    m = None
    def match(pat, str, flags=0):
        RE.m = re.match(pat, str, flags)
        return RE.m
    def search(pat, str, flags=0):
        RE.m = re.search(pat, str, flags)
        return RE.m

def main():
    # run from pmi top_srcdir
    load_pmi_txt("maint/pmi-1.1.txt", "1.1")
    load_pmi_txt("maint/pmi-2.0.txt", "2.0")

    dump_all()

def load_pmi_txt(pmi_txt, ver):
    cur_hash = {"version": ver}
    G.pmi_vers.append(cur_hash)

    prev_cmd = None
    cur_cmd = None
    cur_attrs = None
    with open(pmi_txt, "r") as In:
        for line in In:
            if RE.match(r'([A-Z]+):', line):
                name = RE.m.group(1)
                cur_cmd = {"version": ver} # query-name, query-attrs, response-name, response-attrs
                cur_hash[name] = cur_cmd 
                if name not in G.cmd_hash:
                    G.cmd_list.append(name)
                    G.cmd_hash[name] = cur_cmd
                    prev_cmd = None
                else:
                    prev_cmd = G.cmd_hash[name]
            elif RE.match(r'\s+([QR]):\s*([\w-]+)(.*)', line):
                QR, cmd, tail = RE.m.group(1, 2, 3)
                cur_attrs = []
                if QR == "Q":
                    cur_cmd["query-name"] = cmd
                    if RE.match(r'.*wire=.+', tail):
                        # spawn - we'll manually code it
                        cur_cmd["query-attrs"] = []
                    else:
                        cur_cmd["query-attrs"] = cur_attrs
                else:
                    cur_cmd["response-name"] = cmd
                    cur_cmd["response-attrs"] = cur_attrs
            elif RE.match(r'\s+([\w-]+):\s*([A-Z]+)(.*)', line):
                name, kind, tail = RE.m.group(1, 2, 3)
                cur_attrs.append([name, kind, tail])
            elif RE.match(r'\s+([\[\]])', line):
                cur_attrs.append(RE.m.group(1))

def dump_all():
    def dump_enums(Out):
        print("enum PMIU_CMD_ID {", file=Out)
        print("    PMIU_CMD_INVALID,", file=Out)
        for NAME in G.cmd_list:
            print("    PMIU_CMD_%s," % NAME, file=Out)
        print("};", file=Out)
        print("", file=Out)
    
    def dump_decls(Out):
        std_query="struct PMIU_cmd *pmi_query, int version, bool is_static"
        std_response="struct PMIU_cmd *pmi_query, struct PMIU_cmd *pmi_resp, bool is_static"
        std_get="struct PMIU_cmd *pmi"
        for NAME in G.cmd_list:
            name = NAME.lower()
            v_list = []
            for v in G.pmi_vers:
                if NAME in v:
                    v_list.append(v[NAME])
            v0 = v_list[0]
            decls = []
            if "query-name" in v0:
                if len(v0["query-attrs"]):
                    params = get_set_params(v0["query-attrs"])
                    decls.append("void PMIU_msg_set_query_%s(%s, %s);" % (name, std_query, params))
                    params = get_get_params(v0["query-attrs"])
                    decls.append("int PMIU_msg_get_query_%s(%s, %s);" % (name, std_get, params))
            if "response-name" in v0:
                if len(v0["response-attrs"]):
                    params = get_set_params(v0["response-attrs"])
                    decls.append("int PMIU_msg_set_response_%s(%s, %s);" % (name, std_response, params))
                    params = get_get_params(v0["response-attrs"])
                    decls.append("int PMIU_msg_get_response_%s(%s, %s);" % (name, std_get, params))
            if len(decls):
                print("/* PMIU_CMD_%s */" % NAME, file=Out)
                for l in decls:
                    print(l, file=Out)

    def dump_cmd_to_id(Out):
        print("int PMIU_msg_cmd_to_id(const char *cmd)", file=Out)
        print("{", file=Out)
        t_if = "    if"
        for NAME in G.cmd_list:
            cmp_list = []
            prev = {}
            for v in G.pmi_vers:
                if NAME in v and "query-name" in v[NAME]:
                    t = v[NAME]["query-name"]
                    if t not in prev:
                        cmp_list.append("strcmp(cmd, \"%s\") == 0" % t)
                        prev[t] = 1
            if len(cmp_list):
                print(t_if + " (" + ' || '.join(cmp_list) + ") {", file=Out)
                print("        return PMIU_CMD_%s;" % NAME, file=Out)
                t_if = "    } else if"
        print("    } else {", file=Out)
        print("        return PMIU_CMD_INVALID;", file=Out)
        print("    }", file=Out)
        print("}", file=Out)

    def dump_id_to_str(Out, query):
        namekey = query + "-name"
        print("const char *PMIU_msg_id_to_%s(int version, int cmd_id)" % query, file=Out)
        print("{", file=Out)
        print("    switch(cmd_id) {", file=Out)
        for NAME in G.cmd_list:
            cmp_list = []
            prev = {}
            for v in G.pmi_vers:
                if NAME in v and namekey in v[NAME]:
                    t = v[NAME][namekey]
                    if t not in prev:
                        cmp_list.append(t)
                        prev[t] = 1
            if len(cmp_list) > 0:
                print("        case PMIU_CMD_%s:" % NAME, file=Out)
                if len(cmp_list) == 1:
                    print("            return \"%s\";" % cmp_list[0], file=Out)
                else:
                    print("            return (version == PMIU_WIRE_V1) ? \"%s\" : \"%s\";" % (cmp_list[0], cmp_list[1]), file=Out)

        print("        default:", file=Out)
        print("            return NULL;", file=Out)
        print("    }", file=Out)
        print("}", file=Out)

    def dump_id_to_response(Out):
        print("const char *PMIU_msg_id_to_response(int version, int cmd_id)", file=Out)
        print("{", file=Out)
        print("    switch(cmd_id) {", file=Out)
        print("        default:", file=Out)
        print("            return NULL;", file=Out)
        print("    }", file=Out)
        print("}", file=Out)

    def dump_funcs(Out):
        std_query="struct PMIU_cmd *pmi_query, int version, bool is_static"
        std_response="struct PMIU_cmd *pmi_query, struct PMIU_cmd *pmi_resp, bool is_static"
        std_get="struct PMIU_cmd *pmi"

        def dump_if_version(t_if, version, is_set, is_query):
            if re.match(r"1\.", version):
                ver = "PMIU_WIRE_V1"
            else:
                ver = "PMIU_WIRE_V2"
            if is_set:
                if is_query:
                    print(t_if + " (version == %s) {" % ver, file=Out)
                else:
                    print(t_if + " (pmi_query->version == %s) {" % ver, file=Out)
            else:
                print(t_if + " (pmi->version == %s) {" % ver, file=Out)

        def dump_attrs(spaces, is_set, is_query, attrs, attrs0):
            non_optional = 0
            for i in range(len(attrs)):
                a = attrs[i]
                var = get_var(attrs0[i][0])
                if is_query:
                    pmi = "pmi_query"
                else:
                    pmi = "pmi_resp"

                if a[1] == "INTEGER":
                    kind = "int"
                elif a[1] == "STRING":
                    kind = "str"
                elif a[1] == "BOOLEAN":
                    kind = "bool"
                else:
                    raise Exception("Unhandled kind: " + a[1])

                if is_set:
                    pmiu = "PMIU_cmd_add_" + kind
                    print(spaces + "%s(%s, \"%s\", %s);" % (pmiu, pmi, a[0], var), file=Out)
                else:
                    if RE.match(r'.*optional=(\S+)', a[2]):
                        dflt = RE.m.group(1)
                        pmiu = "PMIU_CMD_GET_%sVAL_WITH_DEFAULT" % kind.upper()
                        print(spaces + "%s(pmi, \"%s\", *%s, %s);" % (pmiu, a[0], var, dflt), file=Out)
                    else:
                        pmiu = "PMIU_CMD_GET_%sVAL" % kind.upper()
                        print(spaces + "%s(pmi, \"%s\", *%s);" % (pmiu, a[0], var), file=Out)
                        non_optional += 1
            return non_optional

        def dump_it(NAME, v_list, is_set, is_query, attrs):
            print("", file=Out)
            ret_errno = True
            if is_set:
                params = get_set_params(attrs)
                if is_query:
                    ret_errno = False
                    print("void PMIU_msg_set_query_%s(%s, %s)" % (name, std_query, params), file=Out)
                else:
                    print("int PMIU_msg_set_response_%s(%s, %s)" % (name, std_response, params), file=Out)
            else:
                params = get_get_params(attrs)
                if is_query:
                    print("int PMIU_msg_get_query_%s(%s, %s)" % (name, std_get, params), file=Out)
                else:
                    print("int PMIU_msg_get_response_%s(%s, %s)" % (name, std_get, params), file=Out)
            print("{", file=Out)
            if ret_errno:
                print("    int pmi_errno = PMIU_SUCCESS;", file=Out)
                print("", file=Out)

            if is_set:
                if is_query:
                    print("    PMIU_msg_set_query(pmi_query, version, PMIU_CMD_%s, is_static);" % NAME, file=Out)
                else:
                    print("    PMIU_Assert(pmi_query->cmd_id == PMIU_CMD_%s);" % NAME, file=Out)
                    print("    pmi_errno = PMIU_msg_set_response(pmi_query, pmi_resp, is_static);", file=Out)
            attrs_b = None
            if len(v_list) > 1:
                if is_query:
                    attrs_b = v_list[1]["query-attrs"]
                else:
                    attrs_b = v_list[1]["response-attrs"]

            non_optional = 0
            if attrs_b is None or attrs_identical(attrs, attrs_b):
                non_optional += dump_attrs("    ", is_set, is_query, attrs, attrs)
            else:
                dump_if_version("    if", v_list[0]["version"], is_set, is_query)
                non_optional += dump_attrs("        ", is_set, is_query, attrs, attrs)
                dump_if_version("    } else if", v_list[1]["version"], is_set, is_query)
                non_optional += dump_attrs("        ", is_set, is_query, attrs_b, attrs)
                if ret_errno:
                    print("    } else {", file=Out)
                    print("        PMIU_ERR_SETANDJUMP(pmi_errno, PMIU_FAIL, \"invalid version\");", file=Out)
                    non_optional += 1
                print("    }", file=Out)

            if non_optional > 0:
                print("", file=Out)
                print("  fn_exit:", file=Out)
                print("    return pmi_errno;", file=Out)
                print("  fn_fail:", file=Out)
                print("    goto fn_exit;", file=Out)
            elif ret_errno:
                print("", file=Out)
                print("    return pmi_errno;", file=Out)
            print("}", file=Out)


        for NAME in G.cmd_list:
            name = NAME.lower()
            v_list = []
            for v in G.pmi_vers:
                if NAME in v:
                    v_list.append(v[NAME])
            v0 = v_list[0]
            if "query-name" in v0:
                if len(v0["query-attrs"]):
                    dump_it(NAME, v_list, True, True, v0["query-attrs"])
                    dump_it(NAME, v_list, False, True, v0["query-attrs"])
            if "response-name" in v0:
                if len(v0["response-attrs"]):
                    dump_it(NAME, v_list, True, False, v0["response-attrs"])
                    dump_it(NAME, v_list, False, False, v0["response-attrs"])

    # ----------------------
    msg_h = "src/pmi_msg.h"
    msg_c = "src/pmi_msg.c"
    with open(msg_h, "w") as Out:
        dump_copyright(Out)
        INC = get_include_guard(msg_h)
        print("#ifndef %s" % INC, file=Out)
        print("#define %s" % INC, file=Out)
        print("", file=Out)
        dump_enums(Out)
        print("", file=Out)
        dump_decls(Out)
        print("", file=Out)
        print("#endif /* %s */" % INC, file=Out)
    with open(msg_c, "w") as Out:
        dump_copyright(Out)
        for inc in ["pmi_config", "mpl", "pmi_util", "pmi_common", "pmi_wire", "pmi_msg"]:
            print("#include \"%s.h\"\n" % inc, file=Out)
        dump_cmd_to_id(Out)
        print("", file=Out)
        dump_id_to_str(Out, "query")
        print("", file=Out)
        dump_id_to_str(Out, "response")
        print("", file=Out)
        dump_funcs(Out)

#---- utils ------------------------------------ 
def get_set_params(attrs):
    tlist = []
    for a in attrs:
        if len(a) == 3:
            # name, kind, tail
            tlist.append(get_kind(a[1]) + get_var(a[0]))
    return ', '.join(tlist)

def get_get_params(attrs):
    tlist = []
    for a in attrs:
        if len(a) == 3:
            # name, kind, tail
            tlist.append(get_kind(a[1]) + '*' + get_var(a[0]))
    return ', '.join(tlist)

def get_var(name):
    return name.replace("-", "_")

def get_kind(kind):
    if kind == "INTEGER":
        return "int "
    elif kind == "STRING":
        return "const char *"
    elif kind == "BOOLEAN":
        return "bool "
    else:
        raise Exception("unexpected kind " + kind)

def attrs_identical(attrs_a, attrs_b):
    if len(attrs_a) != len(attrs_b):
        return False
    for i in range(len(attrs_a)):
        a = attrs_a[i]
        b = attrs_b[i]
        if a[0] != b[0] or a[1] != b[1] or a[2] != b[2]:
            return False
    return True

# ---- dump utils -----------------------------------------
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

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
