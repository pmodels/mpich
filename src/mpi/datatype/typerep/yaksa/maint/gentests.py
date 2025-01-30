#! /usr/bin/env python3
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import sys
import os

skip_test_complex = False
for a in sys.argv[1:]:
    if a == "-skip-test-complex":
        skip_test_complex = True

##### global settings
counts = [ 17, 1075, 65536 ]
iters = {
    17: 32768,
    1075: 128,
    65536: 32,
}
types = [ "int", "short_int", "int:3+float:2", "int:3+double:2"]
if not skip_test_complex:
    types.extend(["c_complex", "c_double_complex"])

oplist = {
    "int": "int",
    "short_int": "int",
    "int:3+float:2": "float",
    "int:3+double:2": "float",
    "c_complex": "complex",
    "c_double_complex": "complex"}
seed = 1

##### simple tests generator
def gen_simple_tests(testlist):
    global seed

    prefix = os.path.dirname(testlist)

    try:
        outfile = open(testlist, "w")
    except:
        sys.stderr.write("error creating testlist %s\n" % testlist)
        sys.exit()

    sys.stdout.write("generating simple tests ... ")
    outfile.write(os.path.join(prefix, "simple_test") + "\n")
    outfile.write(os.path.join(prefix, "threaded_test") + "\n")
    outfile.write(os.path.join(prefix, "lbub") + "\n")
    outfile.write(os.path.join(prefix, "test_contig") + "\n")
    outfile.close()
    sys.stdout.write("done\n")

##### pack/iov tests generator
def gen_pack_iov_tests(fn, testlist, add_ops, extra_args = ""):
    global seed

    segments = [ 1, 64 ]
    orderings = [ "normal", "reverse", "random" ]
    overlaps = [ "none", "regular", "irregular" ]

    prefix = os.path.dirname(testlist)

    try:
        outfile = open(testlist, "w")
    except:
        sys.stderr.write("error creating testlist %s\n" % testlist)
        sys.exit()

    if (extra_args == ""):
        sys.stdout.write("generating %s tests ... " % fn)
    else:
        sys.stdout.write("generating %s (%s) tests ... " % (fn, extra_args))
    for overlap in overlaps:
        for ordering in orderings:
            if (overlap != "none" and ordering != "normal"):
                continue

            for segment in segments:
                if (segment == 1 and (ordering != "normal" or overlap != "none")):
                    continue

                outfile.write("# %s (segments %d, pack order %s, overlap %s)\n" % \
                              (fn, segment, ordering, overlap))
                for count in counts:
                    for t in types:
                        outstr = os.path.join(prefix, fn)
                        outstr += " -datatype %s" % t
                        outstr += " -count %d" % count
                        outstr += " -seed %d" % seed
                        seed = seed + 1
                        outstr += " -iters %d" % iters[count]
                        outstr += " -segments %d" % segment
                        outstr += " -ordering %s" % ordering
                        outstr += " -overlap %s" % overlap
                        if (add_ops == True):
                            outstr += " -oplist %s" % oplist[t]
                        if (extra_args != ""):
                            outstr += extra_args
                        outfile.write(outstr + "\n")
                outfile.write("\n")
    outfile.close()
    sys.stdout.write("done\n")


##### flatten tests generator
def gen_flatten_tests(testlist, extra_args = ""):
    global seed

    prefix = os.path.dirname(testlist)

    try:
        outfile = open(testlist, "w")
    except:
        sys.stderr.write("error creating testlist %s\n" % testlist)
        sys.exit()

    if (extra_args == ""):
        sys.stdout.write("generating flatten tests ... ")
    else:
        sys.stdout.write("generating flatten (%s) tests ... " % extra_args)
    for count in counts:
        for t in types:
            outstr = os.path.join(prefix, "flatten")
            outstr += " -datatype %s" % t
            outstr += " -count %d" % count
            outstr += " -seed %d" % seed
            seed = seed + 1
            outstr += " -iters %d" % iters[count]
            if (extra_args != ""):
                outstr += extra_args
            outfile.write(outstr + "\n")
    outfile.write("\n")
    outfile.close()
    sys.stdout.write("done\n")


##### main function
if __name__ == '__main__':
    gen_simple_tests("test/simple/testlist.gen")

    gen_pack_iov_tests("pack", "test/pack/testlist.gen", True)
    gen_pack_iov_tests("pack", "test/pack/testlist.threads.gen", True, \
                       " -num-threads 4")
    gen_pack_iov_tests("pack", "test/pack/testlist.blocking.gen", True, \
                       " -blocking")
    gen_pack_iov_tests("pack", "test/pack/testlist.stream.gen", True, \
                       " -stream")

    gen_pack_iov_tests("iov", "test/iov/testlist.gen", False)
    gen_pack_iov_tests("iov", "test/iov/testlist.threads.gen", False, " -num-threads 4")
    gen_flatten_tests("test/flatten/testlist.gen")
    gen_flatten_tests("test/flatten/testlist.threads.gen", " -num-threads 4")
