#include <stdio.h>
#include "list.h"
#include "test.h"
#include "lib/log.h"

#define R "\033[91m"
#define G "\033[92m"
#define D "\033[39m"

#define _TEST(cond)                                             \
{                                                               \
    if (cond)                                                   \
    {                                                           \
        log (log::ERR, R "Test FAILED: %s" D, #cond);       \
        failed++;                                               \
    }                                                           \
    else                                                        \
    {                                                           \
        log (log::INF, G "Test OK:     %s" D, #cond);       \
        success++;                                              \
    }                                                           \
}

#define _ASSERT(cond)                                           \
{                                                               \
    if (!(cond))                                                \
    {                                                           \
        log (log::ERR, R "## Test Error: %s##\n" D              \
                        "Condition check failed: %s\n"          \
                        "Test location: File: %s Line: %d",     \
                        __func__, #cond, __FILE__, __LINE__);   \
        return -1;                                              \
    }                                                           \
}

// ----------------------------------------------------------------------------

int test_ctor_verify_not_empty ()
{
    list::list_t list;
    list::ctor (&list, sizeof (int), 10);

    _ASSERT (list::verify (&list) == list::OK);

    list::dtor (&list);
    return 0;
}

int test_ctor_verify_empty ()
{
    list::list_t list;
    list::ctor (&list, sizeof (int), 0);

    _ASSERT (list::verify (&list) == list::OK);

    list::dtor (&list);
    return 0;
}

int test_push_get ()
{
    list::list_t list;
    list::ctor (&list, sizeof (int), 0);

    int val = 228;
    size_t index = list::push_front (&list, &val);

    val = 0;
    list::get (&list, index, &val);
    _ASSERT (val == 228);

    list::dtor (&list);
    return 0;
}

int test_push_pop ()
{
    list::list_t list;
    list::ctor (&list, sizeof (int), 0);

    int val = 1;
    size_t index = list::push_front (&list, &val);
    val = 2;
    list::push_front (&list, &val);
    val = 3;
    list::push_front (&list, &val); 

    val = 0;
    list::pop_front (&list, &val);
    _ASSERT (val == 3);
    list::pop (&list, index, &val);
    _ASSERT (val == 1);
    list::pop_back (&list, &val);
    _ASSERT (val == 2);

    list::dtor (&list);
    return 0;
}

// ----------------------------------------------------------------------------

void run_tests ()
{
    unsigned int success = 0;
    unsigned int failed  = 0;

    log (log::INF, "Starting tests...");
    
    _TEST (test_ctor_verify_not_empty ());
    _TEST (test_ctor_verify_empty ());
    _TEST (test_push_get ());
    _TEST (test_push_pop ());

    log (log::INF, "Tests total: %u, failed %u, success: %u, success ratio: %3.1lf%%",
        failed + success, failed, success, success * 100.0 / (success + failed));
}
