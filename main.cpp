#include "include/common.h"
#include "list.h"



int main ()
{
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsign-conversion"
    list::list_t list;

    list::ctor (&list, sizeof (int), 10);

    int q = -1;
    size_t index = list::insert_after (&list, 0, &q);
    q = -2;
    index = list::insert_after (&list, index, &q);
    q = -3;
    index = list::insert_after (&list, index, &q);

    list::resize (&list, 12);

    q = -4;
    index = list::insert_after (&list, index, &q);

    q = 0;
    index = list::insert_before (&list, 0, &q);

    q = 1;
    index = list::insert_before (&list, index, &q);

    #pragma GCC diagnostic pop

    list::dump (&list);

    list::dtor (&list);
}

