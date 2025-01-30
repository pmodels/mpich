#! /usr/bin/env python3
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import sys
import os
import argparse
import subprocess
from collections import deque

sys.path.append('maint/')
import yutils

sys.path.append('src/backend/')
import gencomm


########################################################################################
##### Global variables
########################################################################################

num_paren_open = 0
blklens = [ "generic" ]
builtin_types = [ "char", "int8_t", "int16_t", \
                  "int32_t", "int64_t", "float", "double", "c_complex", "c_double_complex"]

builtin_maps = {
    "YAKSA_TYPE__UNSIGNED_CHAR": "char",
    "YAKSA_TYPE__UINT8_T": "int8_t",
    "YAKSA_TYPE__UINT16_T": "int16_t",
    "YAKSA_TYPE__UINT32_T": "int32_t",
    "YAKSA_TYPE__UINT64_T": "int64_t",
    "YAKSA_TYPE__BYTE": "int8_t"
}


########################################################################################
##### Type-specific functions
########################################################################################

## hvector routines
def hvector(suffix, dtp, b, last):
    global s
    global idx
    global need_extent
    yutils.display(OUTFILE, "intptr_t stride%d = %s->u.hvector.stride;\n" % (suffix, dtp))
    if (need_extent == True):
        yutils.display(OUTFILE, "uintptr_t extent%d = %s->extent;\n" % (suffix, dtp))
    if (last != 1):
        s += " + x%d * stride%d + x%d * extent%d" % (idx, suffix, idx + 1, suffix + 1)
        need_extent = True
    else:
        s += " + x%d * stride%d + x%d * sizeof(%s)" % (idx, suffix, idx + 1, b)
        need_extent = False
    idx = idx + 2

## blkhindx routines
def blkhindx(suffix, dtp, b, last):
    global s
    global idx
    global need_extent
    yutils.display(OUTFILE, "intptr_t *array_of_displs%d = %s->u.blkhindx.array_of_displs;\n" % (suffix, dtp))
    if (need_extent == True):
        yutils.display(OUTFILE, "uintptr_t extent%d = %s->extent;\n" % (suffix, dtp))
    if (last != 1):
        s += " + array_of_displs%d[x%d] + x%d * extent%d" % \
             (suffix, idx, idx + 1, suffix + 1)
        need_extent = True
    else:
        s += " + array_of_displs%d[x%d] + x%d * sizeof(%s)" % (suffix, idx, idx + 1, b)
        need_extent = False
    idx = idx + 2

## hindexed routines
def hindexed(suffix, dtp, b, last):
    global s
    global idx
    global need_extent
    yutils.display(OUTFILE, "intptr_t *array_of_displs%d = %s->u.hindexed.array_of_displs;\n" % (suffix, dtp))
    if (need_extent == True):
        yutils.display(OUTFILE, "uintptr_t extent%d = %s->extent;\n" % (suffix, dtp))
    if (last != 1):
        s += " + array_of_displs%d[x%d] + x%d * extent%d" % \
             (suffix, idx, idx + 1, suffix + 1)
        need_extent = True
    else:
        s += " + array_of_displs%d[x%d] + x%d * sizeof(%s)" % (suffix, idx, idx + 1, b)
        need_extent = False
    idx = idx + 2

## contig routines
def contig(suffix, dtp, b, last):
    global s
    global idx
    global need_extent
    yutils.display(OUTFILE, "intptr_t stride%d = %s->u.contig.child->extent;\n" % (suffix, dtp))
    if (need_extent == True):
        yutils.display(OUTFILE, "uintptr_t extent%d = %s->extent;\n" % (suffix, dtp))
    need_extent = False
    s += " + x%d * stride%d" % (idx, suffix)
    idx = idx + 1

# resized routines
def resized(suffix, dtp, b, last):
    global need_extent
    if (need_extent == True):
        yutils.display(OUTFILE, "uintptr_t extent%d = %s->extent;\n" % (suffix, dtp))
    need_extent = False


########################################################################################
##### Core kernels
########################################################################################
def generate_kernels(b, darray, op):
    global need_extent
    global s
    global idx

    # we need pup kernels for reduction of basic types
    funclist = [ ]
    funclist.append("pack_%s" % op)
    funclist.append("unpack_%s" % op)


    for func in funclist:
        ##### figure out the function name to use
        funcprefix = "%s_" % func
        for d in darray:
            funcprefix = funcprefix + "%s_" % d
        funcprefix = funcprefix + b.replace(" ", "_")

        ##### generate the ZE kernel
        yutils.display(OUTFILE, "__kernel void yaksuri_zei_kernel_%s(__global const void *inbuf, __global void *outbuf, unsigned long count, __global const yaksuri_zei_md_s *__restrict__ md)\n" % funcprefix)
        yutils.display(OUTFILE, "{\n")
        yutils.display(OUTFILE, "__global const char *__restrict__ sbuf = (__global char *) inbuf;\n");
        yutils.display(OUTFILE, "__global char *__restrict__ dbuf = (__global char *) outbuf;\n")
        if ("unpack" in func and len(darray) != 0):
            yutils.display(OUTFILE, "dbuf = dbuf - md->true_lb;\n");
        elif (len(darray) != 0):
            yutils.display(OUTFILE, "sbuf = (__global const char *) ((__global char *)sbuf - md->true_lb);\n");
        yutils.display(OUTFILE, "uintptr_t extent = md->extent;\n")
        yutils.display(OUTFILE, "uintptr_t idx = get_global_id(0);\n")
        yutils.display(OUTFILE, "uintptr_t res = idx;\n")
        yutils.display(OUTFILE, "uintptr_t inner_elements = md->num_elements;\n")
        yutils.display(OUTFILE, "\n")

        yutils.display(OUTFILE, "if (idx >= (count * inner_elements))\n")
        yutils.display(OUTFILE, "    return;\n")
        yutils.display(OUTFILE, "\n")

        # copy loop
        idx = 0
        md = "md"
        for d in darray:
            if (d == "hvector" or d == "blkhindx" or d == "hindexed" or \
                d == "contig"):
                yutils.display(OUTFILE, "uintptr_t x%d = res / inner_elements;\n" % idx)
                idx = idx + 1
                yutils.display(OUTFILE, "res %= inner_elements;\n")
                yutils.display(OUTFILE, "inner_elements /= %s->u.%s.count;\n" % (md, d))
                yutils.display(OUTFILE, "\n")

            if (d == "hvector" or d == "blkhindx"):
                yutils.display(OUTFILE, "uintptr_t x%d = res / inner_elements;\n" % idx)
                idx = idx + 1
                yutils.display(OUTFILE, "res %= inner_elements;\n")
                yutils.display(OUTFILE, "inner_elements /= %s->u.%s.blocklength;\n" % (md, d))
            elif (d == "hindexed"):
                yutils.display(OUTFILE, "uintptr_t x%d;\n" % idx)
                yutils.display(OUTFILE, "for (intptr_t i = 0; i < %s->u.%s.count; i++) {\n" % (md, d))
                yutils.display(OUTFILE, "    uintptr_t in_elems = %s->u.%s.array_of_blocklengths[i] *\n" % (md, d))
                yutils.display(OUTFILE, "                         %s->u.%s.child->num_elements;\n" % (md, d))
                yutils.display(OUTFILE, "    if (res < in_elems) {\n")
                yutils.display(OUTFILE, "        x%d = i;\n" % idx)
                yutils.display(OUTFILE, "        res %= in_elems;\n")
                yutils.display(OUTFILE, "        inner_elements = %s->u.%s.child->num_elements;\n" % (md, d))
                yutils.display(OUTFILE, "        break;\n")
                yutils.display(OUTFILE, "    } else {\n")
                yutils.display(OUTFILE, "        res -= in_elems;\n")
                yutils.display(OUTFILE, "    }\n")
                yutils.display(OUTFILE, "}\n")
                idx = idx + 1
                yutils.display(OUTFILE, "\n")

            md = "%s->u.%s.child" % (md, d)

        yutils.display(OUTFILE, "uintptr_t x%d = res;\n" % idx)
        yutils.display(OUTFILE, "\n")

        dtp = "md"
        s = "x0 * extent"
        idx = 1
        x = 1
        need_extent = False
        for d in darray:
            if (x == len(darray)):
                last = 1
            else:
                last = 0
            getattr(sys.modules[__name__], d)(x, dtp, b, last)
            x = x + 1
            dtp = dtp + "->u.%s.child" % d

        type = b
        if (b == "c_complex"):
            type = "float2"
        elif (b == "c_double_complex"):
            type = "double2"
        elif (b == "c_long_double_complex"):
            type = "long double2"

        if (func == "pack_REPLACE"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) = *((const %s *) (const void *) (sbuf + %s));\n"
                                       % (type,type,type,s.replace(b,type)))
        elif (func == "pack_SUM"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) += *((const %s *) (const void *) (sbuf + %s));\n"
                                       % (type,type,type,s.replace(b,type)))
        elif (func == "pack_PROD" and (b == "c_complex" or b == "c_double_complex" or b == "c_long_double_complex")):
            yutils.display(OUTFILE, "%s dest;\n" % type)
            yutils.display(OUTFILE, "%s src = *((const %s *) (const void *) (sbuf + %s));\n" % (type,type,s.replace(b,type)))
            yutils.display(OUTFILE, "%s temp_dest = *((%s *) (void *) (dbuf + idx * sizeof(%s)));\n" % (type,type,type))
            yutils.display(OUTFILE, "dest.x = temp_dest.x * src.x - temp_dest.y * src.y;\n")
            yutils.display(OUTFILE, "dest.y = temp_dest.x * src.y + temp_dest.y * src.x;\n")
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) = dest;\n" % (type,type))
        elif (func == "pack_PROD"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) *= *((const %s *) (const void *) (sbuf + %s));\n"
                                       % (b, b, b, s))
        elif (func == "pack_BOR"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) |= *((const %s *) (const void *) (sbuf + %s));\n"
                                       % (b, b, b, s))
        elif (func == "pack_BAND"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) &= *((const %s *) (const void *) (sbuf + %s));\n"
                                       % (b, b, b, s))
        elif (func == "pack_BXOR"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) ^= *((const %s *) (const void *) (sbuf + %s));\n"
                                       % (b, b, b, s))
        elif (func == "pack_LOR"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) = (*((%s *) (void *) (dbuf + idx * sizeof(%s)))) || (*((const %s *) (const void *) (sbuf + %s)));\n"
                                       % (b, b, b, b, b, s))
        elif (func == "pack_LAND"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) = (*((%s *) (void *) (dbuf + idx * sizeof(%s)))) && (*((const %s *) (const void *) (sbuf + %s)));\n"
                                       % (b, b, b, b, b, s))
        elif (func == "pack_LXOR"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) = !(*((%s *) (void *) (dbuf + idx * sizeof(%s)))) != !(*((const %s *) (const void *) (sbuf + %s)));\n"
                                        % (b, b, b, b, b, s))
        elif (func == "pack_MAX" and (b == "float" or b == "double")):
            yutils.display(OUTFILE, "    %s x_[2] = {*((const %s *) (const void *) (sbuf + %s)), *((%s *) (void *) (dbuf + idx * sizeof(%s)))};\n" % (b, b, s, b, b))
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) = x_[*((const %s *) (const void *) (sbuf + %s)) < *((%s *) (void *) (dbuf + idx * sizeof(%s)))];\n"
                    % (b, b, b, s, b, b))
        elif (func == "pack_MAX"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) = *((const %s *) (const void *) (sbuf + %s)) ^ ((*((const %s *) (const void *) (sbuf + %s)) ^ *((%s *) (void *) (dbuf + idx * sizeof(%s)))) & -( *((const %s *) (const void *) (sbuf + %s)) < *((%s *) (void *) (dbuf + idx * sizeof(%s)))));\n"
                    % (b, b, b, s, b, s, b, b, b, s, b, b))
        elif (func == "pack_MIN" and (b == "float" or b == "double")):
            yutils.display(OUTFILE, "    %s x_[2] = {*((const %s *) (const void *) (sbuf + %s)), *((%s *) (void *) (dbuf + idx * sizeof(%s)))};\n" % (b, b, s, b, b))
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) = x_[*((const %s *) (const void *) (sbuf + %s)) > *((%s *) (void *) (dbuf + idx * sizeof(%s)))];\n"
                    % (b, b, b, s, b, b))
        elif (func == "pack_MIN"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + idx * sizeof(%s))) = *((%s *) (void *) (dbuf + idx * sizeof(%s))) ^ ((*((const %s *) (const void *) (sbuf + %s)) ^ *((%s *) (void *) (dbuf + idx * sizeof(%s)))) & -( *((const %s *) (const void *) (sbuf + %s)) < *((%s *) (void *) (dbuf + idx * sizeof(%s)))));\n"
                    % (b, b, b, b, b, s, b, b, b, s, b, b))
        elif (func == "unpack_REPLACE"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) = *((const %s *) (const void *) (sbuf + idx * sizeof(%s)));\n"
                                       % (type,s.replace(b,type),type,type))
        elif (func == "unpack_SUM"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) += *((const %s *) (const void *) (sbuf + idx * sizeof(%s)));\n"
                                       % (type,s.replace(b,type),type,type))
        elif (func == "unpack_PROD" and (b == "c_complex" or b == "c_double_complex" or b == "c_long_double_complex")):
            yutils.display(OUTFILE, "%s dest;\n" % type)
            yutils.display(OUTFILE, "%s src = *((const %s *) (const void *) (sbuf + idx * sizeof(%s)));\n" % (type,type,type))
            yutils.display(OUTFILE, "%s temp_dest = *((%s *) (void *) (dbuf + %s));\n" % (type,type,s.replace(b,type)))
            yutils.display(OUTFILE, "dest.x = temp_dest.x * src.x - temp_dest.y * src.y;\n")
            yutils.display(OUTFILE, "dest.y = temp_dest.x * src.y + temp_dest.y * src.x;\n")
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) = dest;\n" % (type,s.replace(b,type)))
        elif (func == "unpack_PROD"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) *= *((const %s *) (const void *) (sbuf + idx * sizeof(%s)));\n"
                                       % (b, s, b, b))
        elif (func == "unpack_BOR"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) |= *((const %s *) (const void *) (sbuf + idx * sizeof(%s)));\n"
                                       % (b, s, b, b))
        elif (func == "unpack_BAND"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) &= *((const %s *) (const void *) (sbuf + idx * sizeof(%s)));\n"
                                       % (b, s, b, b))
        elif (func == "unpack_BXOR"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) ^= *((const %s *) (const void *) (sbuf + idx * sizeof(%s)));\n"
                                       % (b, s, b, b))
        elif (func == "unpack_LOR"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) = (*((%s *) (void *) (dbuf + %s))) || (*((const %s *) (const void *) (sbuf + idx * sizeof(%s))));\n"
                                       % (b, s, b, s, b, b))
        elif (func == "unpack_LAND"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) = (*((%s *) (void *) (dbuf + %s))) && (*((const %s *) (const void *) (sbuf + idx * sizeof(%s))));\n"
                                       % (b, s, b, s, b, b))
        elif (func == "unpack_LXOR"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) = !(*((%s *) (void *) (dbuf + %s))) != !(*((const %s *) (const void *) (sbuf + idx * sizeof(%s))));\n"
                                       % (b, s, b, s, b, b))
        elif (func == "unpack_MAX" and (b == "float" or b == "double")):
            yutils.display(OUTFILE, "    %s x_[2] = {*((const %s *) (const void *) (sbuf + idx * sizeof(%s))), *((%s *) (void *) (dbuf + %s))};\n" % (b, b, b, b, s))
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) = x_[*((const %s *) (const void *) (sbuf + idx * sizeof(%s))) < *((%s *) (void *) (dbuf + %s))];\n"
                    % (b, s, b, b, b, s))
        elif (func == "unpack_MAX"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) = *((const %s *) (const void *) (sbuf + idx * sizeof(%s))) ^ ((*((const %s *) (const void *) (sbuf + idx * sizeof(%s))) ^ *((%s *) (void *) (dbuf + %s))) & -( *((const %s *) (const void *) (sbuf + idx * sizeof(%s))) < *((%s *) (void *) (dbuf + %s))));\n"
                    % (b, s, b, b, b, b, b, s, b, b, b, s))
        elif (func == "unpack_MIN" and (b == "float" or b == "double")):
            yutils.display(OUTFILE, "    %s x_[2] = {*((const %s *) (const void *) (sbuf + idx * sizeof(%s))), *((%s *) (void *) (dbuf + %s))};\n" % (b, b, b, b, s))
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) = x_[*((const %s *) (const void *) (sbuf + idx * sizeof(%s))) > *((%s *) (void *) (dbuf + %s))];\n"
                    % (b, s, b, b, b, s))
        elif (func == "unpack_MIN"):
            yutils.display(OUTFILE, "*((%s *) (void *) (dbuf + %s)) = *((%s *) (void *) (dbuf + %s)) ^ ((*((const %s *) (const void *) (sbuf + idx * sizeof(%s))) ^ *((%s *) (void *) (dbuf + %s))) & -( *((const %s *) (const void *) (sbuf + idx * sizeof(%s))) < *((%s *) (void *) (dbuf + %s))));\n"
                    % (b, s, b, s, b, b, b, s, b, b, b, s))

        yutils.display(OUTFILE, "}\n\n")


def write_headers():
    yutils.display(OUTFILE, "typedef signed char int8_t;\n")
    yutils.display(OUTFILE, "typedef signed short int int16_t;\n")
    yutils.display(OUTFILE, "typedef signed int int32_t;\n")
    yutils.display(OUTFILE, "typedef signed long int64_t;\n")
    yutils.display(OUTFILE, "typedef unsigned char uint8_t;\n")
    yutils.display(OUTFILE, "typedef unsigned short int uint16_t;\n")
    yutils.display(OUTFILE, "typedef unsigned int uint32_t;\n")
    yutils.display(OUTFILE, "typedef unsigned long uint64_t;\n")
    yutils.display(OUTFILE, "#include \"yaksuri_zei_md.h\"\n")
    yutils.display(OUTFILE, "\n")

def generate_define_kernels(b, darray):
    global k
    for op in gencomm.type_ops[b]:
        funclist = []
        funclist.append("pack_%s" % op)
        funclist.append("unpack_%s" % op)

        for func in funclist:
            ##### figure out the function name to use
            s = "yaksuri_zei_%s_" % func
            for d in darray:
                s = s + "%s_" % d
            s = s + b.replace(" ", "_")
            OUTFILE.write("#define %s  %d\n" % (s, k))
            k += 1

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

    ##### generate the core pack/unpack kernels
    darraylist = [ ]
    yutils.generate_darrays(gencomm.derived_types, darraylist, args.pup_max_nesting - 2)

    for b in builtin_types:
        filename = "src/backend/ze/pup/yaksuri_zei_pup_%s.cl" % b.replace(" ","_")
        yutils.copyright_c(filename)
        OUTFILE = open(filename, "a")
        write_headers()

        emptylist = [ ]
        for op in gencomm.type_ops[b]:
            generate_kernels(b, emptylist, op)

        OUTFILE.close()

    if args.pup_max_nesting > 0:
        for b in builtin_types:
            for d1 in gencomm.derived_types:
                ##### generate the core pack/unpack kernels (single level)
                filename = "src/backend/ze/pup/yaksuri_zei_pup_%s_%s.cl" % (d1, b.replace(" ","_"))
                yutils.copyright_c(filename)
                OUTFILE = open(filename, "a")
                write_headers()

                emptylist = [ ]
                emptylist.append(d1)
                for op in gencomm.type_ops[b]:
                    generate_kernels(b, emptylist, op)
                emptylist.pop()
                OUTFILE.close()

                ##### generate the core pack/unpack kernels (more than one level)
                if args.pup_max_nesting > 1:
                    for d2 in gencomm.derived_types:
                        filename = "src/backend/ze/pup/yaksuri_zei_pup_%s_%s_%s.cl" % (d1, d2, b.replace(" ","_"))
                        yutils.copyright_c(filename)
                        OUTFILE = open(filename, "a")
                        write_headers()

                        for darray in darraylist:
                            darray.append(d1)
                            darray.append(d2)
                            for op in gencomm.type_ops[b]:
                                generate_kernels(b, darray, op)
                            darray.pop()
                            darray.pop()

                        OUTFILE.close()

    ##### generate code to load modules/kernels used by init_hook
    filename = "src/backend/ze/hooks/yaksuri_zei_init_kernels.c"
    yutils.copyright_c(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "#include \"stdlib.h\"\n")
    yutils.display(OUTFILE, "#include \"yaksi.h\"\n")
    yutils.display(OUTFILE, "#include \"yaksuri_zei.h\"\n")
    yutils.display(OUTFILE, "#include \"level_zero/ze_api.h\"\n\n")

    num_modules = 0
    for b in builtin_types:
        OUTFILE.write("extern const unsigned char yaksuri_zei_pup_%s_str[];\n" % b.replace(" ", "_"))
        OUTFILE.write("extern const size_t yaksuri_zei_pup_%s_size;\n" % b.replace(" ", "_"))
        num_modules += 1
        for d1 in gencomm.derived_types:
            ##### generate the core pack/unpack kernels (single level)
            OUTFILE.write("extern const unsigned char yaksuri_zei_pup_%s_%s_str[];\n" % (d1, b.replace(" ", "_")))
            OUTFILE.write("extern const size_t yaksuri_zei_pup_%s_%s_size;\n" % (d1, b.replace(" ", "_")))
            num_modules += 1

            ##### generate the core pack/unpack kernels (more than one level)
            for d2 in gencomm.derived_types:
                OUTFILE.write("extern const unsigned char yaksuri_zei_pup_%s_%s_%s_str[];\n" % (d1, d2, b.replace(" ", "_")))
                OUTFILE.write("extern const size_t yaksuri_zei_pup_%s_%s_%s_size;\n" % (d1, d2, b.replace(" ", "_")))
                num_modules += 1
    OUTFILE.write("\n")
    OUTFILE.write("ze_module_handle_t *yaksuri_ze_modules[%d];\n\n" % num_modules)

    num_kernels = 0
    for b in builtin_types:
        for op in gencomm.type_ops[b]:
            num_kernels += 2
        if args.pup_max_nesting > 0:
            for d1 in gencomm.derived_types:
                for op in gencomm.type_ops[b]:
                    num_kernels += 2

                ##### generate the core pack/unpack kernels (more than one level)
                if args.pup_max_nesting > 1:
                    for d2 in gencomm.derived_types:
                        for darray in darraylist:
                            for op in gencomm.type_ops[b]:
                                num_kernels += 2
    OUTFILE.write("ze_kernel_handle_t *yaksuri_ze_kernels[%d];\n\n" % num_kernels)
    OUTFILE.write("const unsigned char * yaksuri_zei_pup_str[%d];\n" % num_modules)
    OUTFILE.write("unsigned long yaksuri_zei_pup_size[%d];\n\n" % num_modules)

    OUTFILE.write("int yaksuri_zei_kernel_module_map[%d];\n" % num_kernels)
    OUTFILE.write("const char * yaksuri_zei_kernel_funcs[%d] = {\n" % num_kernels)
    m = 0
    i = 0
    for b in builtin_types:
        for op in gencomm.type_ops[b]:
            for func in "pack", "unpack":
                OUTFILE.write("    \"yaksuri_zei_kernel_%s_%s_%s\",\t/* %d */\n" % (func, op, b.replace(" ", "_"), i))
                i += 1
        m += 1
        if args.pup_max_nesting > 0:
            for d1 in gencomm.derived_types:
                for op in gencomm.type_ops[b]:
                    for func in "pack", "unpack":
                        OUTFILE.write("    \"yaksuri_zei_kernel_%s_%s_%s_%s\",\t/* %d */\n" % (func, op, d1, b.replace(" ", "_"), i))
                        i += 1
                m += 1
                ##### generate the core pack/unpack kernels (more than one level)
                if args.pup_max_nesting > 1:
                    for d2 in gencomm.derived_types:
                        for darray in darraylist:
                            darray.append(d1)
                            darray.append(d2)
                            for op in gencomm.type_ops[b]:
                                for func in "pack","unpack":
                                    func_name = "yaksuri_zei_kernel_%s_%s_" % (func, op)
                                    for d in darray:
                                        func_name = func_name + "%s_" % d
                                    func_name = func_name + b.replace(" ", "_")
                                    OUTFILE.write("    \"%s\",\t/* %d */\n" % (func_name, i))
                                    i += 1
                            darray.pop()
                            darray.pop()
                        m += 1
    OUTFILE.write("};\n\n")

    # create modules using level zero API
    yutils.display(OUTFILE, "ze_result_t yaksuri_ze_init_module_kernel(void) {\n")
    OUTFILE.write("    ze_result_t zerr = ZE_RESULT_SUCCESS; \n")

    i = 0
    for b in builtin_types:
        OUTFILE.write("    yaksuri_zei_pup_str[%d] = yaksuri_zei_pup_%s_str;\n" % (i, b.replace(" ", "_")))
        OUTFILE.write("    yaksuri_zei_pup_size[%d] = yaksuri_zei_pup_%s_size;\n" % (i, b.replace(" ", "_")))
        i += 1
        if args.pup_max_nesting > 0:
            for d1 in gencomm.derived_types:
                OUTFILE.write("    yaksuri_zei_pup_str[%d] = yaksuri_zei_pup_%s_%s_str;\n" % (i, d1, b.replace(" ", "_")))
                OUTFILE.write("    yaksuri_zei_pup_size[%d] = yaksuri_zei_pup_%s_%s_size;\n" % (i, d1, b.replace(" ", "_")))
                i += 1
                ##### generate the core pack/unpack kernels (more than one level)
                if args.pup_max_nesting > 1:
                    for d2 in gencomm.derived_types:
                        OUTFILE.write("    yaksuri_zei_pup_str[%d] = yaksuri_zei_pup_%s_%s_%s_str;\n" % (i, d1, d2, b.replace(" ", "_")))
                        OUTFILE.write("    yaksuri_zei_pup_size[%d] = yaksuri_zei_pup_%s_%s_%s_size;\n" % (i, d1, d2, b.replace(" ", "_")))
                        i += 1
    OUTFILE.write("\n")

    i = 0
    m = 0
    for b in builtin_types:
        for op in gencomm.type_ops[b]:
            for func in "pack", "unpack":
                OUTFILE.write("    yaksuri_zei_kernel_module_map[%d] = %d;\n" % (i, m))    
                i += 1
        m += 1
        if args.pup_max_nesting > 0:
            for d1 in gencomm.derived_types:
                for op in gencomm.type_ops[b]:
                    for func in "pack", "unpack":
                        OUTFILE.write("    yaksuri_zei_kernel_module_map[%d] = %d;\n" % (i, m))
                        i += 1
                m += 1
                ##### generate the core pack/unpack kernels (more than one level)
                if args.pup_max_nesting > 1:
                    for d2 in gencomm.derived_types:
                        for darray in darraylist:
                            darray.append(d1)
                            darray.append(d2)
                            for op in gencomm.type_ops[b]:
                                for func in "pack","unpack":
                                    func_name = "yaksuri_zei_kernel_%s_%s_" % (func, op)
                                    for d in darray:
                                        func_name = func_name + "%s_" % d
                                    func_name = func_name + b.replace(" ", "_")
                                    OUTFILE.write("    yaksuri_zei_kernel_module_map[%d] = %d;\n" % (i, m))
                                    i += 1
                            darray.pop()
                            darray.pop()
                        m += 1
    OUTFILE.write("\n")
    OUTFILE.write("    return zerr; \n")
    yutils.display(OUTFILE, "}\n\n")
    OUTFILE.close()

    filename = "src/backend/ze/hooks/yaksuri_zei_finalize_kernels.c"
    yutils.copyright_c(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "#include \"stdlib.h\"\n")
    yutils.display(OUTFILE, "#include \"yaksi.h\"\n")
    yutils.display(OUTFILE, "#include \"yaksuri_zei.h\"\n")
    yutils.display(OUTFILE, "#include \"level_zero/ze_api.h\"\n\n")

    yutils.display(OUTFILE, "ze_result_t yaksuri_ze_finalize_module_kernel(void) {\n")
    OUTFILE.write("    ze_result_t zerr = ZE_RESULT_SUCCESS;\n")
    OUTFILE.write("    int i, k; \n\n")
    OUTFILE.write("    for (k=0; k<%d; k++) {\n" % num_kernels)
    OUTFILE.write("        if (yaksuri_ze_kernels[k] == NULL) continue;\n")
    OUTFILE.write("        for (i=0; i<yaksuri_zei_global.ndevices; i++) {\n")
    OUTFILE.write("            if (yaksuri_ze_kernels[k][i] == NULL) continue;\n")
    OUTFILE.write("            zerr = zeKernelDestroy(yaksuri_ze_kernels[k][i]);\n")
    OUTFILE.write("            if (zerr != ZE_RESULT_SUCCESS) { \n")
    OUTFILE.write("                fprintf(stderr, \"ZE Error (%s:%s,%d): %d \\n\", __func__, __FILE__, __LINE__, zerr); \n")
    OUTFILE.write("                goto fn_fail; \n");
    OUTFILE.write("            }\n");
    OUTFILE.write("            yaksuri_ze_kernels[k][i] = NULL;\n");
    OUTFILE.write("        }\n")
    OUTFILE.write("    }\n\n")
    OUTFILE.write("    for (k=0; k<%d; k++) {\n" % num_modules)
    OUTFILE.write("        if (yaksuri_ze_modules[k] == NULL) continue;\n")
    OUTFILE.write("        for (i=0; i<yaksuri_zei_global.ndevices; i++) {\n")
    OUTFILE.write("            if (yaksuri_ze_modules[k][i] == NULL) continue;\n")
    OUTFILE.write("            zerr = zeModuleDestroy(yaksuri_ze_modules[k][i]);\n")
    OUTFILE.write("            if (zerr != ZE_RESULT_SUCCESS) { \n")
    OUTFILE.write("                fprintf(stderr, \"ZE Error (%s:%s,%d): %d \\n\", __func__, __FILE__, __LINE__, zerr); \n")
    OUTFILE.write("                goto fn_fail; \n");
    OUTFILE.write("            }\n");
    OUTFILE.write("            yaksuri_ze_modules[k][i] = NULL;\n");
    OUTFILE.write("        }\n")
    OUTFILE.write("    }\n\n")
    OUTFILE.write("fn_exit:\n")
    OUTFILE.write("    return zerr; \n")
    OUTFILE.write("fn_fail:\n")
    OUTFILE.write("    goto fn_exit; \n")
    yutils.display(OUTFILE, "}\n")
    OUTFILE.close()

    ##### generate the core pack/unpack kernel declarations
    filename = "src/backend/ze/pup/yaksuri_zei_pup.h"
    yutils.copyright_c(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "#ifndef YAKSURI_ZEI_PUP_H_INCLUDED\n")
    yutils.display(OUTFILE, "#define YAKSURI_ZEI_PUP_H_INCLUDED\n")
    yutils.display(OUTFILE, "\n")

    # bit of an unnecessary header file, but creating it so we can
    # reuse the gencomm module
    k = 0
    for b in builtin_types:
        emptylist = [ ]
        generate_define_kernels(b, emptylist)
        for d1 in gencomm.derived_types:
            emptylist = [ ]
            emptylist.append(d1)
            generate_define_kernels(b, emptylist)
            emptylist.pop()
            for d2 in gencomm.derived_types:
                for darray in darraylist:
                    darray.append(d1)
                    darray.append(d2)
                    generate_define_kernels(b, darray)
                    darray.pop()
                    darray.pop()
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "#endif  /* YAKSURI_ZEI_PUP_H_INCLUDED */\n")
    OUTFILE.close()

    ##### generate the pup makefile
    filename = "src/backend/ze/pup/Makefile.pup.mk"
    yutils.copyright_makefile(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "libyaksa_la_SOURCES += \\\n")
    for b in builtin_types:
        yutils.display(OUTFILE, "\tsrc/backend/ze/pup/yaksuri_zei_pup_%s.c \\\n" % b.replace(" ","_"))
        if args.pup_max_nesting > 0:
            for d1 in gencomm.derived_types:
                yutils.display(OUTFILE, "\tsrc/backend/ze/pup/yaksuri_zei_pup_%s_%s.c \\\n" % (d1, b.replace(" ","_")))
                if args.pup_max_nesting > 1:
                    for d2 in gencomm.derived_types:
                        yutils.display(OUTFILE, "\tsrc/backend/ze/pup/yaksuri_zei_pup_%s_%s_%s.c \\\n" % (d1, d2, b.replace(" ","_")))
    yutils.display(OUTFILE, "\tsrc/backend/ze/pup/yaksuri_zei_pup.c\n")
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "noinst_HEADERS += \\\n")
    yutils.display(OUTFILE, "\tsrc/backend/ze/pup/yaksuri_zei_pup.h\n")
    OUTFILE.close()

    ##### generate the switching logic to select pup functions
    gencomm.populate_pupfns(args.pup_max_nesting, "ze", blklens, builtin_types, builtin_maps)

