#! /usr/bin/env python3
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import sys
import argparse

sys.path.append('maint/')
import yutils

sys.path.append('src/backend/')
import gencomm


########################################################################################
##### Global variables
########################################################################################

num_paren_open = 0
builtin_types = [ "_Bool", "char", "wchar_t", "int8_t", "int16_t", \
                  "int32_t", "int64_t", "float", "double", "long double", "c_complex", "c_double_complex", "c_long_double_complex" ]
blklens = [ "1", "2", "3", "4", "5", "6", "7", "8", "generic" ]
builtin_maps = { }


########################################################################################
##### Type-specific functions
########################################################################################

## hvector routines
def hvector_decl(nesting, dtp, b):
    yutils.display(OUTFILE, "intptr_t count%d = %s->u.hvector.count;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "intptr_t blocklength%d ATTRIBUTE((unused)) = %s->u.hvector.blocklength;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "intptr_t stride%d = %s->u.hvector.stride;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "uintptr_t extent%d ATTRIBUTE((unused)) = %s->extent;\n" % (nesting, dtp))

def hvector(suffix, b, blklen, last):
    global num_paren_open
    num_paren_open += 2
    yutils.display(OUTFILE, "for (intptr_t j%d = 0; j%d < count%d; j%d++) {\n" % (suffix, suffix, suffix, suffix))
    if (blklen == "generic"):
        yutils.display(OUTFILE, "for (intptr_t k%d = 0; k%d < blocklength%d; k%d++) {\n" % (suffix, suffix, suffix, suffix))
    else:
        yutils.display(OUTFILE, "for (intptr_t k%d = 0; k%d < %s; k%d++) {\n" % (suffix, suffix, blklen, suffix))
    global s
    if (last != 1):
        s += " + j%d * stride%d + k%d * extent%d" % (suffix, suffix, suffix, suffix + 1)
    else:
        s += " + j%d * stride%d + k%d * sizeof(%s)" % (suffix, suffix, suffix, b)

## blkhindx routines
def blkhindx_decl(nesting, dtp, b):
    yutils.display(OUTFILE, "intptr_t count%d = %s->u.blkhindx.count;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "intptr_t blocklength%d ATTRIBUTE((unused)) = %s->u.blkhindx.blocklength;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "intptr_t *restrict array_of_displs%d = %s->u.blkhindx.array_of_displs;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "uintptr_t extent%d ATTRIBUTE((unused)) = %s->extent;\n" % (nesting, dtp))

def blkhindx(suffix, b, blklen, last):
    global num_paren_open
    num_paren_open += 2
    yutils.display(OUTFILE, "for (intptr_t j%d = 0; j%d < count%d; j%d++) {\n" % (suffix, suffix, suffix, suffix))
    if (blklen == "generic"):
        yutils.display(OUTFILE, "for (intptr_t k%d = 0; k%d < blocklength%d; k%d++) {\n" % (suffix, suffix, suffix, suffix))
    else:
        yutils.display(OUTFILE, "for (intptr_t k%d = 0; k%d < %s; k%d++) {\n" % (suffix, suffix, blklen, suffix))
    global s
    if (last != 1):
        s += " + array_of_displs%d[j%d] + k%d * extent%d" % \
             (suffix, suffix, suffix, suffix + 1)
    else:
        s += " + array_of_displs%d[j%d] + k%d * sizeof(%s)" % (suffix, suffix, suffix, b)

## hindexed routines
def hindexed_decl(nesting, dtp, b):
    yutils.display(OUTFILE, "intptr_t count%d = %s->u.hindexed.count;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "intptr_t *restrict array_of_blocklengths%d = %s->u.hindexed.array_of_blocklengths;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "intptr_t *restrict array_of_displs%d = %s->u.hindexed.array_of_displs;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "uintptr_t extent%d ATTRIBUTE((unused)) = %s->extent;\n" % (nesting, dtp))

def hindexed(suffix, b, blklen, last):
    global num_paren_open
    num_paren_open += 2
    yutils.display(OUTFILE, "for (intptr_t j%d = 0; j%d < count%d; j%d++) {\n" % (suffix, suffix, suffix, suffix))
    yutils.display(OUTFILE, "for (intptr_t k%d = 0; k%d < array_of_blocklengths%d[j%d]; k%d++) {\n" % \
            (suffix, suffix, suffix, suffix, suffix))
    global s
    if (last != 1):
        s += " + array_of_displs%d[j%d] + k%d * extent%d" % \
             (suffix, suffix, suffix, suffix + 1)
    else:
        s += " + array_of_displs%d[j%d] + k%d * sizeof(%s)" % (suffix, suffix, suffix, b)

## contig routines
def contig_decl(nesting, dtp, b):
    yutils.display(OUTFILE, "intptr_t count%d = %s->u.contig.count;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "intptr_t stride%d = %s->u.contig.child->extent;\n" % (nesting, dtp))
    yutils.display(OUTFILE, "uintptr_t extent%d ATTRIBUTE((unused)) = %s->extent;\n" % (nesting, dtp))

def contig(suffix, b, blklen, last):
    global num_paren_open
    num_paren_open += 1
    yutils.display(OUTFILE, "for (intptr_t j%d = 0; j%d < count%d; j%d++) {\n" % (suffix, suffix, suffix, suffix))
    global s
    s += " + j%d * stride%d" % (suffix, suffix)

# resized routines
def resized_decl(nesting, dtp, b):
    yutils.display(OUTFILE, "uintptr_t extent%d ATTRIBUTE((unused)) = %s->extent;\n" % (nesting, dtp))

def resized(suffix, b, blklen, last):
    pass


########################################################################################
##### Core kernels
########################################################################################
def generate_kernels(b, darray, blklen):
    global num_paren_open
    global s

    # individual blocklength optimization is only for
    # hvector and blkhindx
    if (len(darray) and darray[-1] != "hvector" and darray[-1] != "blkhindx" and blklen != "generic"):
        return

    for func in "pack","unpack":
        ##### figure out the function name to use
        s = "int yaksuri_seqi_%s_" % func
        for d in darray:
            s = s + "%s_" % d
        # hvector and hindexed get blklen-specific function names
        if (len(darray) and (darray[-1] == "hvector" or darray[-1] == "blkhindx")):
            s = s + "blklen_%s_" % blklen + b.replace(" ", "_")
        else:
            s = s + b.replace(" ", "_")
        yutils.display(OUTFILE, "%s(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type, yaksa_op_t op)\n" % s),
        yutils.display(OUTFILE, "{\n")


        ##### variable declarations
        # generic variables
        yutils.display(OUTFILE, "int rc = YAKSA_SUCCESS;\n");
        yutils.display(OUTFILE, "const char *restrict sbuf = (const char *) inbuf;\n");
        yutils.display(OUTFILE, "char *restrict dbuf = (char *) outbuf;\n");
        yutils.display(OUTFILE, "uintptr_t extent ATTRIBUTE((unused)) = type->extent;\n")
        yutils.display(OUTFILE, "\n");

        # variables specific to each nesting level
        s = "type"
        for x in range(len(darray)):
            getattr(sys.modules[__name__], "%s_decl" % darray[x])(x + 1, s, b)
            yutils.display(OUTFILE, "\n")
            s = s + "->u.%s.child" % darray[x]

        yutils.display(OUTFILE, "uintptr_t idx = 0;\n")

        ##### non-hvector and non-blkhindx
        yutils.display(OUTFILE, "switch (op) {\n")
        for op in gencomm.type_ops[b]:
            yutils.display(OUTFILE, "case YAKSA_OP__%s:\n" % op)
            yutils.display(OUTFILE, "{\n")

            yutils.display(OUTFILE, "for (intptr_t i = 0; i < count; i++) {\n")
            num_paren_open += 1
            s = "i * extent"
            for x in range(len(darray)):
                if (x != len(darray) - 1):
                    getattr(sys.modules[__name__], darray[x])(x + 1, b, "generic", 0)
                else:
                    getattr(sys.modules[__name__], darray[x])(x + 1, b, blklen, 1)

            type = b
            if (b == "c_complex"):
                b = "float _Complex"
            elif (b == "c_double_complex"):
                b = "double _Complex"
            elif (b == "c_long_double_complex"):
                b = "long double _Complex"

            if (func == "pack"):
                if ((b == "float" or b == "double" or b == "long double") and
                    (op == "MAX" or op == "MIN")):
                    yutils.display(OUTFILE, "YAKSURI_SEQI_OP_%s_FLOAT(%s, *((const %s *) (const void *) (sbuf + %s)), *((%s *) (void *) (dbuf + idx)));\n" % (op, b, b, s, b))
                else:
                    yutils.display(OUTFILE, "YAKSURI_SEQI_OP_%s(*((const %s *) (const void *) (sbuf + %s)), *((%s *) (void *) (dbuf + idx)));\n" % (op, b, s.replace(type,b), b))
            else:
                if ((b == "float" or b == "double" or b == "long double") and
                    (op == "MAX" or op == "MIN")):
                    yutils.display(OUTFILE, "YAKSURI_SEQI_OP_%s_FLOAT(%s, *((const %s *) (const void *) (sbuf + idx)), *((%s *) (void *) (dbuf + %s)));\n" % (op, b, b, b, s))
                else:
                    yutils.display(OUTFILE, "YAKSURI_SEQI_OP_%s(*((const %s *) (const void *) (sbuf + idx)), *((%s *) (void *) (dbuf + %s)));\n" % (op, b, b, s.replace(type,b)))

            yutils.display(OUTFILE, "idx += sizeof(%s);\n" % b)
            b = type
            for x in range(num_paren_open):
                yutils.display(OUTFILE, "}\n")
            num_paren_open = 0

            yutils.display(OUTFILE, "break;\n")
            yutils.display(OUTFILE, "}\n")
            yutils.display(OUTFILE, "\n")

        yutils.display(OUTFILE, "default:\n")
        yutils.display(OUTFILE, "    break;\n")
        yutils.display(OUTFILE, "}\n")

        yutils.display(OUTFILE, "\n");
        yutils.display(OUTFILE, "return rc;\n")
        yutils.display(OUTFILE, "}\n\n")


########################################################################################
##### main function
########################################################################################
if __name__ == '__main__':
    ##### parse user arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('--pup-max-nesting', type=int, default=3, help='maximum nesting levels to generate')
    args = parser.parse_args()
    if (args.pup_max_nesting < 0):
        parser.print_help()
        print
        print("===> ERROR: pup-max-nesting must be positive")
        sys.exit(1)

    ##### generate the reduction kernels for contiguous types
    for b in builtin_types:
        filename = "src/backend/seq/pup/yaksuri_seqi_pup_%s.c" % b.replace(" ","_")
        yutils.copyright_c(filename)
        OUTFILE = open(filename, "a")
        yutils.display(OUTFILE, "#include <string.h>\n")
        yutils.display(OUTFILE, "#include <stdint.h>\n")
        yutils.display(OUTFILE, "#include <wchar.h>\n")
        yutils.display(OUTFILE, "#include \"yaksuri_seqi_pup.h\"\n")
        yutils.display(OUTFILE, "\n")

        emptylist = [ ]
        generate_kernels(b, emptylist, 0)

        OUTFILE.close()

    ##### generate the core pack/unpack kernels (single level)
    if args.pup_max_nesting > 0:
        for b in builtin_types:
            for d in gencomm.derived_types:
                filename = "src/backend/seq/pup/yaksuri_seqi_pup_%s_%s.c" % (d, b.replace(" ","_"))
                yutils.copyright_c(filename)
                OUTFILE = open(filename, "a")
                yutils.display(OUTFILE, "#include <string.h>\n")
                yutils.display(OUTFILE, "#include <stdint.h>\n")
                yutils.display(OUTFILE, "#include <wchar.h>\n")
                yutils.display(OUTFILE, "#include \"yaksuri_seqi_pup.h\"\n")
                yutils.display(OUTFILE, "\n")

                emptylist = [ ]
                emptylist.append(d)
                for blklen in blklens:
                    generate_kernels(b, emptylist, blklen)
                emptylist.pop()

                OUTFILE.close()

    ##### generate the core pack/unpack kernels (more than one level)
    if args.pup_max_nesting > 1:
        darraylist = [ ]
        yutils.generate_darrays(gencomm.derived_types, darraylist, args.pup_max_nesting - 2)
        for b in builtin_types:
            for d1 in gencomm.derived_types:
                for d2 in gencomm.derived_types:
                    filename = "src/backend/seq/pup/yaksuri_seqi_pup_%s_%s_%s.c" % (d1, d2, b.replace(" ","_"))
                    yutils.copyright_c(filename)
                    OUTFILE = open(filename, "a")
                    yutils.display(OUTFILE, "#include <string.h>\n")
                    yutils.display(OUTFILE, "#include <stdint.h>\n")
                    yutils.display(OUTFILE, "#include <wchar.h>\n")
                    yutils.display(OUTFILE, "#include \"yaksuri_seqi_pup.h\"\n")
                    yutils.display(OUTFILE, "\n")

                    for darray in darraylist:
                        darray.append(d1)
                        darray.append(d2)
                        for blklen in blklens:
                            generate_kernels(b, darray, blklen)
                        darray.pop()
                        darray.pop()

                    OUTFILE.close()

    ##### generate the core pack/unpack kernel declarations
    filename = "src/backend/seq/pup/yaksuri_seqi_pup.h"
    yutils.copyright_c(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "#ifndef YAKSURI_SEQI_PUP_H_INCLUDED\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_PUP_H_INCLUDED\n")
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "#include <string.h>\n")
    yutils.display(OUTFILE, "#include <stdint.h>\n")
    yutils.display(OUTFILE, "#include \"yaksi.h\"\n")
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_MAX(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) = ((in) ^ (((in) ^ (out)) & -((in) < (out)))); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_MIN(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) = ((out) ^ (((in) ^ (out)) & -((in) < (out)))); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_MAX_FLOAT(type,in,out) \\\n")
    yutils.display(OUTFILE, "    do { type x_[2] = { (in), (out) }; (out) = x_[(in) < (out)]; } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_MIN_FLOAT(type,in,out) \\\n")
    yutils.display(OUTFILE, "    do { type x_[2] = { (in), (out) }; (out) = x_[(in) > (out)]; } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_SUM(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) += (in); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_PROD(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) *= (in); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_LAND(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) = ((out) && (in)); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_BAND(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) &= (in); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_LOR(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) = ((out) || (in)); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_BOR(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) |= (in); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_LXOR(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) = (!(out) != !(in)); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_BXOR(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) ^= (in); } while (0)\n")
    yutils.display(OUTFILE, "#define YAKSURI_SEQI_OP_REPLACE(in,out) \\\n")
    yutils.display(OUTFILE, "    do { (out) = (in); } while (0)\n")
    yutils.display(OUTFILE, "\n")

    darraylist = [ ]
    yutils.generate_darrays(gencomm.derived_types, darraylist, args.pup_max_nesting)
    for b in builtin_types:
        yutils.display(OUTFILE, "int yaksuri_seqi_pack_%s(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type, yaksa_op_t op);\n" % b.replace(" ", "_"))
        yutils.display(OUTFILE, "int yaksuri_seqi_unpack_%s(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type, yaksa_op_t op);\n" % b.replace(" ", "_"))
        for darray in darraylist:
            for blklen in blklens:
                # we don't need pup kernels for basic types
                if (len(darray) == 0):
                    continue

                # individual blocklength optimization is only for
                # hvector and blkhindx
                if (darray[-1] != "hvector" and darray[-1] != "blkhindx" \
                    and blklen != "generic"):
                    continue

                for func in "pack","unpack":
                    ##### figure out the function name to use
                    s = "int yaksuri_seqi_%s_" % func
                    for d in darray:
                        s = s + "%s_" % d
                    # hvector and hindexed get blklen-specific function names
                    if (len(darray) and darray[-1] != "hvector" and darray[-1] != "blkhindx"):
                        s = s + b.replace(" ", "_")
                    else:
                        s = s + "blklen_%s_" % blklen + b.replace(" ", "_")
                    yutils.display(OUTFILE, "%s" % s),
                    yutils.display(OUTFILE, "(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type, yaksa_op_t op);\n")

    yutils.display(OUTFILE, "#endif  /* YAKSURI_SEQI_PUP_H_INCLUDED */\n")
    OUTFILE.close()

    ##### generate the pup makefile
    filename = "src/backend/seq/pup/Makefile.pup.mk"
    yutils.copyright_makefile(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "libyaksa_la_SOURCES += \\\n")
    for b in builtin_types:
        yutils.display(OUTFILE, "\tsrc/backend/seq/pup/yaksuri_seqi_pup_%s.c \\\n" % \
                       b.replace(" ","_"))
        if args.pup_max_nesting > 0:
            for d1 in gencomm.derived_types:
                yutils.display(OUTFILE, "\tsrc/backend/seq/pup/yaksuri_seqi_pup_%s_%s.c \\\n" % \
                               (d1, b.replace(" ","_")))
                if args.pup_max_nesting > 1:
                    for d2 in gencomm.derived_types:
                        yutils.display(OUTFILE, "\tsrc/backend/seq/pup/yaksuri_seqi_pup_%s_%s_%s.c \\\n" % \
                                       (d1, d2, b.replace(" ","_")))
    yutils.display(OUTFILE, "\tsrc/backend/seq/pup/yaksuri_seq_pup.c\n")
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "noinst_HEADERS += \\\n")
    yutils.display(OUTFILE, "\tsrc/backend/seq/pup/yaksuri_seqi_pup.h\n")
    OUTFILE.close()

    ##### generate the switching logic to select pup functions
    gencomm.populate_pupfns(args.pup_max_nesting, "seq", blklens, builtin_types, builtin_maps)
