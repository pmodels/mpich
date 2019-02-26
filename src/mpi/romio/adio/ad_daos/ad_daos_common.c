/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
 * The Government's rights to use, modify, reproduce, release, perform, display,
 * or disclose this software are subject to the terms of the Apache License as
 * provided in Contract No. 8F-30005.
 * Any reproduction of computer software, computer software documentation, or
 * portions thereof marked with this legend must also reproduce the markings.
 */

#include "ad_daos.h"
#include <daos_errno.h>

int ADIOI_DAOS_error_convert(int error)
{
    switch (error)
    {
	case -DER_NO_PERM:
	    return MPI_ERR_ACCESS;
	case -DER_ENOENT:
        case -DER_NONEXIST:
	    return MPI_ERR_NO_SUCH_FILE;
	case -DER_IO:
	    return MPI_ERR_IO;
	case -DER_EXIST:
	    return MPI_ERR_FILE_EXISTS;
	case -DER_NOTDIR:
	    return MPI_ERR_BAD_FILE;
	case -DER_INVAL:
	case -DER_STALE:
	    return MPI_ERR_FILE;
	case -DER_NOSPACE:
	    return MPI_ERR_NO_SPACE;
	case -DER_NOSYS:
	    return MPI_ERR_UNSUPPORTED_OPERATION;
	case -DER_NOMEM:
	    return MPI_ERR_INTERN;
	default:
	    return MPI_UNDEFINED;
    }
}

int ADIOI_DAOS_err(const char *myname, const char *filename, int line, int rc)
{
    int error_code = MPI_SUCCESS;

    if (rc == 0)
        return MPI_SUCCESS;

    switch (rc) {
    case -DER_NO_PERM:
        error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                          MPIR_ERR_RECOVERABLE, myname,
                                          line, MPI_ERR_ACCESS,
                                          "**fileaccess", "**fileaccess %s",
                                          filename);
        break;
    case -DER_ENOENT:
    case -DER_NONEXIST:
        error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                          MPIR_ERR_RECOVERABLE, myname,
                                          line, MPI_ERR_NO_SUCH_FILE,
                                          "**filenoexist", "**filenoexist %s",
                                          filename);
        break;
    case -DER_IO:
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          myname, line, MPI_ERR_IO, "**io",
                                          "**io %s", filename);
        break;
    case -DER_EXIST:
        error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                          MPIR_ERR_RECOVERABLE, myname,
                                          line, MPI_ERR_FILE_EXISTS,
                                          "**fileexist", "**fileexist %s",
                                          filename);
        break;
    case -DER_NOTDIR:
        error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                          MPIR_ERR_RECOVERABLE,
                                          myname, line,
                                          MPI_ERR_BAD_FILE,
                                          "**filenamedir", "**filenamedir %s",
                                          filename);
        break;
    case -DER_NOSPACE:
        error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                          MPIR_ERR_RECOVERABLE, myname, line,
                                          MPI_ERR_NO_SPACE, "**filenospace", 0);
        break;
    case -DER_INVAL:
        error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                          MPIR_ERR_RECOVERABLE, myname, line,
                                          MPI_ERR_ARG, "**arg", 0);
        break;
    case -DER_NOSYS:
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          myname, line,
                                          MPI_ERR_UNSUPPORTED_OPERATION,
                                          "**fileopunsupported", 0);
        break;
    case -DER_NOMEM:
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          myname, line, MPI_ERR_NO_MEM,
                                          "**allocmem", 0);
        break;
    default:
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          myname, line, MPI_ERR_IO, "**io",
                                          "**io %s", filename);
        break;
    }

    return error_code;
}
