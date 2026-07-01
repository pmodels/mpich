/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HINT_FNS_H_INCLUDED
#define HINT_FNS_H_INCLUDED

int ADIOI_Info_check_and_install_int(ADIO_File fd, MPI_Info info, const char *key,
                                     int *local_cache, char *funcname, int *error_code);

int ADIOI_Info_check_and_install_enabled(ADIO_File fd, MPI_Info info, const char *key,
                                         int *local_cache, char *funcname, int *error_code);

int ADIOI_Info_check_and_install_true(ADIO_File fd, MPI_Info info, const char *key,
                                      int *local_cache, char *funcname, int *error_code);

int ADIOI_Info_check_and_install_str(ADIO_File fd, MPI_Info info, const char *key,
                                     char **local_cache, char *funcname, int *error_code);

#endif /* HINT_FNS_H_INCLUDED */
