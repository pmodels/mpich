##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.mpi_api import load_mpi_api
from local_python.binding_c import dump_mpi_c, dump_Makefile_mk
from local_python import RE
import os
import glob

def main():
    load_mpi_api("maint/mpi_standard_api.txt")

    os.chdir("src/binding/c")
    api_files = glob.glob("*_api.txt")
    for f in api_files:
        if RE.match(r'(\w+)_api.txt', f):
            # The name in eg pt2pt_api.txt indicates the output folder.
            # Only the api functions with output folder will get generated.
            # This allows simple control of what functions to generate.
            load_mpi_api(f, RE.m.group(1))

    for func in G.FUNCS.values():
        if 'dir' in func:
            dump_mpi_c(func)

    dump_Makefile_mk()

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
