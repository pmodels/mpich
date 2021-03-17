##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.mpi_api import *
from local_python.binding_c import *
from local_python import RE
import glob
import os

def main():
    binding_dir = "src/binding"
    c_dir = "src/binding/c"
    func_list = load_C_func_list(binding_dir)

    # -- Loading extra api prototypes (needed until other `buildiface` scripts are updated)
    G.mpi_declares = []
    proto_extra_h = "%s/mpi_proto_extra.h" % c_dir
    if os.path.exists(proto_extra_h):
        with open(proto_extra_h) as In:
            for line in In:
                if RE.match(r'(int MPIX?_.*MPICH_API_PUBLIC)', line):
                    G.mpi_declares.append(RE.m.group(1))

    # -- Generating code --
    for func in func_list:
        if 'not_implemented' in func:
            print("  skip %s (not_implemented)" % func['name'])
            pass
        else:
            G.out = []
            G.err_codes = {}

            # dumps the code to G.out array
            # Note: set func['_has_poly'] = False to skip embiggenning
            func['_has_poly'] = function_has_POLY_parameters(func)
            if func['_has_poly']:
                dump_mpi_c(func, "SMALL")
                dump_mpi_c(func, "BIG")
            else:
                dump_mpi_c(func, "SMALL")

        file_path = get_func_file_path(func, c_dir)
        dump_c_file(file_path, G.out)

        # add to mpi_sources for dump_Makefile_mk()
        G.mpi_sources.append(file_path)

    dump_Makefile_mk("%s/Makefile.mk" % c_dir)
    dump_mpir_impl_h("src/include/mpir_impl.h")
    dump_errnames_txt("%s/errnames.txt" % c_dir)
    dump_qmpi_register_h("src/mpi_t/qmpi_register.h")
    dump_mpi_proto_h("src/include/mpi_proto.h")
    dump_mtest_mpix_h("test/mpi/include/mtest_mpix.h")

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
