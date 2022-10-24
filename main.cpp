#include <stdio.h>

#include "include/common.h"
#include "lib/log.h"
#include "list.h"
#include "test.h"

int main ()
{
    FILE *log = fopen ("log.html", "w");
    set_log_level (log::DBG);
    set_log_stream (log);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsign-conversion"
    list::list_t list;

    list::ctor (&list, sizeof (int), 10);

    int q = 0;
    q = 0; list::push_back (&list, &q);
    q = 1; list::push_back (&list, &q);
    q = 2; list::push_back (&list, &q);
    q = 3; list::push_back (&list, &q);

    list::pop_front (&list, &q); log (log::INF, "q = %d\n", q);
    list::pop_front (&list, &q); log (log::INF, "q = %d\n", q);
    list::pop_front (&list, &q); log (log::INF, "q = %d\n", q);
    list::pop_front (&list, &q); log (log::INF, "q = %d\n", q);

    q = -0; list::push_back (&list, &q);
    q = -1; list::push_back (&list, &q);
    q = -2; list::push_back (&list, &q);
    q = -3; list::push_back (&list, &q);

    list::dump (&list, get_log_stream ());

    log (log::INF, "-2 iter = %zu\n", list::get_iter (&list, 2));

    list::dump (&list, get_log_stream ());

    list::pop_back (&list, &q);
    log (log::INF, "\nback = %d\n\n", q);

    #pragma GCC diagnostic pop

    list::dump (&list, get_log_stream ());

    list::print_errs (list::verify (&list), get_log_stream (), "\t->");

    list::dtor (&list);

    run_tests ();
}

