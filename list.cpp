#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "include/common.h"
#include "lib/log.h"
#include "list.h"

// ----------------------------------------------------------------------------
// CONST SECTION
// ----------------------------------------------------------------------------

static const size_t FREE_PREV = (size_t) -1;
static const size_t DUMP_FILE_PATH_LEN = 15;
static const char DUMP_FILE_PATH_FORMAT[] = "dump/%d.grv";

const int INDEX_MAX_LEN = 10;

// ----------------------------------------------------------------------------
// GRAPHVIZ SECTION
// ----------------------------------------------------------------------------

const char PREFIX[] = "digraph {\nrankdir=LR;\nnode [shape=record,style=\"filled\"]\nsplines=ortho;\n";

const char FREE_FILLCOLOR[]     = "lightblue";
const char FREE_COLOR[]         = "darkblue";

const char REGULAR_FILLCOLOR[]  = "palegreen";
const char REGULAR_COLOR[]      = "green";

const char INVALID_FILLCOLOR[]  = "lightpink";
const char INVALID_COLOR[]      = "darkred";

const char NULLCELL_FILLCOLOR[] = "linen";
const char NULLCELL_COLOR[]     = "lemonchiffon";

const char NEXT_EDGE_COLOR[]    = "coral";
const char PREV_EDGE_COLOR[]    = "indigo";

// ----------------------------------------------------------------------------
// CRINGE WRAPPER SECTION
// ----------------------------------------------------------------------------

const int RAND_TRUE_MAX = RAND_MAX / 10;
const int NUM_OF_TRIES  = 3;

#include "questions.h"

// ----------------------------------------------------------------------------
// STATIC DEFINITIONS
// ----------------------------------------------------------------------------

#define UNWRAP_MALLOC(val)    \
{                             \
    if ((val) == nullptr)     \
    {                         \
        log (log::ERR, "OOM");\
        return list::OOM;     \
    }                         \
}

static bool check_cell  (const list::list_t *list, list::iter_t index);
static void verify_data_loop  (const list::list_t *list, list::err_flags *flags);

static void generate_graphiz_code (const list::list_t *list, FILE *stream);
static void set_colors (const list::list_t *list, list::iter_t index,
                        const char **fillcolor, const char **color);
static void node_codegen (const list::list_t *list, list::iter_t iter, size_t index, const char *fillcolor,
                          const char *color, FILE *stream);
static void edge_codegen (const list::list_t *list, list::iter_t iter, size_t index, FILE *stream);

// ----------------------------------------------------------------------------
// PUBLIC FUNCTIONS
// ----------------------------------------------------------------------------

#define _UNWRAP_MALLOC_GOTO(ptr)    \
{                                   \
    if (ptr == nullptr)             \
    {                               \
        goto failed_malloc_cleanup; \
    }                               \
}

list::err_t list::ctor (list_t *list, size_t obj_size, size_t reserved,
                                void (*print_func)(void *elem, FILE *stream))
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (obj_size > 0 && "Object size can't be less than 1");
    assert (print_func != nullptr && "pointer can't be nullptr");

    //Nuke them
    list->null_node = nullptr;

    // Allocate null object + reserved
    list->null_node = (node_t *)calloc (sizeof (node_t) + obj_size, 1);
    _UNWRAP_MALLOC_GOTO (list->null_node);

    // Init fields
    list->obj_size   = obj_size;
    list->size       = 0;
    list->print_func = print_func;

    // Init null cell
    list->null_node->prev  = list->null_node;
    list->null_node->next  = list->null_node;

    return list::OK;

    failed_malloc_cleanup:
        free (list->null_node);
        return list::OOM;
}

#undef _UNWRAP_MALLOC_GOTO

// ----------------------------------------------------------------------------

void list::dtor (list_t *list)
{
    assert (list != nullptr && "pointer can't be null");

    if (list::verify (list) != list::OK)
    {
        log (log::WRN, "Destructing invalid list with errors\n");
        list::print_errs (verify (list), get_log_stream(), "-->\t");
    }

    node_t *next_node = list->null_node;

    for (size_t i = 0; i < list->size; ++i)
    {
        next_node = next_node->next;
        free (next_node->prev);
    }

    free (next_node);
}

// ----------------------------------------------------------------------------

list::err_flags list::verify (const list_t *list)
{
    if (list == nullptr)
    {
        return list::NULLPTR;
    }

    list::err_flags flags = list::OK;

    if (flags == list::OK)
    {
        verify_data_loop (list, &flags);
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

list::iter_t list::insert_after (list_t *list, iter_t cur_cell, const void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    list_assert (list);

    // Find free cell
    node_t *new_cell = (node_t *) calloc (sizeof (node_t) + list->obj_size, 1);
    if (new_cell == nullptr) return nullptr;
    
    // Copy data
    memcpy (new_cell+1, elem, list->obj_size);
    new_cell->value = new_cell + 1;

    // Update pointers
    cur_cell->next->prev = new_cell;
    new_cell->next       = cur_cell->next;
    new_cell->prev       = cur_cell;
    cur_cell->next       = new_cell;

    list->size++;

    return new_cell;
}

list::iter_t list::insert_before (list_t *list, iter_t cur_cell, const void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    list_assert (list);

    return list::insert_after (list, cur_cell->prev, elem);
}

list::iter_t list::push_back (list_t *list, const void *elem)
{
    assert (list != nullptr && "pointer can't be null");
    assert (elem != nullptr && "pointer can't be null");
    list_assert (list);

    return list::insert_before (list, list->null_node, elem);
}

list::iter_t list::push_front (list_t *list, const void *elem)
{
    assert (list != nullptr && "pointer can't be null");
    assert (elem != nullptr && "pointer can't be null");
    list_assert (list);

    return list::insert_after (list, list->null_node, elem);
}

// ----------------------------------------------------------------------------

void list::get (list_t *list, iter_t index, void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    list_assert (list);

    memcpy (elem, index+1, list->obj_size);
}

// ----------------------------------------------------------------------------

void list::remove (list_t *list, iter_t index, void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    list_assert (list);
    assert (index != list->null_node && "Invalid operation");

    list::get (list, index, elem);
    index->prev->next = index->next;
    index->next->prev = index->prev;
    free (index);

    list->size--;
}

void list::pop_front (list_t *list, void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    list_assert (list);

    list::remove (list, list::next (list, list->null_node), elem);
}

void list::pop_back (list_t *list, void *elem)
{
    assert (list != nullptr && "pointer can't be nullptr");
    assert (elem != nullptr && "pointer can't be nullptr");
    list_assert (list);

    list::remove (list, list::prev (list, list->null_node), elem);
}

// ----------------------------------------------------------------------------

list::iter_t list::next (const list_t *list, iter_t index)
{
    assert (list != nullptr && "pointer can't be nullptr");
    list_assert (list);

    return index->next;
}

list::iter_t list::prev (const list_t *list, iter_t index)
{
    assert (list != nullptr && "pointer can't be nullptr");
    list_assert (list);

    return index->prev;
}

list::iter_t list::head (const list_t *list)
{
    assert (list != nullptr && "pointer can't be nullptr");
    list_assert (list);

    return list::next (list, list->null_node);
}

list::iter_t list::tail (const list_t *list)
{
    assert (list != nullptr && "pointer can't be nullptr");
    list_assert (list);

    return list::prev (list, list->null_node);
}

// ----------------------------------------------------------------------------

list::iter_t list::get_iter (const list_t *list, size_t index)
{
    assert (list != nullptr && "pointer can't be null");
    list_assert (list);
    assert (index < list->size && "index out of bounds");

    iter_t iter = list::head (list);
    for (size_t i = 0; i < index; ++i)
    {
        iter = list::next (list, iter);
    }

    return iter;
}

// ----------------------------------------------------------------------------

void list::graph_dump (const list::list_t *list, const char *reason_fmt, ...)
{
    assert (list != nullptr && "pointer can't be nullptr");

    static int counter = 0;
    counter++;

    char filepath[DUMP_FILE_PATH_LEN+1] = "";    
    sprintf (filepath, DUMP_FILE_PATH_FORMAT, counter);

    FILE *dump_file = fopen (filepath, "w");
    if (dump_file == nullptr)
    {
        log (log::ERR, "Failed to open dump file '%s'", filepath);
        return;
    }

    generate_graphiz_code (list, dump_file);
    fclose (dump_file);

    char cmd[2*DUMP_FILE_PATH_LEN+20+1] = "";
    sprintf (cmd, "dot -T png -o %s.png %s", filepath, filepath);
    if (system (cmd) != 0)
    {
        log (log::ERR, "Failed to execute '%s'", cmd);
    }

    va_list args;
    va_start (args, reason_fmt);

    #if HTML_LOGS
        fprintf  (get_log_stream(), "<h2>List dump: ");
        vfprintf (get_log_stream(), reason_fmt, args);
        fprintf  (get_log_stream(), "</h2>");

        fprintf (get_log_stream(), "\n\n<img src=\"%s.png\">\n\n", filepath);
    #else
        log (log::INF, "Dump path: %s.png", filepath);
    #endif

    va_end (args);

    fflush (get_log_stream ());
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

static bool check_cell (const list::list_t *list, list::iter_t index)
{
    assert (list != nullptr && "pointer can't be null");

    if (index->prev->next != index)
    {
        log (log::ERR, "next[prev[index]] != index");
        return false;
    }

    if (index->next->prev != index)
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

    list::iter_t index = list->null_node->next;

    if (*flags == list::OK && list->size > 0)
    {
        for (size_t i = 0; i + 1 < list->size; ++i)
        {
            if (check_cell (list, index))
            {
                index = index->next;
            }
            else
            {
                log (log::ERR, "Broken cell");
                *flags |= list::BROKEN_DATA_LOOP;
                return;
            }
        }
    }

    if (index->next != list->null_node)
    {
        log (log::ERR, "Invalid loop size, last index is %zu", index);
        *flags |= list::BROKEN_DATA_LOOP;
        return;
    }   
}

// ----------------------------------------------------------------------------

static void generate_graphiz_code (const list::list_t *list, FILE *stream)
{
    assert (list   != nullptr && "pointer can't be null");
    assert (stream != nullptr && "pointer can't be null");

    fprintf (stream, PREFIX);
    const char *fillcolor = nullptr;
    const char *color     = nullptr;

    fprintf (get_log_stream(), "\n<hr>\n");

    fprintf (stream, "node_main [label = \" "
                    "   obj_size: %zu "
                      "| size: %zu\"]\n",
                      list->obj_size, list->size);

    fprintf (stream, "node_main    -> node_0   [style=\"invis\", weight=100]\n");

    fprintf (stream, "node_ind_struct [label=\"STRUCT\", fillcolor=\"white\"]\n");
    fprintf (stream, "node_ind_struct -> node_ind_0 [style=\"invis\", weight = 100]\n");


    list::iter_t index = list->null_node;

    for (size_t i = 0; i < list->size + 1; ++i)
    {
        // Set colors
        set_colors (list, index, &fillcolor, &color);

        fprintf (stream, "node_ind_%zu [label=\"%zu\", fillcolor=\"white\"]\n", i, i);

        // Print node
        node_codegen (list, index, i, fillcolor, color, stream);

        // Generate edges
        edge_codegen (list, index, i, stream);

        index = index->next;
    }

    fprintf (stream ,"}");
}

// ----------------------------------------------------------------------------

static void set_colors (const list::list_t *list, list::iter_t index,
                        const char **fillcolor, const char **color)
{
    assert (list      != nullptr && "invalid pointer");
    assert (fillcolor != nullptr && "invalid pointer");
    assert (color     != nullptr && "invalid pointer");

    if (index == 0)
    {
        *color     = NULLCELL_COLOR;
        *fillcolor = NULLCELL_FILLCOLOR;
    }
    else if (check_cell (list, index) || index == 0)
    {
        *color     = REGULAR_COLOR;
        *fillcolor = REGULAR_FILLCOLOR;
    }
    else
    {
        *color     = INVALID_COLOR;
        *fillcolor = INVALID_FILLCOLOR;
    }
}

// ----------------------------------------------------------------------------

static void node_codegen (const list::list_t *list, list::iter_t iter, size_t index, const char *fillcolor,
                        const char *color, FILE *stream)
{
    assert (list      != nullptr && "invalid pointer");
    assert (fillcolor != nullptr && "invalid pointer");
    assert (color     != nullptr && "invalid pointer");
    assert (stream    != nullptr && "invalid pointer");

    fprintf (stream, "node_%zu [label = \"", index);
    if (index != 0)
    {
        list->print_func (iter->value, stream);
    }
    else
    {
        fprintf (stream, "nil"); 
    }

    fprintf (stream, "| %p", iter);
    fprintf (stream, "\"fillcolor=\"%s\", color=\"%s\"];\n",
                         fillcolor, color);
}

// ----------------------------------------------------------------------------

static void edge_codegen (const list::list_t *list, list::iter_t iter, size_t index, FILE *stream)
{
    assert (list   != nullptr && "pointer can't be nullptr");
    assert (stream != nullptr && "pointer can't be nullptr");

    // Invisible edge

    if (index < list->size)
    {
        fprintf (stream, "node_%zu->node_%zu [style=invis, weight = 100]\n",
            index, index + 1);
        fprintf (stream, "node_ind_%zu->node_ind_%zu [style=invis, weight = 100]\n",
                    index, index + 1);
    }

    // Prev edge
    fprintf (stream, "node_%zu -> node_%zu[color = \"%s\","
                     "constraint=false];\n", index, index == 0 ? list->size : index - 1,
                     PREV_EDGE_COLOR);

    // Next edge
    fprintf (stream, "node_%zu -> node_%zu[color = \"%s\","
                     "constraint=false];\n", index, (index + 1) % (list->size + 1),
                     NEXT_EDGE_COLOR);
}