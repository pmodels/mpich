##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import re
import os

class G:
    abi_h_lines = []
    abi_datatypes = {}
    abi_ops = {}
    op_mask = 0x1f;
    datatype_mask = 0x1ff;

class RE:
    m = None
    def match(pat, str, flags=0):
        RE.m = re.match(pat, str, flags)
        return RE.m
    def search(pat, str, flags=0):
        RE.m = re.search(pat, str, flags)
        return RE.m

def main():
    load_mpi_abi_h("src/binding/abi/mpi_abi.h")
    dump_mpi_abi_internal_h("src/binding/abi/mpi_abi_internal.h")
    dump_romio_abi_internal_h("src/mpi/romio/include/romio_abi_internal.h")
    dump_mpi_abi_util_c("src/binding/abi/mpi_abi_util.c")

def load_mpi_abi_h(mpi_abi_h):
    with open(mpi_abi_h, "r") as In:
        G.abi_h_lines = In.readlines()

def dump_mpi_abi_internal_h(mpi_abi_internal_h):
    define_constants = {}
    def gen_mpi_abi_internal_h(out):
        re_Handle = r'\bMPI_(Comm|Datatype|Errhandler|Group|Info|Message|Op|Request|Session|Win|KEYVAL_INVALID|TAG_UB|IO|HOST|WTIME_IS_GLOBAL|APPNUM|LASTUSEDCODE|UNIVERSE_SIZE|WIN_BASE|WIN_DISP_UNIT|WIN_SIZE|WIN_CREATE_FLAVOR|WIN_MODEL)\b'
        for line in G.abi_h_lines:
            if RE.search(r'MPI_ABI_H_INCLUDED', line):
                # skip the include guard, harmless
                pass
            elif RE.match(r'\s*int MPI_(SOURCE|TAG|ERROR);', line):
                # no need to rename status fields
                out.append(line.rstrip())
            elif RE.match(r'\s*(int|MPI_Fint) .*(reserved|internal)\[(\d+)\];', line):
                # internal fields in MPI_Status/MPI_F08_status
                T = RE.m.group(1)
                n = int(RE.m.group(3))
                out.append("    %s count_lo;" % T)
                out.append("    %s count_hi_and_cancelled;" % T)
                out.append("    %s reserved[%d];" % (T, n - 2))
            elif RE.match(r'#define\s+(MPI_\w+)\s+\(?\((MPI_\w+)\)\s*(0x\w+)\)?', line):
                (name, T, val) = RE.m.group(1,2,3)
                if T == "MPI_Datatype":
                    idx = int(val, 0) & G.datatype_mask
                    G.abi_datatypes[idx] = name
                elif T == "MPI_Op":
                    idx = int(val, 0) & G.op_mask
                    G.abi_ops[idx] = name

                if T == "MPI_File":
                    # pass through
                    out.append(line.rstrip())
                else:
                    # replace param prefix
                    out.append(re.sub(r'\bMPI_', 'ABI_', line.rstrip()))
            elif RE.match(r'#define MPI_(LONG_LONG|C_COMPLEX)', line):
                # datatype aliases
                out.append(re.sub(r'\bMPI_', 'ABI_', line.rstrip()))
            elif RE.match(r'^typedef struct \w+\s*\*\s*(MPI_T_\w+);', line):
                # rename to internal mpi_t struct type
                T = RE.m.group(1)
                out.append("typedef struct %s_s *%s;" % (re.sub(r'MPI_', 'MPIR_', T), T))
            elif RE.match(r'^(int|double|MPI_\w+) (P?MPI\w+)\((.*)\);', line):
                # prototypes, rename param prefix, add MPICH_API_PUBLIC
                (T, name, param) = RE.m.group(1,2,3)
                T=re.sub(re_Handle, r'ABI_\1', T)
                param=re.sub(re_Handle, r'ABI_\1', param)
                out.append("%s %s(%s) MPICH_API_PUBLIC;" % (T, name, param))
            elif RE.match(r'^extern\s+(\w+\s*\**)\s+(MPI_\w+);', line):
                (T, name) = RE.m.group(1,2)
                out.append("extern %s %s MPICH_API_PUBLIC;" % (T, name))
            elif RE.match(r'\s*(MPI_THREAD_\w+)\s*=\s*(\d+)', line):
                # These constants need to be converted from enum-constants into macro-constants
                define_constants[RE.m.group(1)] = RE.m.group(2)
                pass
            else:
                # replace param prefix
                out.append(re.sub(re_Handle, r'ABI_\1', line.rstrip()))

    # ----
    output_lines = []
    gen_mpi_abi_internal_h(output_lines)

    print(" --> [%s]" % mpi_abi_internal_h)
    with open(mpi_abi_internal_h, "w") as Out:
        dump_copyright(Out)
        print("#ifndef MPI_ABI_INTERNAL_H_INCLUDED", file=Out)
        print("#define MPI_ABI_INTERNAL_H_INCLUDED", file=Out)
        print("", file=Out)

        for k in define_constants:
            print("#define %s %s" % (k, define_constants[k]), file=Out)
        print("", file=Out)
        for line in output_lines:
            print(line, file=Out)
        print("", file=Out)
        
        print("#endif /* MPI_ABI_INTERNAL_H_INCLUDED */", file=Out)

def dump_romio_abi_internal_h(romio_abi_internal_h):
    def gen_romio_abi_internal_h(out):
        for line in G.abi_h_lines:
            if RE.search(r'MPI_ABI_H_INCLUDED', line):
                # skip the include guard, harmless
                pass
            elif RE.match(r'typedef struct.*\bMPI_File;\s*$', line):
                out.append("typedef struct ADIOI_FileD *MPI_File;")
            elif RE.match(r'(int|double|MPI_\w+) (P?MPI\w+)\((.*)\);', line):
                # prototypes, rename param prefix, add MPICH_API_PUBLIC
                (T, name, param) = RE.m.group(1,2,3)
                if RE.match(r'P?MPI_(File_\w+|Register_datarep\w*)', name):
                    out.append("%s %s(%s) ROMIO_API_PUBLIC;" % (T, name, param))
                else:
                    out.append("%s %s(%s);" % (T, name, param))
            else:
                # replace param prefix
                out.append(line.rstrip())

    def add_romio_visibility(out):
        out.append("#if defined(HAVE_VISIBILITY)")
        out.append("#define ROMIO_API_PUBLIC __attribute__((visibility (\"default\")))")
        out.append("#else")
        out.append("#define ROMIO_API_PUBLIC")
        out.append("#endif")
        out.append("")

    def add_mpix_grequest(out):
        out.append("#define MPIO_Request MPI_Request")
        out.append("#define MPIO_USES_MPI_REQUEST")
        out.append("#define MPIO_Wait MPI_Wait")
        out.append("#define MPIO_Test MPI_Test")
        out.append("#define PMPIO_Wait PMPI_Wait")
        out.append("#define PMPIO_Test PMPI_Test")
        out.append("#define MPIO_REQUEST_DEFINED")
        out.append("")
        # they are not in the header, but they are in c_binding_abi.c
        out.append("typedef int MPIX_Grequest_class;")
        out.append("typedef int (MPIX_Grequest_poll_function)(void *, MPI_Status *);")
        out.append("typedef int (MPIX_Grequest_wait_function)(int, void **, double, MPI_Status *);")
        out.append("int MPIX_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, MPIX_Grequest_poll_function *poll_fn, MPIX_Grequest_wait_function *wait_fn, void *extra_state, MPI_Request *request);")
        out.append("int MPIX_Grequest_class_create(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, MPIX_Grequest_poll_function *poll_fn, MPIX_Grequest_wait_function *wait_fn, MPIX_Grequest_class *greq_class);")
        out.append("int MPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, void *extra_state, MPI_Request *request);")
        out.append("int PMPIX_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, MPIX_Grequest_poll_function *poll_fn, MPIX_Grequest_wait_function *wait_fn, void *extra_state, MPI_Request *request);")
        out.append("int PMPIX_Grequest_class_create(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, MPIX_Grequest_poll_function *poll_fn, MPIX_Grequest_wait_function *wait_fn, MPIX_Grequest_class *greq_class);")
        out.append("int PMPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, void *extra_state, MPI_Request *request);")
        out.append("")

    def add_mpix_iov(out):
        out.append("typedef struct MPIX_Iov {")
        out.append("    void *iov_base;")
        out.append("    MPI_Aint iov_len;")
        out.append("} MPIX_Iov;")
        out.append("")
        out.append("int MPIX_Type_iov_len(MPI_Datatype datatype, MPI_Count max_iov_bytes, MPI_Count *iov_len, MPI_Count *actual_iov_bytes);")
        out.append("int MPIX_Type_iov(MPI_Datatype datatype, MPI_Count iov_offset, MPIX_Iov *iov, MPI_Count max_iov_len, MPI_Count *actual_iov_len);")
        out.append("")

    def add_other(out):
        out.append("#define MPICH")  # ref. mpi-io/mpich_fileutil.c
        out.append("#define MPIR_ERRORS_THROW_EXCEPTIONS (MPI_Errhandler)0")

    # ----
    output_lines = []
    add_romio_visibility(output_lines)
    gen_romio_abi_internal_h(output_lines)
    add_mpix_grequest(output_lines)
    add_mpix_iov(output_lines)
    add_other(output_lines)

    print(" --> [%s]" % romio_abi_internal_h)
    with open(romio_abi_internal_h, "w") as Out:
        dump_copyright(Out)
        print("#ifndef ROMIO_ABI_INTERNAL_H_INCLUDED", file=Out)
        print("#define ROMIO_ABI_INTERNAL_H_INCLUDED", file=Out)
        print("", file=Out)

        for line in output_lines:
            print(line, file=Out)
        print("", file=Out)
        
        print("#endif /* ROMIO_ABI_INTERNAL_H_INCLUDED */", file=Out)

def dump_mpi_abi_util_c(mpi_abi_util_c):
    def dump_init_once(Out):
        print("    static int initialized = 0;", file=Out)
        print("    if (initialized) {", file=Out)
        print("        return;", file=Out)
        print("    }", file=Out)
        print("    initialized = 1;", file=Out)
        print("", file=Out)
    # ----
    print(" --> [%s]" % mpi_abi_util_c)
    with open(mpi_abi_util_c, "w") as Out:
        dump_copyright(Out)
        print("#include \"mpiimpl.h\"", file=Out)
        print("#include \"mpi_abi_util.h\"", file=Out)
        print("", file=Out)
        print("MPI_Datatype abi_datatype_builtins[ABI_MAX_DATATYPE_BUILTINS];", file=Out)
        print("MPI_Op abi_op_builtins[ABI_MAX_OP_BUILTINS];", file=Out)
        print("", file=Out)
        print("void ABI_init_builtins(void)", file=Out)
        print("{", file=Out)
        dump_init_once(Out)
        for idx in G.abi_datatypes.keys():
            print("    abi_datatype_builtins[%d] = %s;" % (idx, G.abi_datatypes[idx]), file=Out)
        print("", file=Out)
        for idx in G.abi_ops.keys():
            print("    abi_op_builtins[%d] = %s;" % (idx, G.abi_ops[idx]), file=Out)
        print("}", file=Out)
    num_datatypes = len(G.abi_datatypes)
    G.abi_h_lines.append("#define ABI_MAX_DATATYPE_BUILTINS %d" % num_datatypes)

def dump_copyright(Out):
    print("/*", file=Out)
    print(" * Copyright (C) by Argonne National Laboratory", file=Out)
    print(" *     See COPYRIGHT in top-level directory", file=Out)
    print(" */", file=Out)
    print("/* This file is automatically generated. DO NOT EDIT. */", file=Out)
    print("", file=Out)

#----------------------------------------
if __name__ == "__main__":
    main()
