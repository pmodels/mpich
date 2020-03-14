/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

struct HYD_ui_info_s {
    char *prepend_pattern;
    char *outfile_pattern;
    char *errfile_pattern;
};

extern struct HYD_ui_info_s HYD_ui_info;

#endif /* UI_H_INCLUDED */
