##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

from local_python import MPI_API_Global as G
from local_python.binding_common import *
from local_python import RE

import re

def dump_f90_func(func):
    f90_mapping = get_kind_map('F90', False)

    f_param_list = []
    decl_list = []

    tkr_list = []  # variables that need IGNORE tkr (type-kind-rank) check
    uses = {}
    for p in func['parameters']:
        if re.search(r'suppress=.*f90_parameter', p['t']):
            continue
        if re.search(r'large_only', p['t']):
            continue
        f_param_list.append(p['name'])
        f_type = f90_mapping[p['kind']]
        if re.match(r'<type>', f_type, re.IGNORECASE):
            # TODO: configure it
            if False:
                # use assumed type
                f_type = 'TYPE(*), INTENT(IN)'
            else:
                f_type = 'REAL'
            tkr_list.append(p['name'])
        elif RE.match(r'.*(MPI_\w+_KIND)', f_type, re.IGNORECASE):
            int_kind = RE.m.group(1)
            if RE.match(r'MPI_Type_(lb|ub|extent|hvector|hindexed|struct)', func['name'], re.IGNORECASE):
                f_type = 'INTEGER'
            else:
                uses[int_kind] = 1

        if p['kind'] == 'STRING_ARRAY':
            decl = "%s :: %s(*)" % (f_type, p['name'])
        elif p['kind'] == 'STRING_2DARRAY':
            if re.match(r'mpi_comm_spawn_multiple', func['name'], re.IGNORECASE):
                decl = "%s :: %s(count, *)" % (f_type, p['name'])
            else:
                raise Exception("Unexpected")
        elif p['length'] is not None:
            if re.match(r'CHARACTER.*\*', f_type, re.IGNORECASE):
                decl = "%s :: %s" % (f_type, p['name'])
            elif isinstance(p['length'], list):
                # assume [n, 3] as ranges in MPI_Group_range_excl
                decl = "%s :: %s(%s, *)" % (f_type, p['name'], p['length'][1])
            elif p['length']:
                decl = "%s :: %s(%s)" % (f_type, p['name'], p['length'])
            else:
                decl = "%s :: %s(*)" % (f_type, p['name'])
        elif p['kind'] == 'STATUS':
            uses['MPI_STATUS_SIZE'] = 1
            decl = "INTEGER :: %s(MPI_STATUS_SIZE)" % (p['name'])
        else:
            decl = "%s :: %s" % (f_type, p['name'])
        decl_list.append(decl)

    def dump_uses():
        G.out.append("USE MPI_CONSTANTS, ONLY: %s" % ', '.join(uses.keys()))

    def dump_ignore_tkr():
        tkr_vars = ', '.join(tkr_list)
        if G.opts['ignore-tkr'] == 'gcc':  # e.g. gfort
            # gfortran since 4.9
            G.out.append("!GCC$ ATTRIBUTES NO_ARG_CHECK :: " + tkr_vars)
        elif G.opts['ignore-tkr'] == 'dec': # e.g. ifort
            G.out.append("!DEC$ ATTRIBUTES NO_ARG_CHECK :: " + tkr_vars)
        elif G.opts['ignore-tkr'] == 'pragma': # e.g. sunfort
            G.out.append("!$PRAGMA IGNORE_TKR " + tkr_vars)
        elif G.opts['ignore-tkr'] == 'dir': # e.g. flang
            G.out.append("!DIR$ IGNORE_TKR " + tkr_vars)
        elif G.opts['ignore-tkr'] == 'ibm': # e.g. IBM
            G.out.append("!IBM* IGNORE_TKR " + tkr_vars)
        elif G.opts['ignore-tkr'] == 'assumed': # Assumed type and rank
            G.out.append("TYPE(*), DIMENSION(..) :: " + tkr_vars)
        else:
            raise Exception("Unrognized tkr options: %s" % G.opts['ignore-tkr'])

    if tkr_list and 'ignore-tkr' not in G.opts:
        # skip routines with choice buffers unless we can ignore TKR check
        return

    func_name = get_function_name(func)
    G.out.append("")
    if 'return' not in func:
        if not len(f_param_list) or not RE.match(r'ierr(or)?', f_param_list[-1]):
            f_param_list.append('ierr')
            decl_list.append("INTEGER :: ierr")
        dump_fortran_line("SUBROUTINE %s(%s)" % (func_name, ', '.join(f_param_list)))
    else:
        dump_fortran_line("FUNCTION %s(%s) result(res)" % (func_name, ', '.join(f_param_list)))
        ret_type = f90_mapping[func['return']]
        decl_list.append("%s :: res" % ret_type)
    G.out.append("INDENT")
    if uses:
        dump_uses()
    G.out.append("IMPLICIT NONE")
    if tkr_list:
        dump_ignore_tkr()
    G.out.extend(decl_list)
    G.out.append("DEDENT")
    if 'return' not in func:
        G.out.append("END SUBROUTINE %s" % func_name)
    else:
        G.out.append("END FUNCTION %s" % func_name)

def dump_f90_constants():
    def get_op_procname(a, op):
        if RE.match(r'MPIX?_(\w+)', a):
            a = RE.m.group(1)
        if op == '.EQ.':
            return a.lower() + 'eq'
        elif op == '.NE.':
            return a.lower() + 'neq'
        else:
            raise Exception("Unrecognized op: %s" % op)

    # ----------------------------------
    G.out.append("INCLUDE 'mpifnoext.h'")

    G.out.append("")
    dump_F_type_open('MPI_Status')
    for field in G.status_fields:
        G.out.append("INTEGER :: %s" % field)
    dump_F_type_close('MPI_Status')

    for a in G.handle_list:
        G.out.append("")
        dump_F_type_open("%s" % a)
        G.out.append("INTEGER :: MPI_VAL")
        dump_F_type_close("%s" % a)

    for op in ['.EQ.', '.NE.']:
        G.out.append("")
        G.out.append("INTERFACE OPERATOR(%s)" % op)
        for a in G.handle_list:
            G.out.append("    MODULE PROCEDURE %s" % get_op_procname(a, op))
        G.out.append("END INTERFACE")

    G.out.append("")
    G.out.append("CONTAINS")
    for op in ['.EQ.', '.NE.']:
        for a in G.handle_list:
            procname = get_op_procname(a, op)
            G.out.append("")
            G.out.append("LOGICAL FUNCTION %s(lhs, rhs)" % procname)
            G.out.append("    TYPE(%s), INTENT(IN) :: lhs, rhs" % a)
            G.out.append("    %s = lhs%%MPI_VAL %s rhs%%MPI_VAL" % (procname, op))
            G.out.append("END FUNCTION %s" % procname)


def dump_f90_sizeofs():
    # deprecated in MPI-4, replaced by Fortran intrinsic c_sizeof() and storage_size()
    types = {}  # list of types we support
    types['CH1'] = "CHARACTER"
    types["L%d" % int(G.opts['f-logical-size'])] = "LOGICAL"
    # NOTE: we assume the fixed-size types are available. The alternative is to use
    #       integer kind and real kind. MPI_SIZEOF is deprecated. We'll keep it simple
    #       until we encounter compilers doesn't support fixed-size types.
    types['I1'] = "INTEGER*1"
    types['I2'] = "INTEGER*2"
    types['I4'] = "INTEGER*4"
    types['I8'] = "INTEGER*8"
    types['R4'] = "REAL*4"
    types['R8'] = "REAL*8"
    types['CX8'] = "COMPLEX*8"
    types['CX16'] = "COMPLEX*16"

    G.out.append("PUBLIC :: MPI_SIZEOF")
    G.out.append("INTERFACE MPI_SIZEOF")
    for k in types.keys():
        G.out.append("    MODULE PROCEDURE MPI_SIZEOF_%s" % k)
        G.out.append("    MODULE PROCEDURE MPI_SIZEOF_%sV" % k)
    G.out.append("END INTERFACE")
    G.out.append("")
    G.out.append("CONTAINS")
    for k, v in types.items():
        if RE.match(r'[A-Z]+(\d+)', k):
            n = int(RE.m.group(1))
        # Scalar
        G.out.append("")
        G.out.append("SUBROUTINE MPI_SIZEOF_%s(X, SIZE, IERRROR)" %k)
        G.out.append("    %s :: X" % v)
        G.out.append("    INTEGER :: SIZE, IERRROR")
        G.out.append("    SIZE = %d" % n)
        G.out.append("    IERRROR = 0")
        G.out.append("END SUBROUTINE MPI_SIZEOF_%s" % k)
        # Array
        G.out.append("")
        G.out.append("SUBROUTINE MPI_SIZEOF_%sV(X, SIZE, IERRROR)" %k)
        G.out.append("    %s :: X(*)" % v)
        G.out.append("    INTEGER :: SIZE, IERRROR")
        G.out.append("    SIZE = %d" % n)
        G.out.append("    IERRROR = 0")
        G.out.append("END SUBROUTINE MPI_SIZEOF_%sV" % k)

#---------------------------------------- 
def check_func_directives(func):
    if 'dir' in func and func['dir'] == "mpit":
        func['_skip_fortran'] = 1
    elif RE.match(r'mpix_(grequest_|type_iov)', func['name'], re.IGNORECASE):
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_attr_', func['name'], re.IGNORECASE):
        func['_skip_fortran'] = 1
    elif RE.match(r'.*_function', func['name'], re.IGNORECASE):
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_pcontrol', func['name'], re.IGNORECASE):
        func['_skip_fortran'] = 1
    elif RE.match(r'mpi_\w+_(f|f08|c)2(f|f08|c)$', func['name'], re.IGNORECASE):
        # implemented in mpi_f08_types.f90
        func['_skip_fortran'] = 1

#---------------------------------------- 
def dump_F_module_open(name):
    G.out.append("MODULE %s" % name)
    G.out.append("INDENT")
    G.out.append("IMPLICIT NONE")

def dump_F_module_close(name):
    G.out.append("DEDENT")
    G.out.append("END MODULE %s" % name)

def dump_F_type_open(name):
    G.out.append("TYPE :: %s" % name)
    G.out.append("INDENT")
    G.out.append("SEQUENCE")

def dump_F_type_close(name):
    G.out.append("DEDENT")
    G.out.append("END TYPE %s" % name)

def dump_fortran_line(s):
    tlist = split_line_with_break(s, '', 100)
    n = len(tlist)
    if n > 1:
        for i in range(n-1):
            tlist[i] = tlist[i] + ' &'
    G.out.extend(tlist)

def dump_f90_file(f, lines):
    print("  --> [%s]" % f)
    with open(f, "w") as Out:
        for l in G.copyright_f90:
            print(l, file=Out)
        indent = 0
        for l in lines:
            if RE.match(r'INDENT', l):
                indent += 1
            elif RE.match(r'DEDENT', l):
                indent -= 1
            else:
                if indent > 0:
                    print("    " * indent, end='', file=Out)
                print(l, file=Out)

