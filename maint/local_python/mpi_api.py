##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import RE
from local_python import MPI_API_Global as G

import re
import copy

# If gen_in_dir is given, all functions loaded in api_txt are candidates 
# for binding generation. This allows fine-grain binding control.
def load_mpi_api(api_txt, gen_in_dir=""):
    """Load mpi standard api into global (G) lists and dictionaries."""
    see_also_idx = 0
    cur_func, cur_map, cur_name = '', '', ''
    stage = ''
    with open(api_txt, "r") as In:
        for line in In:
            # -- stage header --
            if RE.match(r'(MPI\w+):\s*(.*)', line):
                name, attr = RE.m.group(1, 2)
                stage = "FUNC"
                cur_name = name
                if name in G.FUNCS:
                    cur_func = G.FUNCS[name]
                    if attr:
                        cur_func['attrs'] = attr
                else:
                    cur_func = {'name': name, 'params': [], 'attrs': attr, 'desc': ""}
                    G.FUNCS[name] = cur_func
                if gen_in_dir:
                    cur_func['dir'] = gen_in_dir
            elif RE.match(r'(\w+_KIND_MAP):', line):
                name = RE.m.group(1)
                stage = "MAP"
                cur_name = name
                cur_map = {'_name': name}
                G.MAPS[name] = cur_map
            elif RE.match(r'Default Descriptions', line):
                stage = "default_descriptions"
                cur_name = "Default Descriptions"
            # -- per-stage parsing --
            elif stage == "FUNC":
                if RE.match(r'\s+\.(\w+):\s*(.*)', line):
                    key, val = RE.m.group(1, 2)
                    cur_func[key] = val
                elif RE.match(r'\s+(\w+):\s*(\w+)(.*)', line):
                    name, kind, t = RE.m.group(1, 2, 3)
                    p = {'name': name, 'kind': kind}
                    if RE.match(r'(.*),\s*\[(.*)\]\s*$', t):
                        t, p['desc'] = RE.m.group(1, 2)
                    p['t'] = t
                    cur_func['params'].append(p)
                elif RE.match(r'{', line):
                    stage = "FUNC-body"
                    if 'body' not in cur_func:
                        cur_func['body'] = []
                elif RE.match(r'\/\*', line):
                    stage = "FUNC-notes"
                    if 'notes' not in cur_func:
                        cur_func['notes'] = []
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
            elif stage == "FUNC-body":
                if RE.match(r'}', line):
                    stage = "FUNC"
                else:
                    line = re.sub(r'^    ', '', line)
                    cur_func['body'].append(line)
            elif stage == "FUNC-notes":
                if RE.match(r'\*\/', line):
                    stage = "FUNC"
                else:
                    line = re.sub(r'^    ', '', line)
                    cur_func['notes'].append(line)

