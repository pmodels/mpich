##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import RE
import os

def collect_info_hint_blocks(root_dir):
    """Collect INFO_HINT_BLOCKS from source files and return a function-name-keyed dictionary"""
    infos_by_funcname = {}

    files = []
    for dirpath, dirnames, filenames in os.walk(root_dir):
        # sorted, so that the generated code does not depend on the filesystem
        dirnames.sort()
        for name in sorted(filenames):
            if name.endswith(('.c', '.h')):
                path = os.path.join(dirpath, name)
                try:
                    with open(path, errors='replace') as In:
                        if 'BEGIN_INFO_HINT_BLOCK' in In.read():
                            files.append(path)
                except OSError:
                    # e.g. a dangling symlink; grep skipped those as well
                    pass
    for f in files:
        infos = parse_info_block(f)
        for info in infos:
            for funcname in info['functions'].replace(' ', '').split(','):
                if funcname not in infos_by_funcname:
                    infos_by_funcname[funcname] = []
                infos_by_funcname[funcname].append(info)
    return infos_by_funcname

def parse_info_block(f):
    """Parse a source file with INFO_HINT_BLOCKs, and return a list of info hints"""
    hints = []
    info = None # loop variable
    stage = 0
    with open(f) as In:
        for line in In:
            if line.startswith("=== BEGIN_INFO_HINT_BLOCK ==="):
                stage = 1
            elif line.startswith("=== END_INFO_HINT_BLOCK ==="):
                stage = 0
            elif stage:
                if RE.match(r'\s*-\s*name\s*:\s*(\w+)', line):
                    info = {"name":RE.m.group(1)}
                    hints.append(info)
                elif RE.match(r'\s*(functions|type|default)\s*:\s*(.*)', line):
                    info[RE.m.group(1)] = RE.m.group(2)
                elif RE.match(r'\s*description\s*:\s*>-', line):
                    info['description'] = ""
                elif RE.match(r'\s+(\S.+)', line):
                    info['description'] += RE.m.group(1) + ' '
                else:
                    info = None
    return hints
