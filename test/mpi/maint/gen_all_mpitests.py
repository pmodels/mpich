##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import os
import re

class RE:
    m = None
    def match(pat, str, flags=0):
        RE.m = re.match(pat, str, flags)
        return RE.m

alltests = []
alltests_attrs = {}

copyright_mk = [
        "##",
        "## Copyright (C) by Argonne National Laboratory",
        "##     See COPYRIGHT in top-level directory",
        "##",
        "## This file is generated. Do not edit.",
        "##"
    ]

copyright_c = [
        "/*",
        " * Copyright (C) by Argonne National Laboratory",
        " *     See COPYRIGHT in top-level directory",
        " */",
        ""
        "/* This file is generated. Do not edit. */",
        ""
    ]

def main():
    In = open("maint/all_mpitests.txt", "r")
    for line in In:
        if RE.match(r'^\s*#', line):
            pass
        elif RE.match(r'(\S+)(.*)', line):
            name, tail = RE.m.group(1,2)
            alltests.append(name)
            if RE.match(r'\s*(\S.*\S)', tail):
                key = name + "-LDADD"
                alltests_attrs[key] = RE.m.group(1)

    out = []
    out.append("#include \"mpitest.h\"")
    out.append("")
    out.append("#ifdef MULTI_TESTS")
    out.append("")
    for a in alltests:
        fn_name = re.sub('/', '_', a)
        out.append("int %s(const char *args);" % fn_name)
    out.append("")
    out.append("struct mpitest alltests[] = {")
    for a in alltests:
        fn_name = re.sub('/', '_', a)
        out.append("    {\"%s\", %s}," % (a, fn_name))
    out.append("    {NULL, NULL}")
    out.append("};")
    out.append("")
    out.append("#endif /* MULTI_TESTS */")

    f = "all_mpitests.c"
    print(" -> [%s]" % f)
    with open(f, "w") as Out:
        for l in copyright_c:
            print(l, file=Out)
        for l in out:
            print(l, file=Out)

    out = []
    out.append("noinst_PROGRAMS = run_mpitests")
    out.append("run_mpitests_CPPFLAGS = $(AM_CPPFLAGS) -DMULTI_TESTS")
    out.append("run_mpitests_SOURCES = \\")
    out.append("    util/run_mpitests.c \\")
    for a in alltests:
        out.append("    %s.c \\" % a)
    out.append("    all_mpitests.c")
    ldadd = collect_LDADD()
    if ldadd:
        out.append("")
        out.append("run_mpitests_LDADD = $(LDADD) " + ldadd)

    f = "Makefile_mpitests.mtest"
    print(" -> [%s]" % f)
    with open(f, "w") as Out:
        for l in copyright_mk:
            print(l, file=Out)
        for l in out:
            print(l, file=Out)

    # Individual Makefile.am in sub test folder include individual Makefile_mpitests.mk
    # to provide per-test LDADD.
    ldadd_incs = {}
    for a in alltests:
        m = re.match(r'(.*)\/(.*)', a)
        if m:
            testdir, name = m.group(1,2)
            if not testdir in ldadd_incs:
                ldadd_incs[testdir] = []
            ldadd = "$(LDADD) $(run_mpitests_obj)"
            if a + '-LDADD' in alltests_attrs:
                ldadd += ' ' + alltests_attrs[a + '-LDADD']
            ldadd_incs[testdir].append("%s_LDADD = %s" % (name, ldadd))

    for testdir in ldadd_incs:
        f = "%s/Makefile_mpitests.mk" % testdir
        print(" -> [%s]" % f)
        with open(f, "w") as Out:
            for l in copyright_mk:
                print(l, file=Out)
            for l in ldadd_incs[testdir]:
                print(l, file=Out)

def collect_LDADD():
    ldadd_list = []
    ldadd_hash = {}
    for a in alltests_attrs:
        if re.match(r'.*-LDADD', a):
            for b in alltests_attrs[a].split():
                if not b in ldadd_hash:
                    ldadd_hash[b] = 1
                    ldadd_list.append(b)

    return ' '.join(ldadd_list)

#---------------------------------------- 
if __name__ == "__main__":
    main()
