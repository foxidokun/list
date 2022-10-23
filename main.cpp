#include "include/common.h"
#include "list.h"

#include <stdio.h>
#include "lib/log.h"

int main ()
{
    set_log_level (log::DBG);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsign-conversion"
    list::list_t list;

    list::ctor (&list, sizeof (int), 10);

    int q = -1;
    size_t index = list::push_front (&list, &q);
    q = -2;
    index = list::insert_after (&list, index, &q);
    q = -3;
    index = list::insert_after (&list, index, &q);

    list::resize (&list, 12);

    q = -4;
    index = list::insert_after (&list, index, &q);

    q = 0;
    index = list::push_back (&list, &q);

    q = 1;
    index = list::insert_before (&list, index, &q);

    q = -5;
    index = list::push_front (&list, &q);
    q = -6;
    index = list::push_front (&list, &q);

    list::dump (&list);

    list::pop_back (&list, &q);
    printf ("\nback = %d\n\n", q);

    #pragma GCC diagnostic pop

    list::dump (&list);

    list::print_errs (list::verify (&list), stdout, "\t->");

    list::dtor (&list);
}

