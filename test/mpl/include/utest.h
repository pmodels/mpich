/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef UTEST_H_INCLUDED
#define UTEST_H_INCLUDED

#include <ctype.h>

/*
 * The test set list specifies names of each test case (should be descriptive) and the corresponding
 * function pointer of the implementing. The function prototype is:
 *
 *   void testcase_func(void);
 *
 * Use the following macro to define the test set
 *   UT_TEST_SET = {
 *       { "testcase1_name", testcase1_func_ptr },
 *       { "testcase2_name", testcase2_func_ptr },
 *       ...
 *       { NULL, NULL }
 *   };
 */
#define UT_TEST_SET               const ut_test_t ut_test_set__[]

/* for testing condition and report result.
 *
 * UT_TEST_CHECK takes the condition being checked and optional printf arguments for more info.
 * In one test case function, if any of the UT_TEST_CHECK fail, it will fail the entire test case.
 */
#define UT_TEST_CHECK_(cond, ...)  ut_test_check__((cond), __FILE__, __LINE__, __VA_ARGS__)
#define UT_TEST_CHECK(cond)        ut_test_check__((cond), __FILE__, __LINE__, "%s", #cond)

/* for aborting when condition is not met.
 */
#define UT_TEST_ASSERT_(cond, ...)                                     \
    do {                                                               \
        if (!ut_test_check__((cond), __FILE__, __LINE__, __VA_ARGS__)) \
            ut_test_abort__();                                         \
    } while (0)
#define UT_TEST_ASSERT(cond)                                           \
    do {                                                               \
        if (!ut_test_check__((cond), __FILE__, __LINE__, "%s", #cond)) \
            ut_test_abort__();                                         \
    } while (0)

/*
 * The following are macros for more information. It only works if there is a failure
 * or when test is in verbose level.
 * printf for message when faliure happened or in verbose.
 */
#define UT_TEST_MSG(...)           ut_test_message__(__VA_ARGS__)

/* max output length per UT_TEST_MSG call. */
#ifndef UT_TEST_MSG_MAXSIZE
#define UT_TEST_MSG_MAXSIZE    1024
#endif

/*
 * Macro for dumping memory for failure or verbose at 16 bytes per line.
 */
#define UT_TEST_DUMP(title, addr, size)    ut_test_dump__(title, addr, size)

/* max output bytes per UT_TEST_DUMP */
#ifndef UT_TEST_DUMP_MAXSIZE
#define UT_TEST_DUMP_MAXSIZE   256
#endif

typedef struct {
    const char *name;
    void (*func) (void);
} ut_test_t;

enum {
    UT_LOG_NONE = -1,
    UT_LOG_INFO = 0,
    UT_LOG_WARN = 1,
    UT_LOG_ERROR = 2,
    UT_LOG_VERBOSE = 3
};

extern const ut_test_t ut_test_set__[];

int ut_test_check__(int cond, const char *file, int line, const char *fmt, ...);
void ut_test_message__(const char *fmt, ...);
void ut_test_dump__(const char *title, const void *addr, size_t size);
void ut_test_abort__(void);

void ut_init(int argc, char **argv);
int ut_finalize();
void ut_run();

#endif /* #ifndef UTEST_H_INCLUDED */
