##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.mpi_api import *
from local_python.binding_c import *
from local_python import RE
import glob

def main():
    binding_dir = G.get_srcdir_path("src/binding")
    c_dir = "src/binding/c"
    func_list = load_C_func_list(binding_dir)

    # -- Loading extra api prototypes (needed until other `buildiface` scripts are updated)
    G.mpi_declares = []

    # -- functions that are not generated yet
    extras = []
    extras.append("MPI_DUP_FN")
    for a in ['c2f', 'f2c', 'f082c', 'c2f08', 'f082f', 'f2f08']:
        extras.append("MPI_Status_%s" % a)
    for a in ['integer', 'real', 'complex']:
        extras.append("MPI_Type_create_f90_%s" % a)
    # now generate the prototypes
    for a in extras:
        func = G.FUNCS[a.lower()]
        mapping = G.MAPS['SMALL_C_KIND_MAP']
        G.mpi_declares.append(get_declare_function(func, False, "proto"))

    # -- Generating code --
    for func in func_list:
        G.out = []
        G.err_codes = {}

        # dumps the code to G.out array
        # Note: set func['_has_poly'] = False to skip embiggenning
        func['_has_poly'] = function_has_POLY_parameters(func)
        if func['_has_poly']:
            dump_mpi_c(func, False)
            dump_mpi_c(func, True)
        else:
            dump_mpi_c(func, False)

        file_path = get_func_file_path(func, c_dir)
        G.check_write_path(file_path)
        dump_c_file(file_path, G.out)

        # add to mpi_sources for dump_Makefile_mk()
        G.mpi_sources.append(file_path)

    G.check_write_path(c_dir)
    G.check_write_path("src/include")
    G.check_write_path("src/mpi_t")
    G.check_write_path("src/include/mpi_proto.h")
    dump_Makefile_mk("%s/Makefile.mk" % c_dir)
    dump_mpir_impl_h("src/include/mpir_impl.h")
    dump_errnames_txt("%s/errnames.txt" % c_dir)
    dump_qmpi_register_h("src/mpi_t/qmpi_register.h")
    dump_mpi_proto_h("src/include/mpi_proto.h")
    dump_mtest_mpix_h("test/mpi/include/mtest_mpix.h")

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
