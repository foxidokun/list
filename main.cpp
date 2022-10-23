#include "include/common.h"
#include "list.h"

int main ()
{
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

    list::dump (&list);

    list::dtor (&list);
}