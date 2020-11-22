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
        'GREQ_CLASS': "MPIR_Grequest_class",
    }

