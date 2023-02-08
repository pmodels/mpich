##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##
#
# This script generates all MPI function prototypes for both
# MPI and PMPI prefixes.
#
# It serves as an example of how to use the python framework.
#

from local_python import MPI_API_Global as G
from local_python.mpi_api import *
from local_python.binding_c import *
from local_python import RE

def main():
    binding_dir = G.get_srcdir_path("src/binding")
    load_mpi_api(binding_dir + "/mpi_standard_api.txt")
    load_mpi_mapping(binding_dir + "/apis_mapping.txt")

    func_list = G.FUNCS.values()

    decls = []
    fndefs = []
    for func in func_list:
        if re.match(r'.*_(function)', func['name']):
            # these are function typedefs
            fndefs.append(get_declare_function(func, False))
            if function_has_POLY_parameters(func):
                fndefs.append(get_declare_function(func, True))
        else:
            decls.append(get_declare_function(func, False))
            if function_has_POLY_parameters(func):
                decls.append(get_declare_function(func, True))
    
    for l in fndefs:
        print("typedef " + l + ";")

    for l in decls:
        print(l + ";")

    for l in decls:
        # assume the first MPI_ is the function name
        print(re.sub(r'\b(MPI_\w+)\(', r'P\1(', l) + ";")

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
