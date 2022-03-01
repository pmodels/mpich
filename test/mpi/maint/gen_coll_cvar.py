##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import os
import re

script_dir = os.path.dirname(__file__)

# RE class allows convenience of using regex capture in a condition
class RE:
    m = None
    def match(pat, str, flags=0):
        RE.m = re.match(pat, str, flags)
        return RE.m
    def search(pat, str, flags=0):
        RE.m = re.search(pat, str, flags)
        return RE.m

def main():
    (tests, algos, algo_params) = load_config("coll_cvars.txt")
    testlist_cvar = "coll/testlist.collalgo"
    print("  --> [%s]" % testlist_cvar)
    with open(testlist_cvar, "w") as Out:
        for (key, testlist) in tests.items():
            if RE.match(r'(\w+)-(persistent)', key):
                algo_key = "%s-intra-nonblocking" % RE.m.group(1)
                dump_tests(Out, algo_key, testlist, algos, algo_params)
            elif RE.match(r'(\w+)-(\w+)-(\w+)', key):
                (name, intra, blocking) = RE.m.group(1,2,3)
                dump_tests(Out, key, testlist, algos, algo_params)
                if blocking == "blocking":
                    algo_key = "%s-%s-nonblocking" % (name, intra)
                    dump_tests(Out, algo_key, testlist, algos, algo_params, "nb")

def dump_tests(Out, key, testlist, algos, algo_params, special=None):
    if key not in algos:
        return
    if RE.match(r'(\w+)-(\w+)-(\w+)', key):
        (name, intra, blocking) = RE.m.group(1,2,3)
        NAME = name.upper()
        INTRA = intra.upper()
        if blocking == "nonblocking" and not special:
            NAME = 'I' + NAME
        for test in testlist:
            for algo in algos[key]:
                segs = [test]
                if special == "nb":
                    segs.append("env=MPIR_CVAR_%s_DEVICE_COLLECTIVE=0" % NAME)
                    segs.append("env=MPIR_CVAR_%s_%s_ALGORITHM=nb" % (NAME, INTRA))
                    segs.append("env=MPIR_CVAR_I%s_DEVICE_COLLECTIVE=0" % NAME)
                    segs.append("env=MPIR_CVAR_I%s_%s_ALGORITHM=%s" % (NAME, INTRA, algo))
                elif RE.match(r'composition:(\w+)', algo):
                    segs.append("env=MPIR_CVAR_%s_COMPOSITION=%s" % (NAME, RE.m.group(1)))
                elif RE.match(r'(\w+):(\w+)', algo):
                    DEVICE = RE.m.group(1).upper()
                    segs.append("env=MPIR_CVAR_%s_%s_%s_ALGORITHM=%s" % (NAME, DEVICE, INTRA, RE.m.group(2)))
                else:
                    segs.append("env=MPIR_CVAR_%s_DEVICE_COLLECTIVE=0" % NAME)
                    segs.append("env=MPIR_CVAR_%s_%s_ALGORITHM=%s" % (NAME, INTRA, algo))
                algo_key = key + "-" + algo
                if algo_key in algo_params:
                    count = count_algo_params(algo_params[algo_key], test)
                    for i in range(count):
                        s = get_algo_params(algo_params[algo_key], i)
                        print(' '.join(segs) + s, file=Out)
                else:
                    print(' '.join(segs), file=Out)

def count_algo_params(params, test):
    count = 1
    for t in params:
        if RE.match(r'(MPIR_CVAR_\w+)=(.*)', t):
            tlist = RE.m.group(2).split(',')
            count *= len(tlist)
    return count

def get_algo_params(params, i):
    s = ''
    for t in params:
        if RE.match(r'(MPIR_CVAR_\w+)=(.*)', t):
            name = RE.m.group(1)
            tlist = RE.m.group(2).split(',')
            count = len(tlist)
            idx = i % count
            i //= count
            s += ' env=%s=%s' % (name, tlist[idx])
    return s

def load_config(config_file):
    tests = {}
    algos = {}
    algo_params = {}
    
    sets = None
    name = ""
    key = ""
    algo_key = ""
    with open(script_dir + "/" + config_file, "r") as In:
        for line in In:
            # INDENT Level 0
            if re.match(r'tests:', line):
                sets = tests
            elif re.match(r'algorithms:', line):
                sets = algos
            # INDENT Level 1 (4 spaces)
            elif RE.match(r'^    (\w+):', line):
                name = RE.m.group(1)
            elif RE.match(r'^\s+((intra|inter)-(non)?blocking|persistent):', line):
                key = name + "-" + RE.m.group(1)
                sets[key] = []
            # tests
            elif RE.match(r'^\s+(\w+\s+\d+.*)', line):
                # test
                sets[key].append(RE.m.group(1))
            # algorithm
            elif RE.match(r'^\s+((\w+:)?\w+)', line):
                sets[key].append(RE.m.group(1))
                algo_key = "%s-%s" % (key, RE.m.group(1))
            # algo params
            elif RE.match(r'^\s+\.(MPIR_CVAR_\w+=.+|[a-z-]+)', line):
                if algo_key not in algo_params:
                    algo_params[algo_key] = []
                algo_params[algo_key].append(RE.m.group(1))
    return (tests, algos, algo_params)

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
