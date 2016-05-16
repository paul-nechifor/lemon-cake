#include <stdint.h>
#include <stdio.h>

#define offsetof(st, m) ((size_t) ( (char *)&((st *)0)->m - (char *)0 ))

extern void (*c_exit)(int status);
extern int (*c_fprintf)(FILE *stream, const char *format, ...);
extern void (*c_free)(void *ptr);
extern ssize_t (*c_getline)(char **lineptr, size_t *n, FILE *stream);
extern void *(*c_malloc)(size_t size);
extern void *(*c_memcpy)(void *destination, const void *source, size_t num);
extern void *(*c_memset)(void *ptr, int value, size_t num);
extern int (*c_strcmp)(const char *str1, const char *str2);
extern size_t (*c_strlen)(const char *str);

enum {
    TYPE_INT = 1,
    TYPE_STRING,
    TYPE_LIST,
    TYPE_SYMBOL,
    TYPE_DICT,
};

typedef struct {
    // The number of actual characters ('\0' is not included).
    uint64_t length;
    // Contains `length` characters plus an aditional '\0' at the end.
    char *value;
} string_struct;

typedef struct {
    // The unique id for the symbol.
    uint64_t id;
    // The number of actual characters ('\0' is not included).
    uint64_t length;
    // Contains `length` characters plus an aditional '\0' at the end.
    char *value;
} symbol_struct;

struct object {
    uint64_t type;
    uint64_t mark;
    struct object *next_object;
    void *value;
};
struct object;
typedef struct object object;

struct list_elem {
    object *value;
    struct list_elem *next;
};
struct list_elem;
typedef struct list_elem list_elem;

struct dict_pair {
    object *key;
    object *value;
    struct dict_pair *next;
};
struct dict_pair;
typedef struct dict_pair dict_pair;

typedef struct {
    uint64_t n_size;
    uint64_t n_filled;
    dict_pair *table;
} dict;

object *parse_recursive(char *s, uint64_t *i, uint64_t len);
void print(object *o);
void free_object(object *o);
object *eval(object *o);

static void die(char *msg) {
    c_fprintf(stderr, "%s\n", msg);
    c_exit(1);
}

object *new_object(uint64_t type, void *value) {
    object *o = c_malloc(sizeof(object));
    o->type = type;
    o->value = value;
    o->mark = 0;
    o->next_object = NULL; // TODO
    return o;
}

object *new_int(uint64_t n) {
    uint64_t *i = c_malloc(sizeof(uint64_t));
    *i = n;
    return new_object(TYPE_INT, i);
}

object *new_string(char *s, uint64_t chars) {
    string_struct *ss = c_malloc(sizeof(string_struct));
    ss->length = chars;
    ss->value = c_malloc(sizeof(char) * (chars + 1));
    c_memcpy(ss->value, s, chars);
    ss->value[chars] = '\0';

    return new_object(TYPE_STRING, ss);
}

object *new_symbol(char *s, uint64_t chars) {
    // TODO: Use unique symbols.
    symbol_struct *ss = c_malloc(sizeof(symbol_struct));
    ss->id = 0;
    ss->length = chars;
    ss->value = c_malloc(sizeof(char) * (chars + 1));
    c_memcpy(ss->value, s, chars);
    ss->value[chars] = '\0';

    return new_object(TYPE_SYMBOL, ss);
}

object *new_dict(uint64_t size) {
    dict *d = c_malloc(sizeof(dict));

    d->n_size = size;
    d->n_filled = 0;
    uint64_t n_table_bytes = sizeof(dict_pair) * d->n_size;
    d->table = c_malloc(n_table_bytes);
    c_memset(d->table, 0, n_table_bytes);

    return new_object(TYPE_DICT, d);
}

void free_symbol(object *o) {
    c_free(((symbol_struct *) o->value)->value);
    c_free(o->value);
    c_free(o);
}

object *new_list() {
    list_elem *le = c_malloc(sizeof(list_elem));
    le->value = NULL;
    le->next = NULL;

    return new_object(TYPE_LIST, le);
}

void free_list(object *o) {
    list_elem *elem = o->value;
    list_elem *next;
    while (elem) {
        next = elem->next;
        c_free(elem->value);
        c_free(elem);
        elem = next;
    }
    o->value = NULL; // So it's not double freed.
}

object *read_int(char *s, uint64_t *i) {
    char digit;
    uint64_t num = 0;

    for (;;) {
        digit = s[*i];
        if (digit < '0' || digit > '9') {
            break;
        }
        num = num * 10 + digit - '0';
        (*i)++;
    }

    return new_int(num);
}

object *read_string(char *s, uint64_t *i) {
    uint64_t start = *i;
    uint64_t end = start;
    while (s[end] != '"') {
        end++;
    }

    object *o = new_string(&s[start], end - start);

    *i = end + 1;

    return o;
}

object *read_symbol(char *s, uint64_t *i) {
    uint64_t start = *i;

    char c;

    for (;;) {
        c = s[*i];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == ')') {
            break;
        }
        (*i)++;
    }

    return new_symbol(&s[start], *i - start);
}

object *read_list(char *s, uint64_t *i, uint64_t len) {
    object *o = new_list();
    object *read_obj;
    list_elem** curr_elem_ptr = (list_elem **)(
        ((char *) o) + offsetof(object, value)
    );
    list_elem* curr_elem;

    for (;;) {
        read_obj = parse_recursive(s, i, len);
        if (read_obj == (object *) ')') {
            break;
        }

        curr_elem = c_malloc(sizeof(list_elem));
        curr_elem->value = read_obj;
        curr_elem->next = NULL;
        *curr_elem_ptr = curr_elem;
        curr_elem_ptr = (list_elem **)(
            ((char *)curr_elem) + offsetof(list_elem, next)
        );
    }

    return o;
}

void print_list(object *o) {
    list_elem *e;
    uint64_t first_elem = 1;

    c_fprintf(stdout, "(");

    e = (list_elem *) o->value;

    for (e = (list_elem *)o->value; e && e->value; e = e->next) {
        if (first_elem) {
            first_elem = 0;
        } else {
            c_fprintf(stdout, " ");
        }
        print(e->value);
    }

    c_fprintf(stdout, ")");
}

void print(object *o) {
    switch (o->type) {
        case TYPE_INT:
            c_fprintf(stdout, "%llu", *((uint64_t *) o->value));
            break;

        case TYPE_STRING:
            c_fprintf(stdout, "\"%s\"", ((string_struct*) o->value)->value);
            break;

        case TYPE_LIST:
            print_list(o);
            break;

        case TYPE_SYMBOL:
            c_fprintf(stdout, "%s", ((symbol_struct*) o->value)->value);
            break;

        case TYPE_DICT:
            c_fprintf(stdout, "(dict)");
            break;

        default:
            c_fprintf(stdout, "[Unknown type.]");
    }
}

object *add_numbers(object *args_list) {
    uint64_t ret = 0;
    list_elem *next = args_list->value;
    object *o;

    do {
        o = next->value;
        if (o->type != TYPE_INT) {
            die("Not int.");
        }
        ret += *(uint64_t *)o->value;
        next = next->next;
    } while (next);

    return new_int(ret);
}

object *eval_args_list(list_elem *le) {
    object *ret = new_list();
    list_elem *unevaled = le;
    list_elem *evaled = ret->value;

    if (!unevaled) {
        return ret;
    }

    for (;;) {
        evaled->value = eval(unevaled->value);
        unevaled = unevaled->next;

        if (!unevaled) {
            evaled->next = NULL;
            break;
        }

        evaled->next = c_malloc(sizeof(list_elem));
        evaled = evaled->next;
    }

    return ret;
}

uint64_t hash_bytes(char *bytes, uint64_t n) {
    uint64_t i;
    uint64_t ret = 5381;

    for (i = 0; i < n; i++) {
        ret = ret * 33 ^ bytes[i];
    }

    return ret;
}

uint64_t hashcode_object(object *o) {
    switch (o->type) {
        case TYPE_INT:
            return *((uint64_t *) o->value);

        case TYPE_STRING:
            {
                string_struct *ss = o->value;
                return hash_bytes(ss->value, ss->length);
            }

        case TYPE_SYMBOL:
            {
                symbol_struct *ss = o->value;
                return hash_bytes(ss->value, ss->length);
            }

        case TYPE_DICT:
            die("hashcode_object for dict not implemented yet.");
            break;

        case TYPE_LIST:
            die("hashcode_object for list not implemented yet.");
            break;

        default:
            die("Unknown type for hashing");
    }
}

object *hashcode_func(object *args_list) {
    return new_int(hashcode_object(((list_elem *) args_list->value)->value));
}

uint64_t next_prime_size(uint64_t n) {
    if (n < 5) {
        return 5;
    }
    if (n % 2 == 0) {
        n++;
    }
    uint64_t i, max;
    // I'm using n/2 instead of sqrt(n) here so I don't have to import math.h.
    for (;;) {
        max = n / 2;
        for (i = 3; i <= max; i+=2) {
            if (n % i == 0) {
                goto next_number;
            }
        }
        return n;
next_number:;
        n += 2;
    }
}

uint64_t list_length(list_elem *le) {
    if (!le->value) {
        return 0;
    }

    uint64_t len = 1;
    list_elem *next = le->next;

    while (next) {
        next = next->next;
        len++;
    }

    return len;
}

object *dict_func(object *args_list) {
    uint64_t n_args = list_length(args_list->value);
    if (n_args % 2) {
        die("Need even number of args.");
    }
    object *d = new_dict(next_prime_size(n_args / 2));
    return d;
}

object *len_func(object *args_list) {
    object *o = ((list_elem *) args_list->value)->value;

    switch (o->type) {
        case TYPE_STRING:
            {
                string_struct *ss = o->value;
                return new_int(ss->length);
            }
        case TYPE_LIST:
            return new_int(list_length(o->value));

        case TYPE_DICT:
            {
                dict *d = o->value;
                return new_int(d->n_filled);
            }

    }
    die("Don't know how get the length for that type.");
}

object *list_func(object *args_list) {
    return args_list;
}

uint64_t objects_equal(object *a, object *b) {
    if (a->type != b->type) {
        return 0;
    }
    switch (a->type) {
        case TYPE_INT:
            return *((uint64_t *) a->value) == *((uint64_t *) b->value);

        case TYPE_STRING:
            {
                string_struct *ssa = a->value;
                string_struct *ssb = b->value;
                return !c_strcmp(ssa->value, ssb->value);
            }

        case TYPE_SYMBOL:
            die("objects_equal for symbol not implemented yet.");
            break;

        case TYPE_LIST:
            die("objects_equal for list not implemented yet.");
            break;

        case TYPE_DICT:
            die("objects_equal for dict not implemented yet.");
            break;

        default:
            die("Unknown type for checking equality");
    }
    return 1;
}

object *is_func(object *args_list) {
    list_elem *le1 = args_list->value;
    list_elem *le2 = le1->next;

    return new_int(objects_equal(le1->value, le2->value));
}

object *eval_list(object *o) {
    list_elem *le = o->value;

    // An empty lists evaluates to itself.
    if (!le->value) {
        return o;
    }

    object *first_elem = le->value;
    if (first_elem->type != TYPE_SYMBOL) {
        die("Cannot eval non-symbol-starting list.");
    }

    symbol_struct *name_struct = first_elem->value;
    char *name = name_struct->value;

    object *args_list = eval_args_list(le->next);

    if (!c_strcmp(name, "dict")) {
        return dict_func(args_list);
    }

    if (!c_strcmp(name, "len")) {
        return len_func(args_list);
    }

    if (!c_strcmp(name, "list")) {
        return list_func(args_list);
    }

    if (!c_strcmp(name, "hashcode")) {
        return hashcode_func(args_list);
    }

    if (!c_strcmp(name, "is")) {
        return is_func(args_list);
    }

    if (!c_strcmp(name, "+")) {
        return add_numbers(args_list);
    }

    return o;
}

object *eval(object *o) {
    switch (o->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_SYMBOL:
        case TYPE_DICT:
            return o;

        case TYPE_LIST:
            return eval_list(o);

        default:
            die("Don't know how to eval that.");
    }
}

void discard_line(char *s, uint64_t *i) {
    while (s[(*i)++] != '\n');
}

object *parse_recursive(char *s, uint64_t *i, uint64_t len) {
    char c;
    for (;;) {
        if (*i > len) {
            die("Stepping over the end of the code.");
        }

        if (*i == len) {
            return new_list();
        }

        c = s[(*i)++];

        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            continue;
        }

        if (c == '#') {
            discard_line(s, i);
            continue;
        }

        if (c == '(') {
            return read_list(s, i, len);
        }

        if (c == ')') {
            return (object *) ')';
        }

        if (c >= '0' && c <= '9') {
            (*i)--;
            return read_int(s, i);
        }

        if (c == '"') {
            return read_string(s, i);
        }


        (*i)--;
        return read_symbol(s, i);
    }
}

object *parse(char *s, uint64_t len) {
    uint64_t i = 0;
    return parse_recursive(s, &i, len);
}

void free_object(object *o) {
    switch (o->type) {
        case TYPE_INT:
            break;

        case TYPE_STRING:
            c_free(((string_struct *) o->value)->value);
            break;

        case TYPE_LIST:
            free_list(o);
            break;

        case TYPE_SYMBOL:
            free_symbol(o);
            // return not break since symbols have special functionality.
            return;

        case TYPE_DICT:
            die("Free not implemented for dict.");

        default:
            die("Free not implemented for this object type.");
    }
    if (o->value) {
        c_free(o->value);
    }
    c_free(o);
}

void eval_lines() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    object *o;
    for (;;) {
        c_fprintf(stderr, "> ");
        read = c_getline(&line, &len, stdin);
        if (read == -1) {
            break;
        }
        o = eval(parse(line, c_strlen(line)));
        print(o);
        c_fprintf(stdout, "\n");
    }

    if (line) {
        c_free(line);
    }
}
