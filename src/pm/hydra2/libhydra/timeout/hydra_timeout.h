/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef HYDRA_TIMEOUT_H
#define HYDRA_TIMEOUT_H

#include <time.h>

#include "hydra_base.h"

struct timeout_s {
    int sec;
    int signal;
    int timed_out;
    time_t start_time;
};

void HYD_init_timeout(int sec, struct timeout_s *timeout);
void HYD_set_alarm(struct timeout_s *timeout);
void HYD_handle_sigalrm(struct timeout_s *timeout);
int HYD_get_time_left(struct timeout_s *timeout);
int HYD_get_timeout_signal(struct timeout_s *timeout);
void HYD_check_timed_out(struct timeout_s *timeout, int *exit_status);
void HYD_print_timeout_message(struct timeout_s *timeout);

#endif /* HYDRA_TIMEOUT_H */
