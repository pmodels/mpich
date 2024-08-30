##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.mpi_api import *
from local_python.binding_common import *
from local_python.binding_f90 import *
from local_python import RE
import os

def main():
    G.parse_cmdline()

    binding_dir = G.get_srcdir_path("src/binding")
    f90_dir = "src/binding/fortran/use_mpi"
    func_list = load_C_func_list(binding_dir, True) # suppress noise
    if "no-mpiio" in G.opts:
        func_list = [f for f in func_list if not f['name'].startswith('MPI_File_')]
    else:
        func_list.extend(get_mpiio_func_list())
    func_list.extend(get_f77_dummy_func_list())

    def has_cptr(func):
        if G.opts['iso-c-binding'] == "yes":
            for p in func['parameters']:
                if p['kind'] == 'C_BUFFER':
                    return True
        return False

    # mpi_base.f90
    G.out = []
    dump_F_module_open("mpi_base")
    is_pmpi = False
    for func in func_list:
        check_func_directives(func)
        if '_skip_fortran' in func:
            continue
        G.out.append("INTERFACE %s" % func['name'])
        G.out.append("INDENT")
        dump_f90_func(func, is_pmpi)
        if has_cptr(func):
            # specific interface using "TYPE(C_PTR)"
            dump_f90_func(func, is_pmpi, True)
        G.out.append("DEDENT")
        G.out.append("END INTERFACE")
        G.out.append("")
    dump_F_module_close("mpi_base")

    f = "%s/mpi_base.f90" % f90_dir
    dump_f90_file(f, G.out)

    # pmpi_base.f90
    G.out = []
    dump_F_module_open("pmpi_base")
    is_pmpi = True
    for func in func_list:
        check_func_directives(func)
        if '_skip_fortran' in func:
            continue
        G.out.append("INTERFACE P%s" % func['name'])
        G.out.append("INDENT")
        dump_f90_func(func, is_pmpi)
        if re.match(r'mpi_alloc_mem', func['name'], re.IGNORECASE) and G.opts['iso-c-binding'] == "yes":
            # specific interface using "TYPE(C_PTR)"
            dump_f90_func(func, is_pmpi, True)
        G.out.append("DEDENT")
        G.out.append("END INTERFACE")
        G.out.append("")
    dump_F_module_close("pmpi_base")

    f = "%s/pmpi_base.f90" % f90_dir
    dump_f90_file(f, G.out)

    # mpi_constants.f90
    G.out = []
    dump_F_module_open("mpi_constants")
    dump_f90_constants()
    dump_F_module_close("mpi_constants")

    f = "%s/mpi_constants.f90" % f90_dir
    dump_f90_file(f, G.out)

    # mpi_sizeofs.f90
    G.out = []
    dump_F_module_open("mpi_sizeofs")
    dump_f90_sizeofs()
    dump_F_module_close("mpi_sizeofs")

    f = "%s/mpi_sizeofs.f90" % f90_dir
    dump_f90_file(f, G.out)

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
