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

    func_list = load_C_func_list(G.binding_dir)

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

    do_doc = False
    mansrc_dir = 'doc/mansrc/binding'
    if 'output-mansrc' in G.opts:
        do_doc = True
        G.check_write_path(mansrc_dir + '/')
        G.hints = collect_info_hint_blocks("src")
        G.semantics = collect_mansrc_semantics("doc/mansrc/semantics.adoc")
        collect_function_notes("doc/mansrc/funcnotes.txt")
    else:
        G.hints = None
        G.semantics = {}

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


    # We generate io and f2c functions separately for now
    f2c_func_list = []
    io_func_list = []
    remain_list = []
    for func in func_list:
        if re.match(r'.*_(f2c|c2f|c2f08|f082c|f2f08|f082f)', func['name']):
            f2c_func_list.append(func)
        elif func['dir'] == 'io':
            io_func_list.append(func)
        else:
            remain_list.append(func)
    func_list = remain_list

    # -- Generating code --
    G.doc3_src = []
    G.poly_aliases = [] # large-count mansrc aliases
    G.need_dump_romio_reference = True

    # internal function to dump G.out into filepath
    def dump_out(file_path):
        G.check_write_path(file_path)
        dump_c_file(file_path, G.out)
        # add to mpi_sources for dump_Makefile_mk()
        G.mpi_sources.append(file_path)
        G.need_dump_romio_reference = True

    def dump_out_no_make(file_path):
        G.check_write_path(file_path)
        dump_c_file(file_path, G.out)

    def dump_func(func, do_doc):
        G.err_codes = {}
        if do_doc:
            manpage_out = []

        # dumps the code to G.out array
        dump_mpi_c(func, False)
        if func['_has_poly']:
            dump_mpi_c(func, True)

        if do_doc:
            dump_manpage(func, manpage_out)
            f = get_mansrc_file_path(func, mansrc_dir)
            with open(f, "w") as Out:
                for l in manpage_out:
                    print(l.rstrip(), file=Out)
            G.doc3_src.append(f)
            if func['_has_poly']:
                G.poly_aliases.append(func['name'])

    def dump_func_abi(func):
        func['_is_abi'] = True
        # dumps the code to G.out array
        dump_mpi_c(func, False)
        if func['_has_poly']:
            dump_mpi_c(func, True)
        del func['_is_abi']

    # f2c functions need be build by Fortran binding
    def dump_func_f2c(func):
        func_decl = get_declare_function(func, False)

        G.out.append("")

        # declare with MPICH_API_PUBLIC
        G.out.append("%s MPICH_API_PUBLIC;" % func_decl)
        if RE.match(r'(\w+) (MPI.*)', func_decl):
            G.out.append("%s P%s MPICH_API_PUBLIC;" % RE.m.group(1,2))
        G.out.append("")

        func['_is_large'] = False
        G.out.append("")
        dump_profiling(func)

        dump_line_with_break(func_decl)
        G.out.append("{")
        params = func['c_parameters']
        if RE.match(r'MPI_Status_(.*)', func['name']):
            G.out.append("    return MPIR_Status_%s_impl(%s, %s);" % (RE.m.group(1), params[0]['name'], params[1]['name']))
        elif RE.match(r'(MPI.*)_(f2c)', func['name']):
            G.out.append("    return P%s_fromint(%s);" % (RE.m.group(1), params[0]['name']))
        elif RE.match(r'(MPI.*)_(c2f)', func['name']):
            G.out.append("    return P%s_toint(%s);" % (RE.m.group(1), params[0]['name']))
        G.out.append("}")

    def dump_c_binding():
        G.out = []
        G.out.append("#include \"mpiimpl.h\"")
        G.out.append("#include \"mpii_fortlogical.h\"")
        G.out.append("")
        for func in func_list:
            if 'replace' in func and 'body' not in func:
                continue

            dump_func(func, do_doc)
            if '_replaces' in func:
                for t_func in func['_replaces']:
                    dump_func(t_func, do_doc)

            if 'single-source' not in G.opts:
                # dump individual functions in separate source files
                dump_out(get_func_file_path(func, G.c_dir))
                G.out = []
                G.out.append("#include \"mpiimpl.h\"")
                G.out.append("")

        if 'single-source' in G.opts:
            # otherwise, dump all functions in binding.c
            dump_out(G.c_dir + "/c_binding.c")

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

        abi_file_path = G.abi_dir + "/c_binding_abi.c"
        G.check_write_path(abi_file_path)
        dump_c_file(abi_file_path, G.out)

    def dump_io_funcs():
        G.out = []
        G.out.append("#include \"mpiimpl.h\"")
        G.out.append("#include \"mpir_io_impl.h\"")
        G.out.append("")

        for func in io_func_list:
            dump_func(func, do_doc)

        dump_out(G.c_dir + "/io.c")

    def dump_io_funcs_abi():
        G.out = []
        G.out.append("#include \"mpichconf.h\"")
        G.out.append("#include \"io_abi_internal.h\"")
        G.out.append("#include \"mpir_io_impl.h\"")
        G.out.append("#include <limits.h>")
        G.out.append("")

        for func in io_func_list:
            dump_func_abi(func)

        abi_file_path = G.abi_dir + "/io_abi.c"
        G.check_write_path(abi_file_path)
        dump_c_file(abi_file_path, G.out)

    def dump_f2c_funcs():
        G.out = []
        G.out.append("#include \"mpi_fortimpl.h\"")
        G.out.append("")

        for func in f2c_func_list:
            dump_func_f2c(func)
        dump_out_no_make(G.f77_dir + "/f2c.c")

    # ----
    dump_c_binding()
    dump_c_binding_abi()
    dump_io_funcs()
    dump_io_funcs_abi()
    dump_f2c_funcs()

    if do_doc:
        f = mansrc_dir + '/poly_aliases.lst'
        with open(f, "w") as Out:
            for name in G.poly_aliases:
                print("%s - %s_c" % (name, name), file=Out)

    # -- Dump other files --
    G.check_write_path("src/include")
    G.check_write_path("src/mpi_t")
    G.check_write_path("src/include/mpi_proto.h")
    dump_Makefile_mk("%s/Makefile.mk" % G.c_dir)
    dump_mpir_impl_h("src/include/mpir_impl.h")
    dump_mpir_io_impl_h("src/include/mpir_io_impl.h")
    dump_errnames_txt("%s/errnames.txt" % G.c_dir)
    dump_qmpi_register_h("src/mpi_t/qmpi_register.h")
    dump_mpi_proto_h("src/include/mpi_proto.h")
    dump_mtest_mpix_h("test/mpi/include/mtest_mpix.h")

def collect_mansrc_semantics(filepath):
    semantics = {}

    with open(filepath, 'r') as f:
        for line in f:
            if RE.match(r'\/\/ *tag::(\w+)\[\]', line):
                semantics[RE.m.group(1)] = 1
    return semantics

def collect_function_notes(filepath):
    func = None
    notes = None
    with open(filepath, 'r') as f:
        for line in f:
            if RE.match(r'(MPIX*_\w+):', line):
                func = G.FUNCS[RE.m.group(1).lower()]
                if not func:
                    raise Exception("Unknown function %s in %s" % (RE.m.group(1), filepath))
                if 'notes' in func:
                    raise Exception("Duplicated notes in %s" % func['name'])
                notes = []
                func['notes'] = notes
            elif RE.match(r'\s+\/\/ *tag:(\w+)\s*$', line):
                notes = []
                func['notes-' + RE.m.group(1)] = notes
            else:
                notes.append(line[4:])

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
