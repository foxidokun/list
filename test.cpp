#include <stdio.h>
#include "list.h"
#include "test.h"
#include "lib/log.h"

#define _TEST(cond)                                             \
{                                                               \
    if (cond)                                                   \
    {                                                           \
        log (log::ERR, R "Test FAILED: %s" D, #cond);           \
        failed++;                                               \
    }                                                           \
    else                                                        \
    {                                                           \
        log (log::INF, G "Test OK:     %s" D, #cond);           \
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
        list::graph_dump (&list);                               \
        return -1;                                              \
    }                                                           \
}

// ----------------------------------------------------------------------------

static void print_int (void *elem, FILE *stream)
{
    assert (elem   != nullptr && "pointer can't be nullptr");
    assert (stream != nullptr && "pointer can't be nullptr");

    fprintf (stream, "%d", *(int*) elem);
}

// ----------------------------------------------------------------------------

#define TEST_START()                                \
    list::list_t list;                              \
    list::ctor (&list, sizeof (int), 0, print_int); \
    [[maybe_unused]] int val = 0;


#define TEST_END()      \
{                       \
    list::dtor (&list); \
    return 0;           \
}

// ----------------------------------------------------------------------------

int test_ctor_verify_not_empty ()
{
    list::list_t list;
    list::ctor (&list, sizeof (int), 10, print_int);

    _ASSERT (list::verify (&list) == list::OK);

    TEST_END();
}

int test_ctor_verify_empty ()
{
    TEST_START();

    _ASSERT (list::verify (&list) == list::OK);

    TEST_END();
}

int test_push_get ()
{
    TEST_START();

    val = 228;
    size_t index = list::push_front (&list, &val);

    val = 0;
    list::get (&list, index, &val);
    _ASSERT (val == 228);

    TEST_END();
}

int test_push_pop ()
{
    TEST_START();

    val = 1;
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

    TEST_END();
}

int test_sort ()
{
    TEST_START();

    val = 1; list::push_back  (&list, &val);
    val = 2; list::push_back  (&list, &val);
    val = 0; list::push_front (&list, &val);

    _ASSERT (list.is_sorted == false);
    _ASSERT (list::get_iter (&list, 0) == 3);
    _ASSERT (list::get_iter (&list, 1) == 1);
    _ASSERT (list::get_iter (&list, 2) == 2);

    TEST_END();
}

int test_sorted_flag_pop_back ()
{
    TEST_START();

    val = 1; list::push_back  (&list, &val);
    val = 2; list::push_back  (&list, &val);
    val = 0; list::pop_back   (&list, &val);

    _ASSERT (list.is_sorted == true);

    TEST_END();
}

int test_sorted_flag_pop_front ()
{
    TEST_START();

    val = 1; list::push_back  (&list, &val);
    val = 2; list::push_back  (&list, &val);
    val = 0; list::pop_front   (&list, &val);

    _ASSERT (list.is_sorted == true);

    TEST_END();
}


int test_sorted_pop_push_front ()
{
    TEST_START();

    val = 1; list::push_back  (&list, &val);
    val = 2; list::push_back  (&list, &val);
    val = 0; list::pop_front  (&list, &val);
    val = 3; list::push_front (&list, &val);

    _ASSERT (list.is_sorted == true);

    _ASSERT (list::get_iter (&list, 0) == 1);
    _ASSERT (list::get_iter (&list, 1) == 2);

    TEST_END();
}

int test_sorted_pop_push_back ()
{
    TEST_START();

    val = 1; list::push_back  (&list, &val);
    val = 2; list::push_back  (&list, &val);
    val = 0; list::pop_front  (&list, &val);
    val = 3; list::push_front (&list, &val);

    _ASSERT (list.is_sorted == true);

    TEST_END();
}

int test_sorted_with_shift ()
{
    TEST_START ();

    val = 1; list::push_back (&list, &val);
    val = 2; list::push_back (&list, &val);
    val = 3; list::push_back (&list, &val);
    val = 0; list::pop_front (&list, &val);
    val = 4; list::push_back (&list, &val);

    _ASSERT (list.is_sorted == false);

    TEST_END ();
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
    _TEST (test_sort ());
    _TEST (test_sorted_flag_pop_back ());
    _TEST (test_sorted_flag_pop_front ());
    _TEST (test_sorted_pop_push_front ());
    _TEST (test_sorted_pop_push_back ());
    _TEST (test_sorted_with_shift ());


    log (log::INF, "Tests total: %u, failed %u, success: %u, success ratio: %3.1lf%%",
        failed + success, failed, success, success * 100.0 / (success + failed));
}
