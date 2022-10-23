#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

namespace list
{
    struct list_t
    {
        void   *data_arr;
        size_t *prev_arr;
        size_t *next_arr;

        size_t free_head;
        
        size_t obj_size;
        size_t reserved;
        size_t capacity;
        size_t size;
    };

    enum err_t {
        OK,
        OOM,
        EMPTY,
    };

    err_t ctor (list_t *list, size_t obj_size, size_t reserved);

    void dtor (list_t *list);

    [[nodiscard ("You lost your index")]]
    ssize_t insert_after (list_t *list, size_t index, const void *elem);

    err_t resize (list_t *list, size_t new_capacity);

    const char *err_to_str (const err_t err);

    void dump (list_t *list);
}

#endif