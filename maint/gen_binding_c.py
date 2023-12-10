##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.mpi_api import *
from local_python.binding_c import *
from local_python import RE
from local_python.info_hints import collect_info_hint_blocks
import glob

def main():
    # currently support: -output-mansrc
    G.parse_cmdline()

    binding_dir = G.get_srcdir_path("src/binding")
    c_dir = "src/binding/c"
    abi_dir = "src/binding/abi"
    func_list = load_C_func_list(binding_dir)

    # -- Loading extra api prototypes (needed until other `buildiface` scripts are updated)
    G.mpi_declares = []

    # -- functions that are not generated yet
    extras = []
    extras.append("MPI_DUP_FN")
    for a in ['f082c', 'c2f08', 'f082f', 'f2f08']:
        extras.append("MPI_Status_%s" % a)
    # now generate the prototypes
    for a in extras:
        func = G.FUNCS[a.lower()]
        mapping = G.MAPS['SMALL_C_KIND_MAP']
        G.mpi_declares.append(get_declare_function(func, False, "proto"))

    if 'output-mansrc' in G.opts:
        G.check_write_path(c_dir + '/mansrc/')
        G.hints = collect_info_hint_blocks("src")
    else:
        G.hints = None

    # -- Generating code --
    G.out = []
    G.out.append("#include \"mpiimpl.h\"")
    G.out.append("")
    G.doc3_src_txt = []
    G.need_dump_romio_reference = True

    # internal function to dump G.out into filepath
    def dump_out(file_path):
        G.check_write_path(file_path)
        dump_c_file(file_path, G.out)
        # add to mpi_sources for dump_Makefile_mk()
        G.mpi_sources.append(file_path)
        G.out = []
        G.out.append("#include \"mpiimpl.h\"")
        G.out.append("")
        G.need_dump_romio_reference = True

    # ----
    for func in func_list:
        G.err_codes = {}
        manpage_out = []

        # dumps the code to G.out array
        # Note: set func['_has_poly'] = False to skip embiggenning
        func['_has_poly'] = function_has_POLY_parameters(func)
        dump_mpi_c(func, False)
        if func['_has_poly']:
            dump_mpi_c(func, True)

        dump_manpage(func, manpage_out)

        if 'output-mansrc' in G.opts:
            f = get_mansrc_file_path(func, c_dir + '/mansrc')
            with open(f, "w") as Out:
                for l in manpage_out:
                    print(l.rstrip(), file=Out)
            G.doc3_src_txt.append(f)

    dump_out(c_dir + "/c_binding.c")

    # ---- dump c_binding_abi.c
    G.out = []
    G.out.append("#include \"mpiimpl.h\"")
    G.out.append("#include \"mpi_abi_util.h\"")
    G.out.append("")

    for func in func_list:
        if re.match(r'MPIX_', func['name']):
            if re.match(r'MPIX_(Grequest_|Type_iov)', func['name']):
                # needed by ROMIO
                pass
            else:
                continue
        func['_is_abi'] = True
        G.err_codes = {}
        # dumps the code to G.out array
        # Note: set func['_has_poly'] = False to skip embiggenning
        func['_has_poly'] = function_has_POLY_parameters(func)
        dump_mpi_c(func, False)
        if func['_has_poly']:
            dump_mpi_c(func, True)
        del func['_is_abi']

    abi_file_path = abi_dir + "/c_binding_abi.c"
    G.check_write_path(abi_file_path)
    dump_c_file(abi_file_path, G.out)
    G.out = []

    # -- Dump other files --
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
