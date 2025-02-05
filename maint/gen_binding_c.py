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
    # currently support: -single-source
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

    # -- prescan the functions and set internal attributes
    for func in func_list:
        # Note: set func['_has_poly'] = False to skip embiggenning
        func['_has_poly'] = function_has_POLY_parameters(func)

        if 'replace' in func and 'body' not in func:
            m = re.search(r'with\s+(MPI_\w+)', func['replace'])
            repl_func = G.FUNCS[m.group(1).lower()]
            if '_replaces' not in repl_func:
                repl_func['_replaces'] = []
            repl_func['_replaces'].append(func)


    # We generate io functions separately for now
    io_func_list = [f for f in func_list if f['dir'] == 'io']
    func_list = [f for f in func_list if f['dir'] != 'io']

    # -- Generating code --
    G.doc3_src_txt = []
    G.poly_aliases = [] # large-count mansrc aliases
    G.need_dump_romio_reference = True

    # internal function to dump G.out into filepath
    def dump_out(file_path):
        G.check_write_path(file_path)
        dump_c_file(file_path, G.out)
        # add to mpi_sources for dump_Makefile_mk()
        G.mpi_sources.append(file_path)
        G.need_dump_romio_reference = True

    def dump_func(func, manpage_out):
        # dumps the code to G.out array
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
            if func['_has_poly']:
                G.poly_aliases.append(func['name'])

    def dump_func_abi(func):
        func['_is_abi'] = True
        G.err_codes = {}
        # dumps the code to G.out array
        dump_mpi_c(func, False)
        if func['_has_poly']:
            dump_mpi_c(func, True)
        del func['_is_abi']

    def dump_c_binding():
        G.out = []
        G.out.append("#include \"mpiimpl.h\"")
        G.out.append("")
        for func in func_list:
            G.err_codes = {}
            manpage_out = []

            if 'replace' in func and 'body' not in func:
                continue

            dump_func(func, manpage_out)
            if '_replaces' in func:
                for t_func in func['_replaces']:
                    dump_func(t_func, manpage_out)

            if 'single-source' not in G.opts:
                # dump individual functions in separate source files
                dump_out(get_func_file_path(func, c_dir))
                G.out = []
                G.out.append("#include \"mpiimpl.h\"")
                G.out.append("")

        if 'single-source' in G.opts:
            # otherwise, dump all functions in binding.c
            dump_out(c_dir + "/c_binding.c")

    def dump_c_binding_abi():
        G.out = []
        G.out.append("#include \"mpiimpl.h\"")
        G.out.append("#include \"mpi_abi_util.h\"")
        G.out.append("")

        for func in func_list:
            if 'replace' in func and 'body' not in func:
                continue

            if re.match(r'MPIX_', func['name']):
                if re.match(r'MPIX_(Grequest_|Type_iov)', func['name']):
                    # needed by ROMIO
                    pass
                else:
                    continue
            dump_func_abi(func)
            if '_replaces' in func:
                for t_func in func['_replaces']:
                    dump_func_abi(t_func)

        abi_file_path = abi_dir + "/c_binding_abi.c"
        G.check_write_path(abi_file_path)
        dump_c_file(abi_file_path, G.out)

    def dump_io_funcs():
        G.out = []
        G.out.append("#include \"mpiimpl.h\"")
        G.out.append("#include \"mpir_io_impl.h\"")
        G.out.append("")

        for func in io_func_list:
            G.err_codes = {}
            manpage_out = []
            dump_func(func, manpage_out)

        dump_out(c_dir + "/io.c")

    def dump_io_funcs_abi():
        G.out = []
        G.out.append("#include \"mpichconf.h\"")
        G.out.append("#include \"io_abi_internal.h\"")
        G.out.append("#include \"mpir_io_impl.h\"")
        G.out.append("#include <limits.h>")
        G.out.append("")

        for func in io_func_list:
            dump_func_abi(func)

        abi_file_path = abi_dir + "/io_abi.c"
        G.check_write_path(abi_file_path)
        dump_c_file(abi_file_path, G.out)

    # ----
    dump_c_binding()
    dump_c_binding_abi()
    dump_io_funcs()
    dump_io_funcs_abi()

    if 'output-mansrc' in G.opts:
        f = c_dir + '/mansrc/' + 'poly_aliases.lst'
        with open(f, "w") as Out:
            for name in G.poly_aliases:
                print("%s - %s_c" % (name, name), file=Out)

    # -- Dump other files --
    G.check_write_path("src/include")
    G.check_write_path("src/mpi_t")
    G.check_write_path("src/include/mpi_proto.h")
    dump_Makefile_mk("%s/Makefile.mk" % c_dir)
    dump_mpir_impl_h("src/include/mpir_impl.h")
    dump_mpir_io_impl_h("src/include/mpir_io_impl.h")
    dump_errnames_txt("%s/errnames.txt" % c_dir)
    dump_qmpi_register_h("src/mpi_t/qmpi_register.h")
    dump_mpi_proto_h("src/include/mpi_proto.h")
    dump_mtest_mpix_h("test/mpi/include/mtest_mpix.h")

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
