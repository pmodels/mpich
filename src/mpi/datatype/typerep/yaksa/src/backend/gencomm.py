#! /usr/bin/env python3
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import sys
sys.path.append('maint/')
import yutils

## loop through the derived and basic types to generate individual
## pack functions
derived_types = [ "hvector", "blkhindx", "hindexed", "contig", "resized" ]
type_ops = {'_Bool': {'REPLACE', 'LAND', 'LOR', 'LXOR'},
            'bool': {'REPLACE', 'LAND', 'LOR', 'LXOR'},
            'char': {'REPLACE'},
            'wchar_t': {'REPLACE'},
            'int8_t': {'REPLACE', 'SUM', 'PROD', 'MIN', 'MAX', 'LAND', 'LOR', 'LXOR', 'BAND', 'BOR', 'BXOR'},
            'int16_t': {'REPLACE', 'SUM', 'PROD', 'MIN', 'MAX', 'LAND', 'LOR', 'LXOR', 'BAND', 'BOR', 'BXOR'},
            'int32_t': {'REPLACE', 'SUM', 'PROD', 'MIN', 'MAX', 'LAND', 'LOR', 'LXOR', 'BAND', 'BOR', 'BXOR'},
            'int64_t': {'REPLACE', 'SUM', 'PROD', 'MIN', 'MAX', 'LAND', 'LOR', 'LXOR', 'BAND', 'BOR', 'BXOR'},
            'float': {'REPLACE', 'SUM', 'PROD', 'MIN', 'MAX'},
            'double': {'REPLACE', 'SUM', 'PROD', 'MIN', 'MAX'},
            'c_complex': {'REPLACE', 'SUM', 'PROD'},
            'c_double_complex': {'REPLACE', 'SUM', 'PROD'},
            'c_long_double_complex': {'REPLACE', 'SUM', 'PROD'},
            'long double': {'REPLACE', 'SUM', 'PROD', 'MIN', 'MAX'}}
########################################################################################
##### Switch statement generation for pup function selection
########################################################################################
def child_type_str(typelist):
    s = "type"
    for x in typelist:
        s = s + "->u.%s.child" % x
    return s

def switcher_builtin_element(backend, OUTFILE, blklens, typelist, pupstr, key, val):
    yutils.display(OUTFILE, "case %s:\n" % key.upper())

    if (len(typelist) == 0):
        t = ""
    else:
        t = typelist.pop()

    if (t == ""):
        nesting_level = 0
    else:
        nesting_level = len(typelist) + 1

    if ((t == "hvector" or t == "blkhindx") and (len(blklens) > 1)):
        yutils.display(OUTFILE, "switch (%s->u.%s.blocklength) {\n" % (child_type_str(typelist), t))
        for blklen in blklens:
            if (blklen != "generic"):
                yutils.display(OUTFILE, "case %s:\n" % blklen)
            else:
                yutils.display(OUTFILE, "default:\n")
            yutils.display(OUTFILE, "if (max_nesting_level >= %d) {\n" % nesting_level)
            if (backend == "ze"):
                for op in type_ops[val]:
                    yutils.display(OUTFILE, "%s->pack[YAKSA_OP__%s] = yaksuri_%si_pack_%s_%s_blklen_%s_%s;\n" % (backend, op, backend, op, pupstr, blklen, val))
                    yutils.display(OUTFILE, "%s->unpack[YAKSA_OP__%s] = yaksuri_%si_unpack_%s_%s_blklen_%s_%s;\n" % (backend, op, backend, op, pupstr, blklen, val))
                yutils.display(OUTFILE, "%s->name = \"yaksuri_%si_op_%s_blklen_%s_%s\";\n" % (backend, backend, pupstr, blklen, val))
            else:
                yutils.display(OUTFILE, "%s->pack = yaksuri_%si_pack_%s_blklen_%s_%s;\n" % (backend, backend, pupstr, blklen, val))
                yutils.display(OUTFILE, "%s->unpack = yaksuri_%si_unpack_%s_blklen_%s_%s;\n" % (backend, backend, pupstr, blklen, val))
                yutils.display(OUTFILE, "%s->name = \"yaksuri_%si_op_%s_blklen_%s_%s\";\n" % (backend, backend, pupstr, blklen, val))
            yutils.display(OUTFILE, "}\n")
            yutils.display(OUTFILE, "break;\n")
        yutils.display(OUTFILE, "}\n")
    elif (t != ""):
        yutils.display(OUTFILE, "if (max_nesting_level >= %d) {\n" % nesting_level)
        if (backend == "ze"):
            for op in type_ops[val]:
                yutils.display(OUTFILE, "%s->pack[YAKSA_OP__%s] = yaksuri_%si_pack_%s_%s_%s;\n" % (backend, op, backend, op, pupstr, val))
                yutils.display(OUTFILE, "%s->unpack[YAKSA_OP__%s] = yaksuri_%si_unpack_%s_%s_%s;\n" % (backend, op, backend, op, pupstr, val))
            yutils.display(OUTFILE, "%s->name = \"yaksuri_%si_op_%s_%s\";\n" % (backend, backend, pupstr, val))
        else:
            yutils.display(OUTFILE, "%s->pack = yaksuri_%si_pack_%s_%s;\n" % (backend, backend, pupstr, val))
            yutils.display(OUTFILE, "%s->unpack = yaksuri_%si_unpack_%s_%s;\n" % (backend, backend, pupstr, val))
            yutils.display(OUTFILE, "%s->name = \"yaksuri_%si_op_%s_%s\";\n" % (backend, backend, pupstr, val))
        yutils.display(OUTFILE, "}\n")
    else:
        if (backend == "ze"):
            for op in type_ops[val]:
                yutils.display(OUTFILE, "%s->pack[YAKSA_OP__%s] = yaksuri_%si_pack_%s_%s;\n" % (backend, op, backend, op, val))
                yutils.display(OUTFILE, "%s->unpack[YAKSA_OP__%s] = yaksuri_%si_unpack_%s_%s;\n" % (backend, op, backend, op, val))
            yutils.display(OUTFILE, "%s->name = \"yaksuri_%si_op_%s\";\n" % (backend, backend, val))
        else:
            yutils.display(OUTFILE, "%s->pack = yaksuri_%si_pack_%s;\n" % (backend, backend, val))
            yutils.display(OUTFILE, "%s->unpack = yaksuri_%si_unpack_%s;\n" % (backend, backend, val))

    if (t != ""):
        typelist.append(t)
    yutils.display(OUTFILE, "break;\n")

def switcher_builtin(backend, OUTFILE, blklens, builtin_types, builtin_maps, typelist, pupstr):
    yutils.display(OUTFILE, "switch (%s->u.builtin.handle) {\n" % child_type_str(typelist))

    for b in builtin_types:
        if b == "bool":
            switcher_builtin_element(backend, OUTFILE, blklens, typelist, pupstr, "YAKSA_TYPE___BOOL", "bool")
        else:
            switcher_builtin_element(backend, OUTFILE, blklens, typelist, pupstr, "YAKSA_TYPE__%s" % b.replace(" ", "_"), b.replace(" ", "_"))
    for key in builtin_maps:
        switcher_builtin_element(backend, OUTFILE, blklens, typelist, pupstr, key, builtin_maps[key])

    yutils.display(OUTFILE, "default:\n")
    yutils.display(OUTFILE, "    break;\n")
    yutils.display(OUTFILE, "}\n")

def switcher(backend, OUTFILE, blklens, builtin_types, builtin_maps, typelist, pupstr, nests):
    yutils.display(OUTFILE, "switch (%s->kind) {\n" % child_type_str(typelist))

    for x in range(len(derived_types)):
        d = derived_types[x]
        if (nests > 1):
            yutils.display(OUTFILE, "case YAKSI_TYPE_KIND__%s:\n" % d.upper())
            typelist.append(d)
            switcher(backend, OUTFILE, blklens, builtin_types, builtin_maps, typelist, pupstr + "_%s" % d, nests - 1)
            typelist.pop()
            yutils.display(OUTFILE, "break;\n")

    if (len(typelist)):
        yutils.display(OUTFILE, "case YAKSI_TYPE_KIND__BUILTIN:\n")
        switcher_builtin(backend, OUTFILE, blklens, builtin_types, builtin_maps, typelist, pupstr)
        yutils.display(OUTFILE, "break;\n")

    yutils.display(OUTFILE, "default:\n")
    yutils.display(OUTFILE, "    break;\n")
    yutils.display(OUTFILE, "}\n")


########################################################################################
##### main function
########################################################################################
def populate_pupfns(pup_max_nesting, backend, blklens, builtin_types, builtin_maps):
    ##### generate the switching logic to select pup functions
    filename = "src/backend/%s/pup/yaksuri_%si_populate_pupfns.c" % (backend, backend)
    yutils.copyright_c(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "#include <stdio.h>\n")
    yutils.display(OUTFILE, "#include \"yaksi.h\"\n")
    yutils.display(OUTFILE, "#include \"yaksu.h\"\n")
    yutils.display(OUTFILE, "#include \"yaksuri_%si.h\"\n" % backend)
    yutils.display(OUTFILE, "#include \"yaksuri_%si_populate_pupfns.h\"\n" % backend)
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "int yaksuri_%si_populate_pupfns(yaksi_type_s * type)\n" % backend)
    yutils.display(OUTFILE, "{\n")
    yutils.display(OUTFILE, "int rc = YAKSA_SUCCESS;\n")
    yutils.display(OUTFILE, "yaksuri_%si_type_s *%s = (yaksuri_%si_type_s *) type->backend.%s.priv;\n" \
                   % (backend, backend, backend, backend))
    yutils.display(OUTFILE, "\n")
    if (backend == "ze"):
        yutils.display(OUTFILE, "for (int i = 0; i < YAKSA_OP__LAST; i++) {\n")
        yutils.display(OUTFILE, "   %s->pack[i] = YAKSURI_KERNEL_NULL;\n" % backend)
        yutils.display(OUTFILE, "   %s->unpack[i] = YAKSURI_KERNEL_NULL;\n" % backend)
        yutils.display(OUTFILE, "}\n")
    else:
        yutils.display(OUTFILE, "%s->pack = YAKSURI_KERNEL_NULL;\n" % backend)
        yutils.display(OUTFILE, "%s->unpack = YAKSURI_KERNEL_NULL;\n" % backend)
        yutils.display(OUTFILE, "\n")

    yutils.display(OUTFILE, "switch (type->kind) {\n")
    if pup_max_nesting > 0:
        for dtype1 in derived_types:
            yutils.display(OUTFILE, "case YAKSI_TYPE_KIND__%s:\n" % dtype1.upper())
            yutils.display(OUTFILE, "switch (type->u.%s.child->kind) {\n" % dtype1)
            if pup_max_nesting > 1:
                for dtype2 in derived_types:
                    yutils.display(OUTFILE, "case YAKSI_TYPE_KIND__%s:\n" % dtype2.upper())
                    yutils.display(OUTFILE, "rc = yaksuri_%si_populate_pupfns_%s_%s(type);\n" % (backend, dtype1, dtype2))
                    yutils.display(OUTFILE, "break;\n")
                    yutils.display(OUTFILE, "\n")
            yutils.display(OUTFILE, "case YAKSI_TYPE_KIND__BUILTIN:\n")
            yutils.display(OUTFILE, "rc = yaksuri_%si_populate_pupfns_%s_builtin(type);\n" % (backend, dtype1))
            yutils.display(OUTFILE, "break;\n")
            yutils.display(OUTFILE, "\n")
            yutils.display(OUTFILE, "default:\n")
            yutils.display(OUTFILE, "    break;\n")
            yutils.display(OUTFILE, "}\n")
            yutils.display(OUTFILE, "break;\n")
            yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "case YAKSI_TYPE_KIND__BUILTIN:\n")
    yutils.display(OUTFILE, "rc = yaksuri_%si_populate_pupfns_builtin(type);\n" % backend)
    yutils.display(OUTFILE, "break;\n")
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "default:\n")
    yutils.display(OUTFILE, "    break;\n")
    yutils.display(OUTFILE, "}\n")
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "    return rc;\n")
    yutils.display(OUTFILE, "}\n");
    OUTFILE.close()

    if pup_max_nesting > 0:
        for dtype1 in derived_types:
            if pup_max_nesting > 1:
                for dtype2 in derived_types:
                    filename = "src/backend/%s/pup/yaksuri_%si_populate_pupfns_%s_%s.c" % (backend, backend, dtype1, dtype2)
                    yutils.copyright_c(filename)
                    OUTFILE = open(filename, "a")
                    yutils.display(OUTFILE, "#include <stdio.h>\n")
                    yutils.display(OUTFILE, "#include <stdlib.h>\n")
                    yutils.display(OUTFILE, "#include <wchar.h>\n")
                    yutils.display(OUTFILE, "#include \"yaksi.h\"\n")
                    yutils.display(OUTFILE, "#include \"yaksu.h\"\n")
                    yutils.display(OUTFILE, "#include \"yaksuri_%si.h\"\n" % backend)
                    yutils.display(OUTFILE, "#include \"yaksuri_%si_populate_pupfns.h\"\n" % backend)
                    yutils.display(OUTFILE, "#include \"yaksuri_%si_pup.h\"\n" % backend)
                    yutils.display(OUTFILE, "\n")
                    yutils.display(OUTFILE, "int yaksuri_%si_populate_pupfns_%s_%s(yaksi_type_s * type)\n" % (backend, dtype1, dtype2))
                    yutils.display(OUTFILE, "{\n")
                    yutils.display(OUTFILE, "int rc = YAKSA_SUCCESS;\n")
                    yutils.display(OUTFILE, "yaksuri_%si_type_s *%s = (yaksuri_%si_type_s *) type->backend.%s.priv;\n" \
                                   % (backend, backend, backend, backend))
                    yutils.display(OUTFILE, "\n")
                    yutils.display(OUTFILE, "char *str = getenv(\"YAKSA_ENV_MAX_NESTING_LEVEL\");\n")
                    yutils.display(OUTFILE, "int max_nesting_level;\n")
                    yutils.display(OUTFILE, "if (str) {\n")
                    yutils.display(OUTFILE, "max_nesting_level = atoi(str);\n")
                    yutils.display(OUTFILE, "} else {\n")
                    yutils.display(OUTFILE, "max_nesting_level = YAKSI_ENV_DEFAULT_NESTING_LEVEL;\n")
                    yutils.display(OUTFILE, "}\n")
                    yutils.display(OUTFILE, "\n")

                    pupstr = "%s_%s" % (dtype1, dtype2)
                    typelist = [ dtype1, dtype2 ]
                    switcher(backend, OUTFILE, blklens, builtin_types, builtin_maps, typelist, pupstr, pup_max_nesting - 1)
                    yutils.display(OUTFILE, "\n")
                    yutils.display(OUTFILE, "return rc;\n")
                    yutils.display(OUTFILE, "}\n")
                    OUTFILE.close()

            filename = "src/backend/%s/pup/yaksuri_%si_populate_pupfns_%s_builtin.c" % (backend, backend, dtype1)
            yutils.copyright_c(filename)
            OUTFILE = open(filename, "a")
            yutils.display(OUTFILE, "#include <stdio.h>\n")
            yutils.display(OUTFILE, "#include <stdlib.h>\n")
            yutils.display(OUTFILE, "#include <wchar.h>\n")
            yutils.display(OUTFILE, "#include \"yaksi.h\"\n")
            yutils.display(OUTFILE, "#include \"yaksu.h\"\n")
            yutils.display(OUTFILE, "#include \"yaksuri_%si.h\"\n" % backend)
            yutils.display(OUTFILE, "#include \"yaksuri_%si_populate_pupfns.h\"\n" % backend)
            yutils.display(OUTFILE, "#include \"yaksuri_%si_pup.h\"\n" % backend)
            yutils.display(OUTFILE, "\n")
            yutils.display(OUTFILE, "int yaksuri_%si_populate_pupfns_%s_builtin(yaksi_type_s * type)\n" % (backend, dtype1))
            yutils.display(OUTFILE, "{\n")
            yutils.display(OUTFILE, "int rc = YAKSA_SUCCESS;\n")
            yutils.display(OUTFILE, "yaksuri_%si_type_s *%s = (yaksuri_%si_type_s *) type->backend.%s.priv;\n" \
                           % (backend, backend, backend, backend))
            yutils.display(OUTFILE, "\n")
            yutils.display(OUTFILE, "char *str = getenv(\"YAKSA_ENV_MAX_NESTING_LEVEL\");\n")
            yutils.display(OUTFILE, "int max_nesting_level;\n")
            yutils.display(OUTFILE, "if (str) {\n")
            yutils.display(OUTFILE, "max_nesting_level = atoi(str);\n")
            yutils.display(OUTFILE, "} else {\n")
            yutils.display(OUTFILE, "max_nesting_level = YAKSI_ENV_DEFAULT_NESTING_LEVEL;\n")
            yutils.display(OUTFILE, "}\n")
            yutils.display(OUTFILE, "\n")

            pupstr = "%s" % dtype1
            typelist = [ dtype1 ]
            switcher_builtin(backend, OUTFILE, blklens, builtin_types, builtin_maps, typelist, pupstr)
            yutils.display(OUTFILE, "\n")
            yutils.display(OUTFILE, "return rc;\n")
            yutils.display(OUTFILE, "}\n")
            OUTFILE.close()

    ### Generate mapping for reduction kernels of contiguous datatypes
    filename = "src/backend/%s/pup/yaksuri_%si_populate_pupfns_builtin.c" % (backend, backend)
    yutils.copyright_c(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "#include <stdio.h>\n")
    yutils.display(OUTFILE, "#include <stdlib.h>\n")
    yutils.display(OUTFILE, "#include <wchar.h>\n")
    yutils.display(OUTFILE, "#include \"yaksi.h\"\n")
    yutils.display(OUTFILE, "#include \"yaksu.h\"\n")
    yutils.display(OUTFILE, "#include \"yaksuri_%si.h\"\n" % backend)
    yutils.display(OUTFILE, "#include \"yaksuri_%si_populate_pupfns.h\"\n" % backend)
    yutils.display(OUTFILE, "#include \"yaksuri_%si_pup.h\"\n" % backend)
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "int yaksuri_%si_populate_pupfns_builtin(yaksi_type_s * type)\n" % backend)
    yutils.display(OUTFILE, "{\n")
    yutils.display(OUTFILE, "int rc = YAKSA_SUCCESS;\n")
    yutils.display(OUTFILE, "yaksuri_%si_type_s *%s = (yaksuri_%si_type_s *) type->backend.%s.priv;\n" \
                    % (backend, backend, backend, backend))
    yutils.display(OUTFILE, "\n")

    pupstr = ""
    typelist = [ ]
    switcher_builtin(backend, OUTFILE, blklens, builtin_types, builtin_maps, typelist, pupstr)
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "return rc;\n")
    yutils.display(OUTFILE, "}\n")
    OUTFILE.close()

    ##### generate the Makefile for the pup function selection functions
    filename = "src/backend/%s/pup/Makefile.populate_pupfns.mk" % backend
    yutils.copyright_makefile(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "libyaksa_la_SOURCES += \\\n")
    if pup_max_nesting > 0:
        for dtype1 in derived_types:
            if pup_max_nesting > 1:
                for dtype2 in derived_types:
                    yutils.display(OUTFILE, "\tsrc/backend/%s/pup/yaksuri_%si_populate_pupfns_%s_%s.c \\\n" % (backend, backend, dtype1, dtype2))
            yutils.display(OUTFILE, "\tsrc/backend/%s/pup/yaksuri_%si_populate_pupfns_%s_builtin.c \\\n" % (backend, backend, dtype1))
    yutils.display(OUTFILE, "\tsrc/backend/%s/pup/yaksuri_%si_populate_pupfns_builtin.c \\\n" % (backend, backend))
    yutils.display(OUTFILE, "\tsrc/backend/%s/pup/yaksuri_%si_populate_pupfns.c\n" % (backend, backend))
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "noinst_HEADERS += \\\n")
    yutils.display(OUTFILE, "\tsrc/backend/%s/pup/yaksuri_%si_populate_pupfns.h\n" % (backend, backend))
    OUTFILE.close()

    ##### generate the header file for the pup function selection functions
    filename = "src/backend/%s/pup/yaksuri_%si_populate_pupfns.h" % (backend, backend)
    yutils.copyright_c(filename)
    OUTFILE = open(filename, "a")
    yutils.display(OUTFILE, "#ifndef YAKSURI_%sI_POPULATE_PUPFNS_H_INCLUDED\n" % backend.upper())
    yutils.display(OUTFILE, "#define YAKSURI_%sI_POPULATE_PUPFNS_H_INCLUDED\n" % backend.upper())
    yutils.display(OUTFILE, "\n")
    if pup_max_nesting > 0:
        for dtype1 in derived_types:
            if pup_max_nesting > 1:
                for dtype2 in derived_types:
                    yutils.display(OUTFILE, "int yaksuri_%si_populate_pupfns_%s_%s(yaksi_type_s * type);\n" % (backend, dtype1, dtype2))
            yutils.display(OUTFILE, "int yaksuri_%si_populate_pupfns_%s_builtin(yaksi_type_s * type);\n" % (backend, dtype1))
    yutils.display(OUTFILE, "int yaksuri_%si_populate_pupfns_builtin(yaksi_type_s * type);\n" % backend)
    yutils.display(OUTFILE, "\n")
    yutils.display(OUTFILE, "#endif  /* YAKSURI_%sI_POPULATE_PUPFNS_H_INCLUDED */\n" % backend.upper())
    OUTFILE.close()
