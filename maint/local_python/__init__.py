##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import os
import re
import sys

def get_srcdir():
    m = re.match(r'(.*)\/maint\/local_python', __file__)
    if m:
        return m.group(1)
    elif re.match(r'maint\/local_python', __file__):
        return "."
    else:
        raise Exception("Can't determine srcdir from __file__")

# RE class allows convenience of using regex capture in a condition
class RE:
    m = None
    def match(pat, str, flags=0):
        RE.m = re.match(pat, str, flags)
        return RE.m
    def search(pat, str, flags=0):
        RE.m = re.search(pat, str, flags)
        return RE.m

# Global data used across modules
class MPI_API_Global:
    srcdir = get_srcdir()

    def get_srcdir_path(path):
        # assume path is a relative path from top srcdir
        return MPI_API_Global.srcdir + "/" + path

    def check_write_path(path):
        os.makedirs(os.path.dirname(path), exist_ok=True)

    # command line options and arguments
    # By default assumes sizes for LP64 model.
    # The F08 bindings use the sizes to detect duplicate large interfaces
    opts = {'fint-size':4, 'aint-size':8, 'count-size':8, 'cint-size':4}

    args = []
    # output
    out = []
    # api
    FUNCS = {}
    MAPS = {}
    default_descriptions = {}
    # misc globals used during code generation
    err_codes = {}
    mpi_sources = []
    mpi_declares = []
    impl_declares = []
    mpi_errnames = []

    handle_mpir_types = {
        'COMMUNICATOR': "MPIR_Comm",
        'GROUP': "MPIR_Group",
        'DATATYPE': "MPIR_Datatype",
        'ERRHANDLER': "MPIR_Errhandler",
        'OPERATION': "MPIR_Op",
        'INFO': "MPIR_Info",
        'WINDOW': "MPIR_Win",
        'KEYVAL': "MPII_Keyval",
        'REQUEST': "MPIR_Request",
        'MESSAGE': "MPIR_Request",
        'SESSION': "MPIR_Session",
        'GREQUEST_CLASS': "MPIR_Grequest_class",
    }

    handle_error_codes = {
        'COMMUNICATOR': "MPI_ERR_COMM",
        'GROUP': "MPI_ERR_GROUP",
        'DATATYPE': "MPI_ERR_TYPE",
        'ERRHANDLER': "MPI_ERR_ARG",
        'OPERATION': "MPI_ERR_OP",
        'INFO': "MPI_ERR_INFO",
        'WINDOW': "MPI_ERR_WIN",
        'KEYVAL': "MPI_ERR_KEYVAL",
        'REQUEST': "MPI_ERR_REQUEST",
        'MESSAGE': "MPI_ERR_REQUEST",
        'SESSION': "MPI_ERR_SESSION",
        'GREQUEST_CLASS': "",
    }

    handle_out_do_ptrs = {
        'COMMUNICATOR': 1,
        'GROUP': 1,
        'DATATYPE': 1,
        'ERRHANDLER': 1,
        'OPERATION': 1,
        'INFO': 1,
        'WINDOW': 1,
        # 'KEYVAL': 1,
        'REQUEST': 1,
        'MESSAGE': 1,
        'SESSION': 1,
        'GREQUEST_CLASS': 1,
    }

    handle_NULLs = {
        'COMMUNICATOR': "MPI_COMM_NULL",
        'GROUP': "MPI_GROUP_NULL",
        'DATATYPE': "MPI_DATATYPE_NULL",
        'ERRHANDLER': "MPI_ERRHANDLER_NULL",
        'OPERATION': "MPI_OP_NULL",
        'INFO': "MPI_INFO_NULL",
        'WINDOW': "MPI_WIN_NULL",
        # 'KEYVAL': "",
        'REQUEST': "MPI_REQUEST_NULL",
        'MESSAGE': "MPI_MESSAGE_NULL",
        'SESSION': "MPI_SESSION_NULL",
        # 'GREQUEST_CLASS': "",
    }

    copyright_c = [
        "/*",
        " * Copyright (C) by Argonne National Laboratory",
        " *     See COPYRIGHT in top-level directory",
        " */",
        "",
        "/* -- THIS FILE IS AUTO-GENERATED -- */",
        ""
    ]

    copyright_mk = [
        "##",
        "## Copyright (C) by Argonne National Laboratory",
        "##     See COPYRIGHT in top-level directory",
        "##",
        "",
        "# -- THIS FILE IS AUTO-GENERATED -- ",
        ""
    ]

    copyright_f90 = [
        "!",
        "! Copyright (C) by Argonne National Laboratory",
        "!     See COPYRIGHT in top-level directory",
        "!",
        "",
        "! -- THIS FILE IS AUTO-GENERATED -- ",
        ""
    ]

    def parse_cmdline():
        for a in sys.argv[1:]:
            if RE.match(r'--?([\w-]+)=(.*)', a):
                MPI_API_Global.opts[RE.m.group(1)] = RE.m.group(2)
            elif RE.match(r'--?([\w-].+)', a):
                MPI_API_Global.opts[RE.m.group(1)] = 1
            else:
                MPI_API_Global.args.append(a)
