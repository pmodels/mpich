##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import RE
from local_python import MPI_API_Global as G

import sys
import re
import copy
import os
import glob

def load_C_func_list(binding_dir="src/binding", silent=False):
    # -- Loading Standard APIs --
    if os.path.exists("%s/apis.json" % binding_dir):
        if not silent: print("Loading %s/apis.json ..." % binding_dir)
        load_mpi_json("%s/apis.json" % binding_dir)
    else:
        if not silent: print("Loading %s/mpi_standard_api.txt ..." % binding_dir)
        load_mpi_api("%s/mpi_standard_api.txt" % binding_dir)

    if not silent: print("Loading %s/apis_mapping.txt ..." % binding_dir)
    load_mpi_mapping("%s/apis_mapping.txt" % binding_dir)
    if not silent: print("Loading %s/custom_mapping.txt ..." % binding_dir)
    load_mpi_mapping("%s/custom_mapping.txt" % binding_dir)

    # -- Loading MPICH APIs --

    api_files = glob.glob("%s/c/*_api.txt" % binding_dir)
    for f in api_files:
        if RE.match(r'.*\/(\w+)_api.txt', f):
            # The name in eg pt2pt_api.txt indicates the output folder.
            # Only the api functions with output folder will get generated.
            # This allows simple control of what functions to generate.
            if not silent: print("Loading %s ..." % f)
            load_mpi_api(f, RE.m.group(1))

    # -- filter and sort func_list --
    func_list = []
    for f in G.FUNCS.values():
        if 'not_implemented' in f:
            if not silent: print("    skip %s (not_implemented)" % f['name'])
        elif RE.match(r'\w+_(function|FN)$', f['name']):
            # skip various callback functions
            continue
        elif not 'dir' in f:
            if not silent: print("    skip %s (not defined)" % f['name'])
        else:
            func_list.append(f)
    func_list.sort(key = lambda f: f['dir'])

    load_mpix_txt("%s/mpix.txt" % binding_dir)

    return func_list

def load_mpi_json(api_json):
    import json
    with open(api_json) as In:
        G.FUNCS = json.load(In)

def load_mpi_mapping(api_mapping_txt):
    cur_map, cur_name = '', ''
    stage = ''
    with open(api_mapping_txt, "r") as In:
        for line in In:
            # -- stage header --
            if RE.match(r'(\w+_KIND_MAP):', line):
                name = RE.m.group(1)
                stage = "MAP"
                cur_name = name
                if name not in G.MAPS:
                    cur_map = {'_name': name}
                    G.MAPS[name] = cur_map
                else:
                    cur_map = G.MAPS[name]
            elif RE.match(r'Default Descriptions', line):
                stage = "default_descriptions"
                cur_name = "Default Descriptions"
            # -- per-stage parsing --
            elif stage == "MAP":
                if RE.match(r'\s+\.base:\s*(\w+)', line):
                    name = RE.m.group(1)
                    cur_map = copy.deepcopy(G.MAPS[name])
                    cur_map['_name'] = cur_name
                    G.MAPS[cur_name] = cur_map
                elif RE.match(r'\s+(\w+):\s*(.*)', line):
                    name, param_type = RE.m.group(1, 2)
                    cur_map[name] = param_type
            elif stage == "default_descriptions":
                if RE.match(r'\s*(\w+):\s*(.*)', line):
                    key, val = RE.m.group(1, 2)
                    G.default_descriptions[key] = val

def load_mpix_txt(mpix_txt):
    G.mpix_symbols = {}
    stage = "functions"
    if os.path.exists(mpix_txt):
        with open(mpix_txt, "r") as In:
            for line in In:
                if RE.match(r'#\s*mpi.h\s+symbols', line):
                    stage = "symbols"
                if RE.match(r'(MPI_\w+)', line):
                    name = RE.m.group(1)
                    G.mpix_symbols[name] = stage
                    if stage == "functions":
                        G.FUNCS[name.lower()]['mpix'] = 1

# If gen_in_dir is given, all functions loaded in api_txt are candidates 
# for binding generation. This allows fine-grain binding control.
def load_mpi_api(api_txt, gen_in_dir=""):
    """Load mpi standard api into global (G) lists and dictionaries."""
    cur_func, cur_name = '', ''
    stage = ''
    with open(api_txt, "r") as In:
        for line in In:
            # -- stage header --
            if RE.match(r'(MPI\w+):\s*(.*)', line):
                name, attr = RE.m.group(1, 2)
                key = name.lower()
                stage = "FUNC"
                cur_name = name
                if key in G.FUNCS:
                    cur_func = G.FUNCS[key]
                    if RE.search(r'not_implemented', attr):
                        cur_func['not_implemented'] = True
                else:
                    cur_func = {'name': name, 'parameters': [], 'attrs': attr, 'desc': ""}
                    G.FUNCS[key] = cur_func
                if gen_in_dir:
                    cur_func['dir'] = gen_in_dir
            elif RE.match(r'(\w+)', line):
                print("Unexpected leading word [%s] in %s" % (RE.m.group(1), api_txt), file=sys.stderr)
                # anything with unexpected unindented word resets stage
                stage=''
            # -- per-stage parsing --
            elif stage == "FUNC":
                if RE.match(r'\s+\.(\w+):\s*(.*)', line):
                    key, val = RE.m.group(1, 2)
                    cur_func[key] = val
                elif RE.match(r'\s+(\w+):\s*(\w+)(.*)', line):
                    name, kind, t = RE.m.group(1, 2, 3)
                    if name == 'index':
                        # avoid -Wshadow warning
                        name = 'indx'
                    p = {'name': name, 'kind': kind}
                    if RE.match(r'(.*),\s*\[(.*)\]\s*$', t):
                        t, p['desc'] = RE.m.group(1, 2)
                    p['t'] = t
                    # we include all extra attributes in a 't' string for flexibity
                    # we'll parse the common fields also to improve code readability
                    parse_param_attributes(p)
                    cur_func['parameters'].append(p)
                elif RE.match(r'{\s*-+\s*(\w+)\s*-+(.*)', line):
                    stage = "code-"+RE.m.group(1)
                    if stage not in cur_func:
                        cur_func[stage] = []
                    # "error_check" may include list of parameters checked
                    # "handle_ptr" may include list of parameters converted
                    cur_func[stage + "-tail"] = RE.m.group(2).replace(' ','').split(',')
                elif RE.match(r'{', line):
                    stage = "FUNC-body"
                    if 'body' not in cur_func:
                        cur_func['body'] = []
                elif RE.match(r'\/\*', line):
                    # man page notes
                    stage = "FUNC-notes"
                    # 'notes' and 'notes2' goes before and after auto-generated notes
                    if RE.match(r'\/\*\s*-+\s*notes-2\s*-+', line):
                        cur_func['notes2'] = []
                    elif 'notes' not in cur_func:
                        cur_func['notes'] = []
                    else:
                        cur_func['notes2'] = []
            elif stage == "FUNC-body":
                if RE.match(r'}', line):
                    stage = "FUNC"
                else:
                    line = re.sub(r'^    ', '', line)
                    cur_func['body'].append(line)
            elif RE.match(r'(code-\w+)', stage):
                if RE.match(r'}', line):
                    stage = "FUNC"
                else:
                    line = re.sub(r'^    ', '', line)
                    cur_func[stage].append(line)
            elif stage == "FUNC-notes":
                if RE.match(r'\*\/', line):
                    stage = "FUNC"
                else:
                    line = re.sub(r'^    ', '', line)
                    if 'notes2' in cur_func:
                        cur_func['notes2'].append(line)
                    else:
                        cur_func['notes'].append(line)

def parse_param_attributes(p):
    """Parse the parameter attribute string and populate common fields"""
    # this filter function should result in identical results as the JSON
    if RE.search(r'direction\s*=\s*out', p['t'], re.IGNORECASE):
        p['param_direction'] = 'out'
    elif RE.search(r'direction\s*=\s*inout', p['t'], re.IGNORECASE):
        p['param_direction'] = 'inout'
    else:
        p['param_direction'] = 'in'

    if RE.search(r'length\s*=\s*\[(.*)\]', p['t']):
        # only the case of MPI_Group_range_{excl,incl} where length=[n, 3]
        p['length'] = RE.m.group(1).replace(' ', '').split(',')
    elif RE.search(r'length\s*=\s*([^,\s]*)', p['t']):
        p['length'] = RE.m.group(1)
    else:
        p['length'] = None

    if RE.search(r'large_only', p['t']):
        p['large_only'] = True
    else:
        p['large_only'] = False

    if RE.search(r'pointer\s*=\s*True', p['t']):
        p['pointer'] = True
    elif RE.search(r'pointer\s*=\s*False', p['t']):
        p['pointer'] = False
    else:
        p['pointer'] = None

    if RE.search(r'suppress=.*c_parameter', p['t']):
        p['suppress'] = "c_parameter"
    else:
        p['suppress'] = ''

    if RE.search(r'func_type\s*=\s*(\w+)', p['t']):
        p['func_type'] = RE.m.group(1)
    else:
        p['func_type'] = ''

    if RE.search(r'constant\s*=\s*True', p['t']):
        p['constant'] = True
    else:
        p['constant'] = False

    if RE.search(r'asynchronous\s*=\s*True', p['t']):
        p['asynchronous'] = True
    else:
        p['asynchronous'] = False

# FIXME: until ROMIO interface are generated
def get_mpiio_func_list():
    io_func_name_list = [
        "MPI_File_c2f",
        "MPI_File_close",
        "MPI_File_delete",
        "MPI_File_errhandler_function",
        "MPI_File_f2c",
        "MPI_File_get_amode",
        "MPI_File_get_atomicity",
        "MPI_File_get_byte_offset",
        "MPI_File_get_group",
        "MPI_File_get_info",
        "MPI_File_get_position",
        "MPI_File_get_position_shared",
        "MPI_File_get_size",
        "MPI_File_get_type_extent",
        "MPI_File_get_view",
        "MPI_File_iread",
        "MPI_File_iread_all",
        "MPI_File_iread_at",
        "MPI_File_iread_at_all",
        "MPI_File_iread_shared",
        "MPI_File_iwrite",
        "MPI_File_iwrite_all",
        "MPI_File_iwrite_at",
        "MPI_File_iwrite_at_all",
        "MPI_File_iwrite_shared",
        "MPI_File_open",
        "MPI_File_preallocate",
        "MPI_File_read",
        "MPI_File_read_all",
        "MPI_File_read_all_begin",
        "MPI_File_read_all_end",
        "MPI_File_read_at",
        "MPI_File_read_at_all",
        "MPI_File_read_at_all_begin",
        "MPI_File_read_at_all_end",
        "MPI_File_read_ordered",
        "MPI_File_read_ordered_begin",
        "MPI_File_read_ordered_end",
        "MPI_File_read_shared",
        "MPI_File_seek",
        "MPI_File_seek_shared",
        "MPI_File_set_atomicity",
        "MPI_File_set_info",
        "MPI_File_set_size",
        "MPI_File_set_view",
        "MPI_File_sync",
        "MPI_File_write",
        "MPI_File_write_all",
        "MPI_File_write_all_begin",
        "MPI_File_write_all_end",
        "MPI_File_write_at",
        "MPI_File_write_at_all",
        "MPI_File_write_at_all_begin",
        "MPI_File_write_at_all_end",
        "MPI_File_write_ordered",
        "MPI_File_write_ordered_begin",
        "MPI_File_write_ordered_end",
        "MPI_File_write_shared",
        "MPI_Register_datarep"
    ]
    return [G.FUNCS[a.lower()] for a in io_func_name_list]

def get_type_create_f90_func_list():
    type_func_name_list = [
        "MPI_Type_create_f90_integer",
        "MPI_Type_create_f90_real",
        "MPI_Type_create_f90_complex",
    ]
    return [G.FUNCS[a.lower()] for a in type_func_name_list]

def get_f77_dummy_func_list():
    dummy_func_name_list = [
        "MPI_DUP_FN",
        "MPI_NULL_COPY_FN",
        "MPI_NULL_DELETE_FN",
        "MPI_COMM_DUP_FN",
        "MPI_COMM_NULL_COPY_FN",
        "MPI_COMM_NULL_DELETE_FN",
        "MPI_TYPE_DUP_FN",
        "MPI_TYPE_NULL_COPY_FN",
        "MPI_TYPE_NULL_DELETE_FN",
        "MPI_WIN_DUP_FN",
        "MPI_WIN_NULL_COPY_FN",
        "MPI_WIN_NULL_DELETE_FN",
        "MPI_CONVERSION_FN_NULL",
    ]
    return [G.FUNCS[a.lower()] for a in dummy_func_name_list]
