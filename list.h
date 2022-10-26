#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define CRINGE_MODE

namespace list
{
    struct list_t
    {
        void   *data_arr;
        size_t *prev_arr;
        size_t *next_arr;

        size_t free_head;
        size_t free_back;
        
        size_t obj_size;
        size_t reserved;
        size_t capacity;
        size_t size;

        bool is_sorted;

        void (*print_func)(void *elem, FILE *stream);
    };

    typedef uint8_t err_flags; 

    enum err_t {
        OK                  = 0,
        OOM                 = 1 << 0,
        EMPTY               = 1 << 1,
        NULLPTR             = 1 << 2,
        INVALID_CAPACITY    = 1 << 3,
        INVALID_SIZE        = 1 << 4,
        BROKEN_DATA_LOOP    = 1 << 5,
        BROKEN_FREE_LOOP    = 1 << 6
    };

    err_t ctor (list_t *list, size_t obj_size, size_t reserved,
                        void (*print_func)(void *elem, FILE *stream));


    void dtor (list_t *list);

    [[nodiscard]]
    err_flags verify (const list_t *list);

    void print_errs (err_flags flags, FILE *file, const char *prefix);

    [[nodiscard ("You lost your index")]]
    ssize_t insert_after (list_t *list, size_t index, const void *elem);

    [[nodiscard ("You lost your index")]]
    ssize_t insert_before (list_t *list, size_t index, const void *elem);

    ssize_t push_front (list_t *list, const void *elem);

    ssize_t push_back  (list_t *list, const void *elem);

    void get (list_t *list, size_t index, void *elem);

    void pop (list_t *list, size_t index, void *elem);
    void pop_back  (list_t *list, void *elem);
    void pop_front (list_t *list, void *elem);

    size_t next (const list_t *list, size_t index);
    size_t prev (const list_t *list, size_t index);

    size_t head (const list_t *list);
    size_t tail (const list_t *list);

    size_t get_iter (const list_t *list, size_t index);

    err_t resize (list_t *list, size_t new_capacity, bool linearise = false);

    err_t sort (list_t *list);

    const char *err_to_str (const err_t err);

    void dump (const list_t *list, FILE *stream = stdout);
    void graph_dump (const list::list_t *list);
}

#ifndef NDEBUG
    #define list_assert(list)                                       \
    {                                                               \
        list::err_flags check_res = list::verify (list);                  \
        if (check_res != list::OK)                                 \
        {                                                           \
            log(log::ERR,                                           \
                "Invalid list with errors: ");                      \
            list::print_errs (check_res, get_log_stream(), "\t-> ");\
            list::dump (list, get_log_stream());                    \
            fflush (get_log_stream());                              \
            assert (0 && "Invalid list");                           \
        }                                                           \
    }
#else
    #define list_assert(list) {;}    
#endif

#endif