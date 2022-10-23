#include <assert.h>
#include <string.h>

#include "include/common.h"
#include "lib/log.h"
#include "list.h"

// ----------------------------------------------------------------------------
// CONST SECTION
// ----------------------------------------------------------------------------

static const size_t FREE_NEXT = (size_t) -1;

// ----------------------------------------------------------------------------
// STATIC DEFINITIONS
// ----------------------------------------------------------------------------

#define UNWRAP_MALLOC(val)  \
{                           \
    if ((val) == nullptr)   \
    {                       \
        return list::OOM;   \
    }                       \
}

static ssize_t get_free_cell (list::list_t *list);
static void release_free_cell (list::list_t *list, size_t index);
static bool check_cell  (const list::list_t *list, size_t index);
static bool check_index  (const list::list_t *list, size_t index, bool can_be_zero);
static void verify_data_loop  (const list::list_t *list, list::err_flags *flags);
static void verify_free_loop  (const list::list_t *list, list::err_flags *flags);

// ----------------------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------------------

list::err_t list::ctor (list_t *list, size_t obj_size, size_t reserved)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (obj_size > 0 && "Object size can't be less than 1");

    // Allocate null object + reserved
    list->data_arr = calloc (reserved + 1, obj_size);
    UNWRAP_MALLOC (list->data_arr);

    list->prev_arr = (size_t*) calloc (reserved + 1, sizeof (size_t)); 
    UNWRAP_MALLOC (list->prev_arr);

    list->next_arr = (size_t*) calloc (reserved + 1, sizeof (size_t)); 
    UNWRAP_MALLOC (list->next_arr);

    // Init fields
    list->obj_size = obj_size;
    list->reserved = reserved;
    list->capacity = reserved;
    list->size     = 0;

    // Init null cell
    list->prev_arr[0] = 0;
    list->next_arr[0] = 0;

    // Init free cells
    for (size_t i = 1; i <= reserved; ++i)
    {
        list->next_arr[i] = FREE_NEXT;
        list->prev_arr[i] = i-1;
    }
    list->free_head = reserved;

    return list::OK;
}

void list::dtor (list_t *list)
{
    assert (list != nullptr && "pointer can't be null");

    free (list->data_arr);
    free (list->prev_arr);
    free (list->next_arr);
}

// ----------------------------------------------------------------------------

list::err_flags list::verify (const list_t *list)
{
    if (list == nullptr)
    {
        return list::NULLPTR;
    }

    list::err_flags flags = list::OK;

    if (list->capacity < list->reserved)
    {
        flags |= list::INVALID_CAPACITY;
    }

    if (list->size > list->capacity)
    {
        flags |= list::INVALID_SIZE;
    }

    if (flags == list::OK)
    {
        verify_data_loop (list, &flags);
        verify_free_loop (list, &flags);
    }

    return flags;
}

// ----------------------------------------------------------------------------

#define _PRINT_CASE(err, message)                    \
{                                                    \
    if (flags & err)                                 \
    {                                                \
        fprintf (file, "%s" #message "\n", prefix);  \
        tmp_err = err;                               \
        flags &= ~tmp_err;                           \
    }                                                \
}

void list::print_errs (list::err_flags flags, FILE *file, const char *prefix)
{
    assert (file != nullptr   && "pointer can't be nullptr");
    assert (prefix != nullptr && "pointer can't be nullptr");

    if (flags == list::OK)
    {
        fprintf (file, "%sList is OK\n", prefix);
        return;
    }

    err_flags tmp_err = 0;

    _PRINT_CASE (OOM,   "Out Of Memory");
    _PRINT_CASE (EMPTY, "Empty list"   );
    _PRINT_CASE (NULLPTR, "List pointer is nullptr");
    _PRINT_CASE (INVALID_CAPACITY, "Invalid capacity");
    _PRINT_CASE (INVALID_SIZE, "Invalid size");
    _PRINT_CASE (BROKEN_DATA_LOOP, "Broken data loop");
    _PRINT_CASE (BROKEN_FREE_LOOP, "Broken free loop");

    assert (flags == list::OK && "Unknow error flag");
}

// ----------------------------------------------------------------------------

ssize_t list::insert_after (list_t *list, size_t index, const void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    assert (check_index (list, index, true) && "invalid index");

    // Find free cell
    ssize_t free_index_tmp = get_free_cell (list);
    if (free_index_tmp == -1) return -1;

    size_t free_index = (size_t) free_index_tmp;

    // Copy data
    char *cell_data_ptr = (char *)list->data_arr +
                                free_index * list->obj_size;
    memcpy (cell_data_ptr, elem, list->obj_size);

    // Update pointers
    list->prev_arr[list->next_arr[index]] = free_index;
    list->next_arr[free_index] = list->next_arr[index];
    list->prev_arr[free_index] = index;
    list->next_arr[index]      = free_index;

    // Update list fields
    list->size++;

    return (ssize_t) free_index;
}

ssize_t list::insert_before (list_t *list, size_t index, const void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    assert (check_index (list, index, true) && "invalid index");

    return list::insert_after (list, list->prev_arr[index], elem);
}

ssize_t list::push_front (list_t *list, const void *elem)
{
    return list::insert_before (list, 0, elem);
}

ssize_t list::push_back (list_t *list, const void *elem)
{
    return list::insert_after (list, 0, elem);
}

// ----------------------------------------------------------------------------

void list::get (list_t *list, size_t index, void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    assert (check_index (list, index, false) && "invalid index");

    void *val_ptr = (char *)list->data_arr + list->obj_size * index;
    memcpy (elem, val_ptr, list->obj_size);
}

// ----------------------------------------------------------------------------

void list::pop (list_t *list, size_t index, void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    assert (check_index (list, index, false) && "invalid index");

    list->next_arr[list->prev_arr[index]] = list->next_arr[index];
    list->prev_arr[list->next_arr[index]] = list->prev_arr[index];
    list::get (list, index, elem);
    release_free_cell (list, index);
}

void list::pop_back (list_t *list, void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");

    list::pop (list, list::next (list, 0), elem);
}

void list::pop_front (list_t *list, void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");

    list::pop (list, list::prev (list, 0), elem);
}

// ----------------------------------------------------------------------------

size_t list::next (const list_t *list, size_t index)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (check_index (list, index, true) && "invalid index");

    return list->next_arr[index];
}

size_t list::prev (const list_t *list, size_t index)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (check_index (list, index, true) && "invalid index");

    return list->prev_arr[index];
}

// ----------------------------------------------------------------------------

#define _REALLOC(ptr, size, type)                              \
{                                                              \
    tmp_ptr = realloc (ptr, (new_capacity + 1) * size);        \
    UNWRAP_MALLOC (tmp_ptr);                                   \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")      \
    ptr = (type) tmp_ptr;                                      \
    _Pragma ("GCC diagnostic pop")                             \
}

list::err_t list::resize (list::list_t *list, size_t new_capacity)
{
    assert (list != nullptr && "poointer can't be nullptr");
    assert (new_capacity > list->capacity && "current implementation can't shrink");

    // Realocate arrays
    void *tmp_ptr = nullptr;

    _REALLOC (list->data_arr,  list->obj_size, void   *);
    _REALLOC (list->next_arr, sizeof (size_t), size_t *);
    _REALLOC (list->prev_arr, sizeof (size_t), size_t *);

    // Recreate free stack

    list->prev_arr[list->capacity + 1] = list->free_head;
    list->next_arr[list->capacity + 1] = FREE_NEXT;

    for (size_t i = list->capacity + 2; i < new_capacity + 1; ++i)
    {
        list->prev_arr[i] = i - 1;
        list->next_arr[i] = FREE_NEXT;
    }

    list->free_head = new_capacity;
    list->capacity  = new_capacity;

    return list::OK;
}

#undef _REALLOC

// ----------------------------------------------------------------------------

void list::dump (const list::list_t *list)
{
    assert (list != nullptr && "pointer can't be nullptr");

    printf ("List dump:\n");

    printf ("\tfree_head: %zu\n", list->free_head);
    printf ("\tobj_size:  %zu\n", list->obj_size);
    printf ("\treserved:  %zu\n", list->reserved);
    printf ("\tcapacity:  %zu\n", list->capacity);
    printf ("\tsize:      %zu\n", list->size);

    printf("INDX: ");
    for (size_t i = 0; i <= list->capacity; ++i)
    {
        printf ("%3zu ", i);
    }

    printf ("\nData: ");
    for (size_t i = 0; i <= list->capacity; ++i)
    {
        if (list->next_arr[i] != FREE_NEXT)
        {
            printf ("%3d ", ((int *)list->data_arr)[i]);
        }
        else
        {
            printf ("  F ");
        }
    }

    printf ("\nPrev: ");
    for (size_t i = 0; i <= list->capacity; ++i)
    {
        printf ("%3zu ", list->prev_arr[i]);
    }

    printf ("\nNext: ");
    for (size_t i = 0; i <= list->capacity; ++i)
    {
        printf ("%3zd ", (ssize_t) list->next_arr[i]);
    }
    putchar ('\n');
}

// ----------------------------------------------------------------------------

const char *list::err_to_str (const list::err_t err)
{
    switch (err)
    {
        case list::OK:
            return "OK";

        case list::OOM:
            return "Out Of Memory";

        case list::EMPTY:
            return "List is empty";

        case list::NULLPTR:
            return "List pointer is nullptr";

        case list::INVALID_CAPACITY:
            return "Invalid capacity";

        case list::INVALID_SIZE:
            return "Invalid size";

        case list::BROKEN_DATA_LOOP:
            return "Broken data loop";

        case list::BROKEN_FREE_LOOP:
            return "Broken free loop";

        default:
            assert (0 && "Unexpected error code");
    }
}

// ----------------------------------------------------------------------------
// STATIC FUNCTIONS
// ----------------------------------------------------------------------------

static ssize_t get_free_cell (list::list_t *list)
{
    assert (list != nullptr && "pointer can't be nullptr");

    list::err_t res = list::OK;

    // If we need reallocation
    if (list->free_head == 0) 
    {
        res = list::resize (list, list->capacity * 2);

        if (res != list::OK)
        {
            log (log::ERR, "Failed to reallocate with error '%s'", list::err_to_str (res));
            return ERROR;
        }
    }

    size_t free_index = list->free_head;

    list->free_head = list->prev_arr[list->free_head];
    
    return (ssize_t) free_index;
}

static void release_free_cell (list::list_t *list, size_t index)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (check_index (list, index, false) && "invalid index");

    list->next_arr[index] = FREE_NEXT;
    list->prev_arr[index] = list->free_head;
    list->free_head       = index;
}

// ----------------------------------------------------------------------------

#define _ERR_CASE(cond, msg)                        \
{                                                   \
    if (cond)                                       \
    {                                               \
        log (log::ERR, "Invalid index (%s)", msg);  \
        return false;                               \
    }                                               \
}

static bool check_index (const list::list_t *list, size_t index, bool can_be_zero)
{
    assert (list != nullptr && "pointer can't be nullptr");

    _ERR_CASE (!can_be_zero && index == 0, "null");
    _ERR_CASE (index > list->capacity, "out of bounds");
    _ERR_CASE (list->next_arr[index] == FREE_NEXT, "points to free cell");

    return true;
}

#undef _ERR_CASE

// ----------------------------------------------------------------------------

static bool check_cell (const list::list_t *list, size_t index)
{
    assert (list != nullptr && "pointer can't be null");

    if (!check_index (list, index, false))
    {
        log (log::ERR, "current index is incorrect");
        return false;
    }

    if (list->next_arr[index] == FREE_NEXT)
    {
        log (log::ERR, "Free cell");
        return false;
    }

    if (!check_index (list, list->next_arr[index], true))
    {
        log (log::ERR, "next index is incorrect");
        return false;
    }

    if (!check_index (list, list->prev_arr[index], true))
    {
        log (log::ERR, "prev index is incorrect");
        return false;
    }

    if (list->next_arr[list->prev_arr[index]] != index)
    {
        log (log::ERR, "next[prev[index]] != index");
        return false;
    }

    if (list->prev_arr[list->next_arr[index]] != index)
    {
        log (log::ERR, "prev[next[index]] != index");
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------

static void verify_data_loop  (const list::list_t *list, list::err_flags *flags)
{
    assert (list  != nullptr && "pointer can't be nullptr");
    assert (flags != nullptr && "pointer can't be nullptr");

    size_t index = list->next_arr[0];

    if (!check_index (list, index, true))
    {
        *flags |= list::BROKEN_DATA_LOOP;
        return;
    }

    if (*flags == list::OK)
    {
        for (size_t i = 0; i < list->size - 1; ++i)
        {
            if (check_cell (list, index))
            {
                index = list->next_arr[index];
            }
            else
            {
                log (log::ERR, "Broken cell");
                *flags |= list::BROKEN_DATA_LOOP;
                return;
            }
        }
    }

    if (index != 0)
    {
        log (log::ERR, "Invalid loop size, last index is %zu", index);
        *flags |= list::BROKEN_DATA_LOOP;
        return;
    }   
}

// ----------------------------------------------------------------------------

static void verify_free_loop  (const list::list_t *list, list::err_flags *flags)
{
    assert (list  != nullptr && "pointer can't be nullptr");
    assert (flags != nullptr && "pointer can't be nullptr");

    size_t index = list->free_head;

    // Verify head

    if (index > list->capacity)
    {
        log (log::ERR, "Free head out of bounds");
        *flags |= list::BROKEN_FREE_LOOP;
        return;
    }

    if (index == 0)
    {
        if (list->size != list->capacity)
        {
            log (log::ERR, "No free cells, but size != capacity");
            *flags |= list::BROKEN_FREE_LOOP;
            return;
        }
        else
        {
            return;
        }
    }

    // Iterate

    for (size_t i = 0; i < list->capacity - list->size; ++i)
    {
        if (list->next_arr[index] != FREE_NEXT)
        {
            log (log::ERR, "Invalid free cell %zu", i);
            return;
        }

        index = list->prev_arr[index];

        if (index > list->capacity)
        {
            log (log::ERR, "Next index out of bounds");
            *flags |= list::BROKEN_FREE_LOOP;
            return;
        }
    }

    // Check loop

    if (list->prev_arr[index] != 0)
    {
        log (log::ERR, "Broken free loop, index = %zu", index);
        *flags |= list::BROKEN_FREE_LOOP;
        return;
    }
}