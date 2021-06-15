##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.mpi_api import *
from local_python.binding_common import *
from local_python.binding_f08 import *
from local_python import RE
import os

def main():
    # currently support -no-real128, -no-mpiio, -aint-is-int
    G.parse_cmdline()

    binding_dir = G.get_srcdir_path("src/binding")
    f08_dir = "src/binding/fortran/use_mpi_f08"
    G.check_write_path("%s/wrappers_f" % f08_dir)
    G.check_write_path("%s/wrappers_c" % f08_dir)
    func_list = load_C_func_list(binding_dir, True) # suppress noise
    if "no-mpiio" in G.opts:
        # a few MPI_File_xxx functions are already in (MPI_File_xxx_errhandler)
        func_list = [f for f in func_list if not f['name'].startswith('MPI_File_')]
    else:
        # FIXME: until romio interface is generated
        func_list.extend(get_mpiio_func_list())
    func_list.extend(get_type_create_f90_func_list())

    skip_large_list = []
    # skip large variations because MPI_ADDRESS_KIND == MPI_COUNT_KIND
    if 'aint-is-int' not in G.opts:
        skip_large_list.extend(["MPI_Op_create", "MPI_Register_datarep", "MPI_Type_create_resized", "MPI_Type_get_extent", "MPI_Type_get_true_extent", "MPI_File_get_type_extent", "MPI_Win_allocate", "MPI_Win_allocate_shared", "MPI_Win_create", "MPI_Win_shared_query"])
    # skip File large count functions because it is not implemented yet
    for func in func_list:
        if func['name'].startswith('MPI_File_') or func['name'] == 'MPI_Register_datarep':
            skip_large_list.append(func['name'])

    # preprocess
    for func in func_list:
        check_func_directives(func)
        if '_skip_fortran' in func:
            continue
        if function_has_POLY_parameters(func) and func['name'] not in skip_large_list:
            func['_need_large'] = True
        else:
            func['_need_large'] = False
        process_func_parameters(func)
    func_list = [f for f in func_list if '_skip_fortran' not in f]

    # f08_cdesc.c
    G.out = []
    G.decls = []
    for func in func_list:
        if need_cdesc(func):
            G.out.append("")
            dump_f08_wrappers_c(func, False)
            if func['_need_large']:
                G.out.append("")
                dump_f08_wrappers_c(func, True)
    f = "%s/wrappers_c/f08_cdesc.c" % f08_dir
    dump_cdesc_c(f, G.out)
    f = "%s/wrappers_c/cdesc_proto.h" % f08_dir
    dump_cdesc_proto_h(f, G.decls)

    # f08ts.f90
    G.out = []
    for func in func_list:
        dump_f08_wrappers_f(func, False)
        if func['_need_large']:
            dump_f08_wrappers_f(func, True)
    f = "%s/wrappers_f/f08ts.f90" % f08_dir
    dump_f90_file(f, G.out)

    do_profiling = True
    if do_profiling:
        temp_out = []
        for l in G.out:
            temp_out.append(re.sub(r'(subroutine|function)\s+(MPIX?)_', r'\1 P\2R_', l, flags=re.IGNORECASE))
        f = "%s/wrappers_f/pf08ts.f90" % f08_dir
        dump_f90_file(f, temp_out)
        temp_out = None

    # mpi_c_interface_{cdesc,nobuf}.f90
    G.out = []
    dump_interface_module_open("mpi_c_interface_cdesc")
    for func in func_list:
        if need_cdesc(func):
            dump_mpi_c_interface_cdesc(func, False)
            if func['_need_large']:
                dump_mpi_c_interface_cdesc(func, True)
    f_sync_reg = {'name':"MPI_F_sync_reg", 'parameters':[{'name':"buf", 'kind':"BUFFER", 't':'', 'large_only':None, 'param_direction':"in"}]}
    dump_mpi_c_interface_cdesc(f_sync_reg, False)
    dump_interface_module_close("mpi_c_interface_cdesc")
    f = "%s/mpi_c_interface_cdesc.f90" % f08_dir
    dump_f90_file(f, G.out)

    G.out = []
    dump_interface_module_open("mpi_c_interface_nobuf")
    for func in func_list:
        if not need_cdesc(func):
            dump_mpi_c_interface_nobuf(func, False)
            if func['_need_large']:
                dump_mpi_c_interface_nobuf(func, True)
    dump_interface_module_close("mpi_c_interface_nobuf")
    f = "%s/mpi_c_interface_nobuf.f90" % f08_dir
    dump_f90_file(f, G.out)

    # mpi_f08.f90 and pmpi_f08.f90
    G.out = []
    dump_F_module_open("mpi_f08")
    G.out.append("USE,intrinsic :: iso_c_binding, ONLY: c_ptr")
    G.out.append("USE :: pmpi_f08")
    G.out.append("USE :: mpi_f08_types")
    G.out.append("USE :: mpi_f08_compile_constants")
    G.out.append("USE :: mpi_f08_link_constants")
    G.out.append("USE :: mpi_f08_callbacks")
    G.out.append("")
    G.out.append("IMPLICIT NONE")
    for func in func_list:
        G.out.append("")
        func_name = get_function_name(func, False)
        G.out.append("INTERFACE %s" % func_name)
        G.out.append("INDENT")
        dump_mpi_f08(func, False)
        if func['_need_large']:
            G.out.append("")
            dump_mpi_f08(func, True)
        G.out.append("DEDENT")
        G.out.append("END INTERFACE %s" % func_name)
    G.out.append("")
    dump_F_module_close("mpi_f08")

    f = "%s/mpi_f08.f90" % f08_dir
    dump_f90_file(f, G.out)

    if do_profiling:
        temp_out = []
        for l in G.out:
            if l == "USE :: pmpi_f08":
                pass
            elif RE.match(r'((?:\s*end\s+)?module)\s+(\w+)', l, re.IGNORECASE):
                temp_out.append(RE.m.group(1) + ' p' + RE.m.group(2))
            elif RE.match(r'((?:\s*end\s+)?interface)\s+(\w+)', l, re.IGNORECASE):
                temp_out.append(RE.m.group(1) + ' P' + RE.m.group(2))
            else:
                temp_out.append(re.sub(r'(subroutine|function)\s+(MPIX?)_', r'\1 P\2R_', l, flags=re.IGNORECASE))
        f = "%s/pmpi_f08.f90" % f08_dir
        dump_f90_file(f, temp_out)
        temp_out = None

    # mpi_f08_types.f90
    G.out = []
    dump_mpi_f08_types()
    f = "%s/mpi_f08_types.f90" % f08_dir
    dump_f90_file(f, G.out)

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
