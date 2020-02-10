/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "utest.h"
#include "stdbool.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(__APPLE__)
#define UTEST_UNIX__      1
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#if defined CLOCK_PROCESS_CPUTIME_ID  &&  defined CLOCK_MONOTONIC
#define UTEST_HAS_POSIX_TIMER__       1
#endif
#endif

#if defined(__gnu_linux__)
#define UTEST_LINUX__     1
#include <fcntl.h>
#include <sys/stat.h>
#endif

extern const ut_test_t ut_test_set__[];

typedef struct {
    unsigned char flags;
    double duration;
} ut_test_detail_t;

enum {
    UT_TEST_FLAG_RUN__ = 1 << 0,
    UT_TEST_FLAG_SUCCESS__ = 1 << 1,
    UT_TEST_FLAG_FAILURE__ = 1 << 2,
};

size_t num_tests__ = 0;
ut_test_detail_t *test_details__ = NULL;
size_t test_count__ = 0;
int test_cond_failed__ = 0;

int test_stat_failed_units__ = 0;
int test_stat_run_units__ = 0;

const ut_test_t *test_current_unit__ = NULL;
int test_current_logged__ = 0;
int test_case_current_logged__ = 0;
int test_verbose_level__ = UT_LOG_ERROR;
int test_current_failures__ = 0;
int enable_colorize__ = 0;
int test_timer__ = 0;

#if defined UTEST_HAS_POSIX_TIMER__
clockid_t test_timer_id__;
typedef struct timespec test_timer_t;

#else
typedef int test_timer_t;
#endif
test_timer_t test_timer_start__;
test_timer_t test_timer_end__;

static void timer_init();
static void timer_get_time(struct timespec *ts);
static double timer_diff(struct timespec start, struct timespec end);
static void timer_diff_print();

#define COLOR_DEFAULT__            0
#define COLOR_GREEN__              1
#define COLOR_RED__                2
#define COLOR_DEFAULT_INTENSIVE__  3
#define COLOR_GREEN_INTENSIVE__    4
#define COLOR_RED_INTENSIVE__      5

static int print_colorize(int color, const char *fmt, ...);

static void test_begin_msg(const ut_test_t * test);
static void test_end_msg(int result);
static void line_indent(int level);

static void test_list_print();
static void test_enable(int i);
static void test_complete(int i, int success);
static void test_set_duration(int i, double duration);
static void test_error(const char *fmt, ...);
/* Call directly the given test unit function. */
static int test_do_run(const ut_test_t * test);
/* Trigger the unit test. If possible (and not suppressed) it starts a child
 * process who calls test_do_run(), otherwise it calls test_do_run()
 * directly. */
static void test_run(const ut_test_t * test, int test_index);

#if defined UTEST_HAS_POSIX_TIMER__
static void timer_init()
{
    if (test_timer__ == 1)
        test_timer_id__ = CLOCK_MONOTONIC;
    else if (test_timer__ == 2)
        test_timer_id__ = CLOCK_PROCESS_CPUTIME_ID;
}

static void timer_get_time(struct timespec *ts)
{
    clock_gettime(test_timer_id__, ts);
}

static double timer_diff(struct timespec start, struct timespec end)
{
    double endns;
    double startns;

    endns = end.tv_sec;
    endns *= 1e9;
    endns += end.tv_nsec;

    startns = start.tv_sec;
    startns *= 1e9;
    startns += start.tv_nsec;

    return ((endns - startns) / 1e9);
}

static void timer_diff_print()
{
    printf("%.6lf secs", timer_diff(test_timer_start__, test_timer_end__));
}
#else
static void timer_init()
{
}

static void timer_get_time(int *ts)
{
    void ts;
}

static double timer_diff(int start, int end)
{
    void start;
    void end;
    return 0.0;
}

static void timer_diff_print()
{
}
#endif

static int print_colorize(int color, const char *fmt, ...)
{
    va_list args;
    char buffer[256];
    int n;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    buffer[sizeof(buffer) - 1] = '\0';

    if (!enable_colorize__) {
        return printf("%s", buffer);
    }
#if defined UTEST_UNIX__
    const char *col_str;
    switch (color) {
        case COLOR_GREEN__:
            col_str = "\033[0;32m";
            break;
        case COLOR_RED__:
            col_str = "\033[0;31m";
            break;
        case COLOR_GREEN_INTENSIVE__:
            col_str = "\033[1;32m";
            break;
        case COLOR_RED_INTENSIVE__:
            col_str = "\033[1;31m";
            break;
        case COLOR_DEFAULT_INTENSIVE__:
            col_str = "\033[1m";
            break;
        default:
            col_str = "\033[0m";
            break;
    }
    printf("%s", col_str);
    n = printf("%s", buffer);
    printf("\033[0m");
    return n;
#else
    n = printf("%s", buffer);
    return n;
#endif
}

static void test_begin_msg(const ut_test_t * test)
{
    if (test_verbose_level__ >= UT_LOG_VERBOSE) {
        print_colorize(COLOR_DEFAULT_INTENSIVE__, "Test %s:\n", test->name);
        test_current_logged__++;
    } else if (test_verbose_level__ >= UT_LOG_WARN) {
        int n;
        char spaces[64];

        n = print_colorize(COLOR_DEFAULT_INTENSIVE__, "Test %s... ", test->name);
        memset(spaces, ' ', sizeof(spaces));
        if (n < (int) sizeof(spaces))
            printf("%.*s", (int) sizeof(spaces) - n, spaces);
    } else {
        test_current_logged__ = 1;
    }
}

static void test_end_msg(int result)
{
    int color = (result == 0) ? COLOR_GREEN_INTENSIVE__ : COLOR_RED_INTENSIVE__;
    const char *str = (result == 0) ? "OK" : "FAILED";
    printf("[ ");
    print_colorize(color, "%s", str);
    printf(" ]");

    if (result == 0 && test_timer__) {
        printf("  ");
        timer_diff_print();
    }

    printf("\n");
}

static void line_indent(int level)
{
    static const char spaces[] = "                ";
    int n = level * 2;

    while (n > strlen(spaces)) {
        printf("%s", spaces);
        n -= strlen(spaces);
    }
    printf("%.*s", n, spaces);
}

static void test_list_print()
{
    const ut_test_t *test;

    printf("Unit tests:\n");
    for (test = &ut_test_set__[0]; test->func != NULL; test++)
        printf("  %s\n", test->name);
}

static void test_enable(int i)
{
    if (test_details__[i].flags & UT_TEST_FLAG_RUN__)
        return;

    test_details__[i].flags |= UT_TEST_FLAG_RUN__;
    test_count__++;
}

static void test_complete(int i, int success)
{
    test_details__[i].flags |= success ? UT_TEST_FLAG_SUCCESS__ : UT_TEST_FLAG_FAILURE__;
}

static void test_set_duration(int i, double duration)
{
    test_details__[i].duration = duration;
}

static void test_error(const char *fmt, ...)
{
    va_list args;

    if (test_verbose_level__ == UT_LOG_INFO)
        return;

    if (test_verbose_level__ >= UT_LOG_ERROR) {
        line_indent(1);
        if (test_verbose_level__ >= UT_LOG_VERBOSE)
            print_colorize(COLOR_RED_INTENSIVE__, "ERROR: ");
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }

    if (test_verbose_level__ >= UT_LOG_VERBOSE) {
        printf("\n");
    }
}

/* Call directly the given test unit function. */
static int test_do_run(const ut_test_t * test)
{
    test_current_unit__ = test;
    test_current_failures__ = 0;
    test_current_logged__ = 0;
    test_cond_failed__ = 0;

    test_begin_msg(test);

    fflush(stdout);
    fflush(stderr);

    timer_get_time(&test_timer_start__);
    test->func();
    timer_get_time(&test_timer_end__);

    if (test_verbose_level__ >= UT_LOG_VERBOSE) {
        line_indent(1);
        if (test_current_failures__ == 0) {
            print_colorize(COLOR_GREEN_INTENSIVE__, "SUCCESS: ");
            printf("All conditions have passed.\n");

            if (test_timer__) {
                line_indent(1);
                printf("Duration: ");
                timer_diff_print();
                printf("\n");
            }
        } else {
            print_colorize(COLOR_RED_INTENSIVE__, "FAILED: ");
            printf("Aborted.\n");
        }
        printf("\n");
    } else if (test_verbose_level__ >= UT_LOG_WARN && test_current_failures__ == 0) {
        test_end_msg(0);
    }

    test_current_unit__ = NULL;
    return (test_current_failures__ == 0) ? 0 : -1;
}

/* Trigger the unit test. If possible (and not suppressed) it starts a child
 * process who calls test_do_run(), otherwise it calls test_do_run()
 * directly. */
static void test_run(const ut_test_t * test, int test_index)
{
    int failed = 1;
    test_timer_t start, end;

    test_current_unit__ = test;

    timer_get_time(&start);

    failed = test_do_run(test);

    timer_get_time(&end);

    test_current_unit__ = NULL;

    test_stat_run_units__++;
    if (failed)
        test_stat_failed_units__++;

    test_complete(test_index, !failed);
    test_set_duration(test_index, timer_diff(start, end));
}

void ut_init(int argc, char **argv)
{
#if defined UTEST_UNIX__
    enable_colorize__ = isatty(STDOUT_FILENO);
#else
    enable_colorize__ = 0;
#endif

    /* Count all test units */
    num_tests__ = 0;
    for (int i = 0; ut_test_set__[i].func != NULL; i++)
        num_tests__++;

    test_details__ = (ut_test_detail_t *) calloc(num_tests__, sizeof(ut_test_detail_t));
    if (test_details__ == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(2);
    }

    /* Initialize the proper timer. */
    timer_init();
}

int ut_finalize()
{
    if (test_verbose_level__ >= UT_LOG_WARN) {
        if (test_verbose_level__ >= UT_LOG_VERBOSE) {
            print_colorize(COLOR_DEFAULT_INTENSIVE__, "Summary:\n");

            printf("  Number of all unit tests:     %4d\n", (int) num_tests__);
            printf("  Number of run unit tests:     %4d\n", test_stat_run_units__);
            printf("  Number of failed unit tests:  %4d\n", test_stat_failed_units__);
            printf("  Number of skipped unit tests: %4d\n",
                   (int) num_tests__ - test_stat_run_units__);
        }

        if (test_stat_failed_units__ == 0) {
            print_colorize(COLOR_GREEN_INTENSIVE__, "SUCCESS:");
            printf(" All unit tests have passed.\n");
        } else {
            print_colorize(COLOR_RED_INTENSIVE__, "FAILED:");
            printf(" %d of %d unit tests %s failed.\n",
                   test_stat_failed_units__, test_stat_run_units__,
                   (test_stat_failed_units__ == 1) ? "has" : "have");
        }

        if (test_verbose_level__ >= UT_LOG_VERBOSE)
            printf("\n");
    }

    free(test_details__);

    return (test_stat_failed_units__ == 0) ? 0 : 1;
}

void ut_run()
{
    if (test_count__ == 0) {
        for (int i = 0; ut_test_set__[i].func != NULL; i++)
            test_enable(i);
    }

    for (int i = 0; ut_test_set__[i].func != NULL; i++) {
        test_run(&ut_test_set__[i], i);
    }

}

int main(int argc, char **argv)
{
    ut_init(argc, argv);
    ut_run();
    return ut_finalize();
}

int ut_test_check__(int cond, const char *file, int line, const char *fmt, ...)
{
    const char *result_str;
    int result_color;
    bool print_test_case = false;

    if (cond) {
        result_str = "ok";
        result_color = COLOR_GREEN__;
    } else {
        if (!test_current_logged__ && test_current_unit__ != NULL)
            test_end_msg(-1);

        result_str = "failed";
        result_color = COLOR_RED__;
        print_test_case = true;
        test_current_failures__++;
        test_current_logged__++;
    }

    if (print_test_case || test_verbose_level__ >= UT_LOG_VERBOSE) {
        va_list args;

        line_indent(1);
        if (file != NULL) {
            const char *lastsep = strrchr(file, '/');
            if (lastsep != NULL)
                file = lastsep + 1;
            printf("%s:%d: Check ", file, line);
        }

        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);

        printf("... ");
        print_colorize(result_color, "%s", result_str);
        printf("\n");
        test_current_logged__++;
    }

    test_cond_failed__ = (cond == 0);
    return !test_cond_failed__;
}

void ut_test_message__(const char *fmt, ...)
{
    char buffer[UT_TEST_MSG_MAXSIZE];
    char *line_begin;
    char *line_end;
    va_list args;

    if (test_verbose_level__ < UT_LOG_ERROR)
        return;

    /* allow message only when in verbose or failure. */
    if (test_verbose_level__ != UT_LOG_VERBOSE
        && (test_current_unit__ == NULL || !test_cond_failed__))
        return;

    va_start(args, fmt);
    vsnprintf(buffer, UT_TEST_MSG_MAXSIZE, fmt, args);
    va_end(args);
    buffer[UT_TEST_MSG_MAXSIZE - 1] = '\0';

    line_begin = buffer;
    while ((line_end = strchr(line_begin, '\n')) != NULL) {
        line_indent(2);
        printf("%.*s\n", (int) (line_end - line_begin), line_begin);
        line_begin = line_end + 1;
    }
    if (line_begin[0] != '\0') {
        line_indent(2);
        printf("%s\n", line_begin);
    }
}

void ut_test_dump__(const char *title, const void *addr, size_t size)
{
    static const size_t BYTES_PER_LINE = 16;
    size_t line_begin;
    size_t truncate = 0;

    if (test_verbose_level__ < UT_LOG_ERROR)
        return;

    /* allow memory dumping only when in verbose or failure. */
    if (test_verbose_level__ != UT_LOG_VERBOSE
        && (test_current_unit__ == NULL || !test_cond_failed__))
        return;

    if (size > UT_TEST_DUMP_MAXSIZE) {
        truncate = size - UT_TEST_DUMP_MAXSIZE;
        size = UT_TEST_DUMP_MAXSIZE;
    }

    line_indent(2);
    printf((title[strlen(title) - 1] == ':') ? "%s\n" : "%s:\n", title);

    for (line_begin = 0; line_begin < size; line_begin += BYTES_PER_LINE) {
        size_t line_end = line_begin + BYTES_PER_LINE;
        size_t off;

        line_indent(3);
        printf("%08lx: ", (unsigned long) line_begin);
        for (off = line_begin; off < line_end; off++) {
            if (off < size)
                printf(" %02x", ((const unsigned char *) addr)[off]);
            else
                printf("   ");
        }

        printf("  ");
        for (off = line_begin; off < line_end; off++) {
            unsigned char byte = ((const unsigned char *) addr)[off];
            if (off < size)
                printf("%c", (iscntrl(byte) ? '.' : byte));
            else
                break;
        }

        printf("\n");
    }

    if (truncate > 0) {
        line_indent(3);
        printf("           ... (and more %u bytes)\n", (unsigned) truncate);
    }
}

void ut_test_abort__()
{
    abort();
}

