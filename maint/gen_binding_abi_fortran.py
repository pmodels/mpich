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
    f77_dir = "src/binding/abi_fortran/mpif_h"

    func_list = load_C_func_list(binding_dir, True) # suppress noise

    func_list.extend(get_mpiio_func_list())
    func_list.extend(get_f77_dummy_func_list())
    func_list.append(G.FUNCS['mpi_f_sync_reg'])

    # preprocess
    for func in func_list:
        check_func_directives(func)
    func_list = [f for f in func_list if '_skip_fortran' not in f]

    # fortran_binding.c
    def has_cptr(func):
        for p in func['parameters']:
            if p['kind'] == 'C_BUFFER':
                return True
        return False

    G.out = []
    G.profile_out = []
    for func in func_list:
        G.out.append("")
        dump_f77_c_func(func)
        if has_cptr(func):
            dump_f77_c_func(func, True)

    f = "%s/fortran_binding.c" % f77_dir
    dump_f77_c_file(f, G.out)

    f = "%s/fortran_profile.h" % f77_dir
    dump_f77_c_file(f, G.profile_out)

    # .in files has to be generated in the source tree
    if G.is_autogen():
        G.mpih_defines = {}
        load_mpi_abi_h("src/binding/abi/mpi_abi.h")
        load_mpi_abi_h("src/binding/abi_fortran/mpi_abi_fort.h")
        del G.mpih_defines['MPI_ABI_Aint']
        del G.mpih_defines['MPI_ABI_Offset']
        del G.mpih_defines['MPI_ABI_Count']
        del G.mpih_defines['MPI_ABI_Fint']
        G.mpih_defines['MPI_LONG_LONG_INT'] = G.mpih_defines['MPI_LONG_LONG']
        G.mpih_defines['MPI_C_COMPLEX'] = G.mpih_defines['MPI_C_FLOAT_COMPLEX']

        G.mpih_defines['MPI_INTEGER_KIND'] = 4
        G.mpih_defines['MPI_ADDRESS_KIND'] = 8
        G.mpih_defines['MPI_COUNT_KIND'] = 8
        G.mpih_defines['MPI_OFFSET_KIND'] = 8
        for name in ['INTEGER_KIND', 'ADDRESS_KIND', 'COUNT_KIND', 'OFFSET_KIND']:
            if name in os.environ:
                G.mpih_defines['MPI_' + name] = os.environ[name]
        autoconf_macros = {}
        autoconf_macros['FORTRAN_MPI_OFFSET'] = 'INTEGER(KIND=MPI_OFFSET_KIND)'
        autoconf_macros['DLLIMPORT'] = ''

        f = "%s/mpif.h" % f77_dir
        dump_mpif_h(f, autoconf_macros)

def load_mpi_abi_h(f):
    # load constants into G.mpih_defines
    with open(f, "r") as In:
        for line in In:
            # trim trailing comments
            line = re.sub(r'\s+\/\*.*', '', line)
            if RE.match(r'#define\s+(MPI_\w+)\s+(.+)', line):
                # direct macros
                (name, val) = RE.m.group(1, 2)
                if RE.match(r'\(+MPI_\w+\)\(?0x([0-9a-fA-F]+)', val):
                    # handle constants
                    val = int(RE.m.group(1), 16)
                elif RE.match(r'\(+MPI_Offset\)(-?\d+)', val):
                    # MPI_DISPLACEMENT_CURRENT
                    val = RE.m.group(1)
                elif RE.match(r'0x([0-9a-fA-F]+)', val):
                    # direct hex constants (KEYVAL constants)
                    val = int(RE.m.group(1), 16)
                    if RE.match(r'MPI_(TAG_UB|HOST|IO|WTIME_IS_GLOBAL|UNIVERSE_SIZE|LASTUSEDCODE|APPNUM|WIN_(BASE|SIZE|DISP_UNIT|CREATE_FLAVOR|MODEL))', name):
                        # KEYVAL, Fortran value is C-value + 1
                        val = val + 1
                elif RE.match(r'MPI_MAX_', name):
                    # Fortran string buffer limit need be 1-less
                    if re.match(r'@\w+@', val):
                        val += "-1"
                    else:
                        val = int(val) - 1
                elif RE.match(r'\(([-\d]+)\)', val):
                    # take off the extra parentheses
                    val = RE.m.group(1)

                G.mpih_defines[name] = val
            elif RE.match(r'\s+(MPI_\w+)\s*=\s*(\d+)', line):
                # enum values
                (name, val) = RE.m.group(1, 2)
                G.mpih_defines[name] = val

# ---------------------------------------------------------
if __name__ == "__main__":
    main()

