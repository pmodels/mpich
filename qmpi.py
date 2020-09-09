#!/usr/bin/env python3

import subprocess
import re

result = subprocess.run("git grep --name-only --ignore-case 'MPI_[a-zA-Z0-9_]*(' src/mpi src/mpi_t src/binding", stdout=subprocess.PIPE, shell=True).stdout.decode('utf-8')
for filename in result.split():
    if "/test/" in filename:
        continue;
    elif ".f90" in filename:
        continue;
    elif ".f08" in filename:
        continue;
    elif "/adio/" in filename:
        continue;
    elif "src/binding/cxx/buildiface" in filename:
        continue;
    elif "buildiface" in filename:
        continue;
    elif "/romio/mpi2-other" in filename:
        continue;
    elif "/romio/mpi-io/fortran" in filename:
        continue;
    elif "/romio/mpi-io/register_datarep.c" in filename:
        continue;
    elif "/romio/mpi-io/glue" in filename:
        continue;
    elif "/src/mpi/datatype/typerep" in filename:
        continue;
    elif "/src/mpi/debugger" in filename:
        continue;
    elif "src/mpi/errhan/errutil.c" in filename:
        continue;
    elif "src/mpi/errhan/errhandler_create.c" in filename:
        continue;
    elif "src/mpi/errhan/errhandler_get.c" in filename:
        continue;
    elif "src/mpi/errhan/errhandler_set.c" in filename:
        continue;
    elif "src/mpi/romio/include/mpio.h.in" in filename:
        continue;
    elif "src/mpi/datatype/address.c" in filename:
        continue;
    elif "src/mpi/datatype/type_create_pairtype.c" in filename:
        continue;
    elif "src/mpi/datatype/type_extent.c" in filename:
        continue;
    elif "src/mpi/datatype/type_hindexed.c" in filename:
        continue;
    elif "src/mpi/datatype/type_hvector.c" in filename:
        continue;
    elif "src/mpi/datatype/type_lb.c" in filename:
        continue;
    elif "src/mpi/datatype/type_struct.c" in filename:
        continue;
    elif "src/mpi/datatype/type_ub.c" in filename:
        continue;
    elif "src/mpi_t/mpit.c" in filename:
        continue;

    #print("Trying file: " + filename);

    # Find last instance of MPI_ and replace it with QMPI_
    file = open(filename, "r");
    lines = file.readlines();
    newlines = [];
    matched = 0;
    indent = 0;
    end_char = '';
    inside_weak_block = 0;
    signature = "";
    short_params = "";
    function_name = "";
    indent = '                     '; # 21 space indent for newlines inside wrapper function
    newlined = False;
    for line in reversed(lines):
        #print("Reading line: " + line);
        if ("/*@" in line and matched):
            short_params = short_params.replace(')', '');
            # Insert QMPI wrapper function
            newlines.append(line);
            newlines.append("\n");
            newlines.append("}\n");
            if (function_name == "MPI_Pcontrol"):
                newlines.append(f"    return (*fn_ptr)(context, level, args);" "\n");
            else:
                newlines.append(f"    return (*fn_ptr)(context{short_params});" "\n");
            newlines.append(f"    QMPI_Get_function(MPIR_QMPI_num_tools + 1, {function_name.upper()}_T, ((void (**)(void)) &fn_ptr), &context);" "\n");
            if (function_name == "MPI_Pcontrol"):
                newlines.append("    va_list args;\n    va_start(args, level);\n");
            newlines.append(f"        return Q{function_name}(context{short_params});" "\n");
            newlines.append("    if (MPIR_QMPI_num_tools == 0)" "\n");
            newlines.append("    void *context;\n");
            newlines.append(f"    Q{function_name}_t *fn_ptr;" "\n");
            newlines.append("{\n");
            newlines.append(signature);
            continue;

        if (matched):
            newlines.append(line);
        else:
            if (line != "\n"):
                if (line[-2] == ';' or line[-2] == '{'):
                    end_char = line[-2];

            regex = re.compile('(.*)(MPI_(T_)*([a-zA-Z0-9_]*)\()(.*)$');
            match = regex.match(line);
            #if (match):
            #    print("MATCHED LINE: " + line);
            #    print("): " + str((line.find(')') == len(line) - 2 or line.find(')') == -1)))
            #    print("END CHAR: " + end_char)
            #    print("IS UPPER: " + str(match.group(2).isupper()));

            if (match and (line.find(')') == len(line) - 2 or line.find(')') == -1) and
                    end_char == '{' and match.group(1)[-1] != 'Q' and not match.group(2).isupper()):
                function_name = match.group(2)[:-1];
                if (match.group(5) == "void)"):
                    replacement = f"{match.group(1)}Q{match.group(2)}void *context)" + "\n";
                    signature   = f"{match.group(1)}{match.group(2)}void)" + "\n";
                else:
                    replacement = f"{match.group(1)}Q{match.group(2)}void *context, {match.group(5)}" + "\n";
                    signature   = f"{match.group(1)}{match.group(2)}{match.group(5)}" + "\n";
                    for arg in match.group(5).split(','):
                        if (arg.strip() == ""):
                            continue;
                        tokens = arg.split();
                        if (len(short_params) + len(tokens[-1].replace('*', '')) + 2 + 21 > 100 and not newlined):
                            short_params = short_params + ",\n" + indent;
                            newlined = True;
                        short_params = f"{short_params}, {tokens[-1].replace('*', '').replace('[3]', '').replace('[]', '')}";

                newlines.append(replacement);
                matched = 1;
                if (line[-2] != ')' and line[-2] != ',' and match.group(5) != "void)"):
                    print("Invalid line entry: " + line);
                if (line[-2] != ')'):
                    back_index = -2;
                    while True:
                        if (newlines[back_index][-2] != ')' and newlines[back_index][-2] != ','):
                            print(f"Invalid line entry ({function_name}): " + newlines[back_index]);
                        for arg in newlines[back_index].split(','):
                            if (arg.strip() == ""):
                                continue;
                            tokens = arg.split();
                            if (len(short_params) + len(tokens[-1].replace('*', '')) + 2 + 21 > 100 and not newlined):
                                short_params = short_params + ",\n" + indent;
                                newlined = True;
                                short_params = f"{short_params}{tokens[-1].replace('*', '').replace('[3]', '').replace('[]', '')}";
                            else:
                                short_params = f"{short_params}, {tokens[-1].replace('*', '').replace('[3]', '').replace('[]', '')}";
                        signature = signature + newlines[back_index];
                        newlines[back_index] = " " + newlines[back_index];
                        back_index = back_index - 1;
                        if (newlines[back_index][0] != ' '):
                            break
            else:
                #print("No match. Appending.");
                newlines.append(line);

    file.close();
    file = open(filename, "w");
    for line in reversed(newlines):
        file.write(line);

