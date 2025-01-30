#!/usr/bin/env python3
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import sys, os

# inline.py [infile] [outfile] [topdir] [native: 0|1|2]

infile = sys.argv[1]
outfile = sys.argv[2]
top_srcdir = sys.argv[3]
native = sys.argv[4]

base = os.path.basename(outfile)
basename = os.path.splitext(base)[0]

sys.path.append(top_srcdir + '/maint/')
import yutils

yutils.copyright_c(outfile)
OUTFILE = open(outfile, 'a')

if native == "2":
    OUTFILE.write("/* native format with multiple devices */\n\n")
elif native == "1":
    OUTFILE.write("/* native format */\n\n")
else:
    OUTFILE.write("/* SPIR-V format */\n\n")
OUTFILE.write("#include <stdlib.h>\n\n")
OUTFILE.write("const unsigned char %s_str[] = {\n" % basename)

if os.path.exists(infile):
    bfile = open(infile, 'rb')
    char = bfile.read(1)
    OUTFILE.write("%s" % hex(ord(char)))
    while 1:
        char = bfile.read(1)
        if not char:
            break
        OUTFILE.write(", %s" % hex(ord(char)))
    bfile.close()
    OUTFILE.write("};\n")
    OUTFILE.write("const size_t %s_size = %d;\n" % (basename, os.stat(infile).st_size))
else:
    OUTFILE.write("};\n")
    OUTFILE.write("const size_t %s_size = 0;\n" % basename)

OUTFILE.close()
