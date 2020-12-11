##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.mpi_api import load_mpi_api
from local_python.binding_c import *
from local_python import RE
import glob

def main():
    # -- Loading APIs --
    print("Loading maint/mpi_standard_api.txt ...")
    load_mpi_api("maint/mpi_standard_api.txt")

    binding_dir = "src/binding/c"
    api_files = glob.glob("%s/*_api.txt" % binding_dir)
    for f in api_files:
        if RE.match(r'.*\/(\w+)_api.txt', f):
            # The name in eg pt2pt_api.txt indicates the output folder.
            # Only the api functions with output folder will get generated.
            # This allows simple control of what functions to generate.
            print("Loading %s ..." % f)
            load_mpi_api(f, RE.m.group(1))

    # -- Generating code --
    func_list = [f for f in G.FUNCS.values() if 'dir' in f]
    func_list.sort(key = lambda f: f['dir'])
    for func in func_list:
        if RE.search(r'not_implemented', func['attrs']):
            print("  skip %s (not_implemented)" % func['name'])
            pass
        else:
            G.out = []
            G.err_codes = {}
            mapping = G.MAPS['SMALL_C_KIND_MAP']

            # dumps the code to G.out array
            dump_mpi_c(func, mapping)

            file_path = get_func_file_path(func, binding_dir)
            dump_c_file(file_path, G.out)

            # add to mpi_sources for dump_Makefile_mk()
            G.mpi_sources.append(file_path)

    dump_Makefile_mk("src/binding/c/Makefile.mk")
    dump_mpir_impl_h("src/include/mpir_impl.h")

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
