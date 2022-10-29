#include <stdio.h>

#include "include/common.h"
#include "lib/log.h"
#include "list.h"
#include "test.h"

static void print_int (void *elem, FILE *stream);

int main ()
{
    // Open logs

    FILE *log = fopen ("log.html", "w");

    if (log == nullptr)
    {
        fprintf (stderr, "Can't open log files");
    }

    set_log_level (log::DBG);
    set_log_stream (log);

    // Init list

    list::list_t list;
    list::ctor (&list, sizeof (int), 10, print_int);
    size_t iter = 0;

    // Push back
    int q = 0;

    for (int i = 0; i < 8; ++i)
    {
        q = i;
        list::push_back (&list, &q);
        log (log::INF, "Push back q = %d", q);
        list::graph_dump (&list, "Push back q = %d", q);
    }

    graph_dump (&list, "Before pop series");

    for (int i = 0; i < 4; ++i)
    {
        list::pop_front (&list, &q);
        list::graph_dump (&list, "Pop front q = %d", q);
    }

    q = -0; list::push_back (&list, &q);        list::graph_dump (&list, "Push back q = %d", q);
    q = -1; iter = list::push_back (&list, &q); list::graph_dump (&list, "Push back q = %d", q);
    q = -2; list::push_back (&list, &q);        list::graph_dump (&list, "Push back q = %d", q);
    q = -3; list::push_back (&list, &q);        list::graph_dump (&list, "Push back q = %d", q);

    q = -10; list::insert_after (&list, iter, &q); list::graph_dump (&list, "Insert after %zu q = %d", iter, q);
    q = -11; list::insert_after (&list, iter, &q); list::graph_dump (&list, "Insert after %zu q = %d", iter, q);
    q = -12; list::insert_after (&list, iter, &q); list::graph_dump (&list, "Insert after %zu q = %d", iter, q);
    q = -13; list::insert_after (&list, iter, &q); list::graph_dump (&list, "Insert after %zu q = %d", iter, q);

    list::graph_dump (&list, "Before sort");

    log (log::DBG, "SORT");
    list::sort (&list);

    list::graph_dump (&list, "After sort");

    log (log::INF, "-2 iter = %zu\n", list::get_iter (&list, 2));

    list::pop_back (&list, &q);
    log (log::INF, "\nback = %d\n\n", q);

    list::graph_dump (&list, "For fun");

    list::print_errs (list::verify (&list), get_log_stream (), "\t->");

    list::dtor (&list);

    run_tests ();
}

static void print_int (void *elem, FILE *stream)
{
    assert (elem   != nullptr && "pointer can't be nullptr");
    assert (stream != nullptr && "pointer can't be nullptr");

    fprintf (stream, "%d", *(int*) elem);
}