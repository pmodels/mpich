/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

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
