##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.mpi_api import *
from local_python.binding_common import *
from local_python.binding_f77 import *
from local_python import RE
import os

def main():
    # currently support -no-real128, -no-mpiio, -aint-is-int
    G.parse_cmdline()

    binding_dir = G.get_srcdir_path("src/binding")
    f77_dir = "src/binding/fortran/mpif_h"

    func_list = load_C_func_list(binding_dir, True) # suppress noise

    if "no-mpiio" in G.opts:
        # a few MPI_File_xxx functions are already in (MPI_File_xxx_errhandler)
        func_list = [f for f in func_list if not f['name'].startswith('MPI_File_')]
    else:
        # FIXME: until romio interface is generated
        func_list.extend(get_mpiio_func_list())
    func_list.extend(get_f77_dummy_func_list())
    func_list.extend(get_type_create_f90_func_list())
    func_list.append(G.FUNCS['mpi_f_sync_reg'])

    # preprocess
    for func in func_list:
        check_func_directives(func)
    func_list = [f for f in func_list if '_skip_fortran' not in f]

    # fortran_binding.c
    G.out = []
    G.profile_out = []
    for func in func_list:
        G.out.append("")
        dump_f77_c_func(func)

    f = "%s/fortran_binding.c" % f77_dir
    dump_f77_c_file(f, G.out)

    f = "%s/fortran_profile.h" % f77_dir
    dump_f77_c_file(f, G.profile_out)

# ---------------------------------------------------------
if __name__ == "__main__":
    main()

