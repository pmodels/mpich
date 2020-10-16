#!/usr/bin/env python3

import subprocess
import sys
import json
MPI_STANDARD_PATH=sys.argv[1];
sys.path.insert(1, f"{MPI_STANDARD_PATH}/binding-tool")
import bindingtypes
from bindingc import _parameter_suppression, _emit_type_attribute, _emit_pointer_attribute, _emit_array_attribute
from typing import Any, Mapping, Optional
import re

qmpi_h_file = open("src/include/qmpi.h", "w");
qmpi_impl_h_file = open("src/include/qmpi_impl.h", "w");
enum_list = [];
register_list = [];
qmpi_h_file_text = [];
ignore_functions = [
        # Ignore removed functions
        "MPI_Address",
        "MPI_Errhandler_create",
        "MPI_Errhandler_get",
        "MPI_Errhandler_set",
        "MPI_Type_extent",
        "MPI_Type_hindexed",
        "MPI_Type_hvector",
        "MPI_Type_lb",
        "MPI_Type_struct",
        "MPI_Type_ub",

        # Temporarily ignore fortran-only functions
        "MPI_Op_f2c",
        "MPI_Request_f2c",
        "MPI_Info_f2c",
        "MPI_Status_f2c",
        "MPI_Op_c2f",
        "MPI_Request_c2f",
        "MPI_Info_c2f",
        "MPI_Status_c2f",
        "MPI_Message_c2f",
        "MPI_Comm_c2f",
        "MPI_Comm_f2c",
        "MPI_Errhandler_f2c",
        "MPI_Group_c2f",
        "MPI_Status_f082c",
        "MPI_Win_f2c",
        "MPI_Win_c2f",
        "MPI_Group_f2c",
        "MPI_Errhandler_c2f",
        "MPI_Type_c2f",
        "MPI_Type_f2c",
        "MPI_Status_c2f08",
        "MPI_Message_f2c",


        # Ignore MPI-4.0 functions for now too
        "MPI_Group_from_session_pset",
        "MPI_Session_call_errhandler",
        "MPI_Session_finalize",
        "MPI_Session_create_errhandler",
        "MPI_Session_get_errhandler",
        "MPI_Session_get_info",
        "MPI_Session_get_nth_pset",
        "MPI_Session_get_num_psets",
        "MPI_Session_get_pset_info",
        "MPI_Session_init",
        "MPI_Session_set_errhandler",
        "MPI_Allgather_init",
        "MPI_Allgatherv_init",
        "MPI_Allreduce_init",
        "MPI_Alltoall_init",
        "MPI_Alltoallv_init",
        "MPI_Alltoallw_init",
        "MPI_Barrier_init",
        "MPI_Bcast_init",
        "MPI_Comm_create_from_group",
        "MPI_COMM_DUP_FN",
        "MPI_Comm_idup_with_info",
        "MPI_COMM_NULL_COPY_FN",
        "MPI_COMM_NULL_DELETE_FN",
        "MPI_CONVERSION_FN_NULL",
        "MPI_DUP_FN",
        "MPI_Exscan_init",
        "MPI_F_sync_reg",
        "MPI_Gather_init",
        "MPI_Gatherv_init",
        "MPI_Info_create_env",
        "MPI_Info_get_string",
        "MPI_Intercomm_create_from_groups",
        "MPI_Isendrecv",
        "MPI_Isendrecv_replace",
        "MPI_Neighbor_allgather_init",
        "MPI_Neighbor_allgatherv_init",
        "MPI_Neighbor_alltoall_init",
        "MPI_Neighbor_alltoallv_init",
        "MPI_Neighbor_alltoallw_init",
        "MPI_NULL_COPY_FN",
        "MPI_NULL_DELETE_FN",
        "MPI_Parrived",
        "MPI_Pready",
        "MPI_Pready_list",
        "MPI_Pready_range",
        "MPI_Precv_init",
        "MPI_Psend_init",
        "MPI_Reduce_init",
        "MPI_Reduce_scatter_block_init",
        "MPI_Reduce_scatter_init",
        "MPI_Register_datarep",
        "MPI_Scan_init",
        "MPI_Scatter_init",
        "MPI_Scatterv_init",
        "MPI_Session_c2f",
        "MPI_Session_f2c",
        "MPI_Sizeof",
        "MPI_Status_f082f",
        "MPI_Status_f2f08",
        "MPI_T_category_get_events",
        "MPI_T_category_get_num_events",
        "MPI_T_event_callback_get_info",
        "MPI_T_event_callback_set_info",
        "MPI_T_event_copy",
        "MPI_T_event_get_index",
        "MPI_T_event_get_info",
        "MPI_T_event_get_num",
        "MPI_T_event_get_source",
        "MPI_T_event_get_timestamp",
        "MPI_T_event_handle_alloc",
        "MPI_T_event_handle_free",
        "MPI_T_event_handle_get_info",
        "MPI_T_event_handle_set_info",
        "MPI_T_event_read",
        "MPI_T_event_register_callback",
        "MPI_T_event_set_dropped_handler",
        "MPI_T_source_get_info",
        "MPI_T_source_get_num",
        "MPI_T_source_get_timestamp",
        "MPI_TYPE_DUP_FN",
        "MPI_TYPE_NULL_COPY_FN",
        "MPI_TYPE_NULL_DELETE_FN",
        "MPI_WIN_DUP_FN",
        "MPI_WIN_NULL_COPY_FN",
        "MPI_WIN_NULL_DELETE_FN",
        ]

def const_attribute(parameter: Mapping[str, Any]) -> str:
    """
    Emits the const attribute of the parameter expression.
    """

    return 'const ' if parameter['constant'] else ''

def generate_code(parseset: Mapping[str, Any], postfix: str, kind_map: Mapping[str, str]) -> None:
    # Generate the list of parameters
    parameters = []
    for parameter in parseset['parameters']:
        if not _parameter_suppression(parameter, postfix):
            parameters.append(parameter)

    # Parse out the "core" function name (remove MPI_ or MPI_T prefix)
    full_name = parseset['name'];
    regex = re.compile('MPI_(T_)*(.*)');
    match = regex.match(full_name);
    short_name = match.group(2);
    if (match.group(1)):
        qmpi_name = f"QMPI_T_{short_name}";
    else:
        qmpi_name = f"QMPI_{short_name}";

    # Generate both the typed form and the shortened form of the parameter list
    varargs = "";
    orig_params = "";
    typed_params = "QMPI_Context context, int tool_id";
    short_params = "context, tool_id";
    for param in parameters:
        # Handle a special case for MPI_Pcontrol
        if (param['kind'] == "VARARGS"):
            orig_params = (f"{orig_params}, ...");
            typed_params = (f"{typed_params}, ...");
            short_params = short_params + ", args";
            if (full_name == "MPI_Pcontrol"):
                varargs = "    va_list args;\n    va_start(args, level);"
            else:
                raise RuntimeError(f"Varargs function that isn't MPI_Pcontrol ({full_name}). Don't know what to do with this...");
        else:
            if (orig_params == ""):
                orig_params = (f"{const_attribute(param)}"
                                f"{_emit_type_attribute(param, postfix, kind_map)} "
                                f"{_emit_pointer_attribute(param)}"
                                f"{param['name']}"
                                f"{_emit_array_attribute(param)}");
            else:
                orig_params = (f"{orig_params}, "
                                f"{const_attribute(param)}"
                                f"{_emit_type_attribute(param, postfix, kind_map)} "
                                f"{_emit_pointer_attribute(param)}"
                                f"{param['name']}"
                                f"{_emit_array_attribute(param)}");

            typed_params = (f"{typed_params}, "
                            f"{const_attribute(param)}"
                            f"{_emit_type_attribute(param, postfix, kind_map)} "
                            f"{_emit_pointer_attribute(param)}"
                            f"{param['name']}"
                            f"{_emit_array_attribute(param)}");
            short_params = short_params + ", " + param['name'];

    # Generate all of the pieces of the function definition
    return_type = kind_map[parseset['return_kind']];
    function_signature = f"{return_type} {qmpi_name}({typed_params})";

    # Generate function declaration
    qmpi_h_file_text.append(f"{function_signature} MPICH_API_PUBLIC;" "\n");

    # Generate function typedef
    qmpi_h_file_text.append(f"typedef {return_type} ({qmpi_name}_t) ({typed_params});" "\n");
    qmpi_h_file_text.append('\n');

    # Add this function to enum
    enum_name = f"{parseset['name'].upper()}_T";
    enum_list.append(enum_name);

    # Add the registration of this function to the initialization function
    register_list.append(f"MPIR_QMPI_pointers[{enum_name}] = (void (*)(void)) &{qmpi_name};" "\n");

LANGUAGE_MODULES_BIG = [('', bindingtypes.BIG_C_KIND_MAP)];
LANGUAGE_MODULES_POLY = [('', bindingtypes.SMALL_C_KIND_MAP),
                         ('_l', bindingtypes.BIG_C_KIND_MAP)];

if not (sys.version_info.major >= 4 or sys.version_info.major == 3 and sys.version_info.minor >= 7):
    raise RuntimeError("This python installation is too old! Use Python 3.7 or later.");

apis_file="apis.json"

# Generate API json file
subprocess.call(f"python3.7 {MPI_STANDARD_PATH}/binding-tool/binding_prepass.py {MPI_STANDARD_PATH} {apis_file}", shell=True);

qmpi_h_file.write('/*\n * Copyright (C) by Argonne National Laboratory\n *     See COPYRIGHT in top-level directory\n */\n\n');
qmpi_h_file.write('#ifndef QMPI_H\n#define QMPI_H\n\n');

qmpi_impl_h_file.write('/*\n * Copyright (C) by Argonne National Laboratory\n *     See COPYRIGHT in top-level directory\n */\n');
qmpi_impl_h_file.write('#ifndef QMPI_IMPL_H\n#define QMPI_IMPL_H\n\n');
qmpi_impl_h_file.write('#include "mpiimpl.h"\n\n');

qmpi_impl_h_file.write("int MPII_qmpi_register_internal_functions(void);\n\n");

apis = json.load(open(apis_file));
for key in apis:
    func = apis[key];

    # Check against a list of functions to ignore
    # Also skip callback functions
    if (func['name'] in ignore_functions) or func['attributes']['callback']:
        #print("Skipping " + func['name']);
        continue;

    # Check if this function uses a POLY type
    elif any((param['kind'].startswith('POLY')) for param in func['parameters']):
        for postfix, kind_map in LANGUAGE_MODULES_POLY:
            generate_code(func, postfix, kind_map);
    else:
        for postfix, kind_map in LANGUAGE_MODULES_BIG:
            generate_code(func, postfix, kind_map);

# Add the enum to the header file
enum_list_string = (',\n    '.join(enum_list));
qmpi_h_file.write(f"enum QMPI_Functions_enum {{" "\n    "
                  f"{enum_list_string}," "\n"
                   "MPI_LAST_FUNC_T\n"
                  f"}};" "\n\n");

qmpi_h_file.write(""
        "typedef struct {\n"
        "    void *storage_stack;\n"
        "} QMPI_Context;\n"
        "\n"
        );

qmpi_h_file.write(''.join(qmpi_h_file_text));

# Add the types and registration functions to the header file
qmpi_h_file.write("#define QMPI_MAX_TOOL_NAME_LENGTH 256\n\n");
qmpi_h_file.write("int QMPI_Register_tool_name(const char *tool_name, void (* init_function_ptr)(int tool_id)) MPICH_API_PUBLIC;\n");
qmpi_h_file.write("int QMPI_Register_tool_storage(int tool_id, void *tool_storage) MPICH_API_PUBLIC;\n");
qmpi_h_file.write("int QMPI_Register_function(int calling_tool_id, enum QMPI_Functions_enum function_enum,\n"
                  "                           void (* function_ptr)(void)) MPICH_API_PUBLIC;\n");
qmpi_h_file.write("\n");
qmpi_h_file.write("#include <stddef.h>\n"
        "extern MPICH_API_PUBLIC void (**MPIR_QMPI_pointers) (void);\n"
        "extern MPICH_API_PUBLIC void **MPIR_QMPI_storage;\n"
        "\n"
        "static inline void QMPI_Get_function(int calling_tool_id, enum QMPI_Functions_enum function_enum,\n"
        "                      void (**function_ptr) (void), QMPI_Context *next_tool_context,\n"
        "                      int *next_tool_id)\n"
        "{\n"
        "    QMPI_Context context;\n"
        "    context.storage_stack = MPIR_QMPI_storage;"
        "\n"
        "    for (int i = calling_tool_id - 1; i >= 0; i--) {\n"
        "        if (MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum] != NULL) {\n"
        "            *function_ptr = MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum];\n"
        "            *next_tool_context = context;\n"
        "            *next_tool_id = i;\n"
        "            return;\n"
        "        }\n"
        "    }\n"
        "}\n"
        );

print('int MPII_qmpi_register_internal_functions(void)\n{\n');
print(''.join(register_list));
print('return 0;\n}\n');

qmpi_h_file.write('#endif /* QMPI_H */\n');
qmpi_impl_h_file.write('#endif /* QMPI_IMPL_H */\n');

qmpi_h_file.close();
qmpi_impl_h_file.close();

# Clean up APIs file
subprocess.call(f"rm -f {apis_file}", shell=True);
subprocess.call("maint/code-cleanup.bash src/include/qmpi.h", shell=True);
