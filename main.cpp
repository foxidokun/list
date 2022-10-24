#include <stdio.h>

#include "include/common.h"
#include "lib/log.h"
#include "list.h"
#include "test.h"

int main ()
{
    set_log_level (log::DBG);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsign-conversion"
    list::list_t list;

    list::ctor (&list, sizeof (int), 10);

    int q = 0;
    q = 0; list::push_back (&list, &q);
    q = 1; list::push_back (&list, &q);
    q = 2; list::push_back (&list, &q);
    q = 3; list::push_back (&list, &q);

    list::pop_front (&list, &q); printf ("q = %d\n", q);
    list::pop_front (&list, &q); printf ("q = %d\n", q);
    list::pop_front (&list, &q); printf ("q = %d\n", q);
    list::pop_front (&list, &q); printf ("q = %d\n", q);

    q = -0; list::push_back (&list, &q);
    q = -1; list::push_back (&list, &q);
    q = -2; list::push_back (&list, &q);
    q = -3; list::push_back (&list, &q);

    list::dump (&list);

    printf ("-2 iter = %zu\n", list::get_iter (&list, 2));

    list::dump (&list);

    list::pop_back (&list, &q);
    printf ("\nback = %d\n\n", q);

    #pragma GCC diagnostic pop

    list::dump (&list);

    list::print_errs (list::verify (&list), stdout, "\t->");

    list::dtor (&list);

    run_tests ();
}

