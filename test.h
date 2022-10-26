#ifndef TEST_H
#define TEST_H

int test_ctor_verify_not_empty ();
int test_ctor_verify_empty ();
int test_push_get ();
int test_push_pop ();

int test_sort ();
int test_sorted_flag_pop_back ();
int test_sorted_flag_pop_front ();
int test_sorted_pop_push_front ();
int test_sorted_with_shift ();
int test_sorted_pop_push_back ();

void run_tests ();

#endif //TEST_H