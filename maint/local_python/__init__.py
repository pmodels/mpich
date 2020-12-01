##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import re

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
    out = []
    FUNCS = {}
    MAPS = {}
    default_descriptions = {}
    err_codes = {}
    mpi_sources = []
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
        # 'SESSION': "",
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
