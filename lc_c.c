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
    TYPE_BUILTIN_FUNC,
    TYPE_CONSTRUCT,
};

struct object_t;
struct vm_state;
struct dict_pair;
typedef struct object_t *func_pointer_t(struct vm_state *, struct object_t *);

struct object_t {
    uint64_t type;
    uint64_t marked;
    struct object_t *next_object;

    union {
        // TYPE_INT
        uint64_t int_value;

        // TYPE_STRING
        struct {
            // The number of actual characters ('\0' is not included).
            uint64_t string_length;
            // Contains `length` characters plus an aditional '\0' at the end.
            char *string_pointer;
        };

        // TYPE_SYMBOL
        struct {
            // The unique id for the symbol.
            uint64_t symbol_id;
            // The number of actual characters ('\0' is not included).
            uint64_t symbol_length;
            // Contains `length` characters plus an aditional '\0' at the end.
            char *symbol_pointer;
        };

        // TYPE_LIST
        struct {
            struct object_t* head;
            struct object_t* tail;
        };

        // TYPE_DICT
        struct {
            uint64_t dict_n_size;
            uint64_t dict_n_filled;
            struct dict_pair *dict_table;
        };

        // TYPE_BUILTIN_FUNC
        func_pointer_t *builtin;

        // TYPE_CONSTRUCT
        func_pointer_t *construct;
    };
};
typedef struct object_t object_t;

struct dict_pair {
    object_t *key;
    object_t *value;
    struct dict_pair *next;
};
typedef struct dict_pair dict_pair;

struct vm_state {
    object_t *env;
    object_t *last_object;
};
typedef struct vm_state vm_state;

object_t *parse_recursive(vm_state *vms, char *s, uint64_t *i, uint64_t len);
void print(object_t *o);
void free_object(object_t *o);
object_t *eval(vm_state *vms, object_t *o);
uint64_t objects_equal(object_t *a, object_t *b);
void dict_add(object_t *d, object_t *key, object_t *value);

static void die(char *msg) {
    c_fprintf(stderr, "%s\n", msg);
    c_exit(1);
}

object_t *new_object_t(vm_state *vms, uint64_t type) {
    object_t *o = c_malloc(sizeof(object_t));
    o->type = type;
    o->marked = 0;
    o->next_object = vms->last_object;
    vms->last_object = o;
    return o;
}

object_t *new_int(vm_state *vms, uint64_t n) {
    object_t *ret = new_object_t(vms, TYPE_INT);
    ret->int_value = n;
    return ret;
}

object_t *new_builtin_func(vm_state *vms, func_pointer_t *func) {
    object_t *ret = new_object_t(vms, TYPE_BUILTIN_FUNC);
    ret->builtin = func;
    return ret;
}

object_t *new_construct(vm_state *vms, func_pointer_t *func) {
    object_t *ret = new_object_t(vms, TYPE_CONSTRUCT);
    ret->construct = func;
    return ret;
}

object_t *new_string(vm_state *vms, char *s, uint64_t chars) {
    object_t *ret = new_object_t(vms, TYPE_STRING);
    ret->string_length = chars;
    ret->string_pointer = c_malloc(sizeof(char) * (chars + 1));
    c_memcpy(ret->string_pointer, s, chars);
    ret->string_pointer[chars] = '\0';
    return ret;
}

object_t *new_symbol(vm_state *vms, char *s, uint64_t chars) {
    // TODO: Use unique symbols.
    object_t *ret = new_object_t(vms, TYPE_SYMBOL);
    ret->symbol_id = 0;
    ret->symbol_length = chars;
    ret->symbol_pointer = c_malloc(sizeof(char) * (chars + 1));
    c_memcpy(ret->symbol_pointer, s, chars);
    ret->symbol_pointer[chars] = '\0';
    return ret;
}

object_t *new_dict(vm_state *vms, uint64_t size) {
    object_t *ret = new_object_t(vms, TYPE_DICT);
    ret->dict_n_size = size;
    ret->dict_n_filled = 0;
    uint64_t n_table_bytes = sizeof(dict_pair) * ret->dict_n_size;
    ret->dict_table = c_malloc(n_table_bytes);
    c_memset(ret->dict_table, 0, n_table_bytes);
    return ret;
}

object_t *new_pair(vm_state *vms) {
    object_t *ret = new_object_t(vms, TYPE_LIST);
    ret->head = NULL;
    ret->tail = NULL;
    return ret;
}

object_t *read_int(vm_state *vms, char *s, uint64_t *i) {
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

    return new_int(vms, num);
}

object_t *read_string(vm_state *vms, char *s, uint64_t *i) {
    uint64_t start = *i;
    uint64_t end = start;
    while (s[end] != '\'') {
        end++;
    }

    object_t *o = new_string(vms, &s[start], end - start);

    *i = end + 1;

    return o;
}

object_t *read_symbol(vm_state *vms, char *s, uint64_t *i) {
    uint64_t start = *i;

    char c;

    for (;;) {
        c = s[*i];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == ')') {
            break;
        }
        (*i)++;
    }

    return new_symbol(vms, &s[start], *i - start);
}

object_t *read_list(vm_state *vms, char *s, uint64_t *i, uint64_t len) {
    object_t *o = new_pair(vms);

    object_t *read_obj = parse_recursive(vms, s, i, len);
    if (read_obj == (object_t *) ')') {
        return o;
    }

    object_t *tail = o;
    tail->head = read_obj;
    tail->tail = new_pair(vms);

    for (;;) {
        read_obj = parse_recursive(vms, s, i, len);
        if (read_obj == (object_t *) ')') {
            return o;
        }

        tail = tail->tail;
        tail->head = read_obj;
        tail->tail = new_pair(vms);
    }
}

void print_list(object_t *o) {
    c_fprintf(stdout, "(");
    uint64_t first_elem = 1;

    object_t *pair = o;

    while (pair->head) {
        if (first_elem) {
            first_elem = 0;
        } else {
            c_fprintf(stdout, " ");
        }
        print(pair->head);
        pair = pair->tail;
    }

end_print_list:
    c_fprintf(stdout, ")");
}

void print_dict(object_t *o) {
    c_fprintf(stdout, "(dict");

    dict_pair *table = o->dict_table;
    dict_pair *pair;

    uint64_t i;
    uint64_t size = o->dict_n_size;
    for (i = 0; i < size; i++) {
        pair = &table[i];
        for (;;) {
            if (pair->value) {
                c_fprintf(stdout, " ");
                print(pair->key);
                c_fprintf(stdout, " ");
                print(pair->value);
            }
            if (!pair->next) {
                break;
            }
            pair = pair->next;
        }
    }

    c_fprintf(stdout, ")");
}

void print(object_t *o) {
    switch (o->type) {
        case TYPE_INT:
            c_fprintf(stdout, "%llu", o->int_value);
            break;

        case TYPE_STRING:
            c_fprintf(stdout, "\'%s\'", o->string_pointer);
            break;

        case TYPE_LIST:
            print_list(o);
            break;

        case TYPE_SYMBOL:
            c_fprintf(stdout, "%s", o->symbol_pointer);
            break;

        case TYPE_DICT:
            print_dict(o);
            break;

        case TYPE_BUILTIN_FUNC:
            c_fprintf(stdout, "(builtin %llu)", o->builtin);
            break;

        default:
            c_fprintf(stdout, "[Unknown type.]");
    }
}

object_t *plus_func(vm_state *vms, object_t *args_list) {
    uint64_t ret = 0;

    object_t *pair = args_list;
    object_t *o;

    while (pair->head) {
        o = pair->head;
        if (o->type != TYPE_INT) {
            die("Not int.");
        }
        ret += o->int_value;
        pair = pair->tail;
    }

    return new_int(vms, ret);
}

void list_append(vm_state *vms, object_t *list, object_t *o) {
    object_t *p = list;
    while (p->head) {
        p = p->tail;
    }
    p->head = o;
    p->tail = new_pair(vms);
}

object_t *append_func(vm_state *vms, object_t *args_list) {
    list_append(vms, args_list->head, args_list->tail->head);
    return args_list->head;
}

object_t *head_func(vm_state *vms, object_t *args_list) {
    return args_list->head->head;
}

object_t *tail_func(vm_state *vms, object_t *args_list) {
    return args_list->head->tail;
}

object_t *eval_args_list(vm_state *vms, object_t *list) {
    object_t *ret = new_pair(vms);
    object_t *unevaled = list;

    if (!unevaled->head) {
        return ret;
    }

    object_t *evaled = ret;

    for (;;) {
        evaled->head = eval(vms, unevaled->head);
        evaled->tail = new_pair(vms);
        unevaled = unevaled->tail;

        if (!unevaled->head) {
            break;
        }

        evaled = evaled->tail;
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

uint64_t hashcode_object(object_t *o) {
    switch (o->type) {
        case TYPE_INT:
            return o->int_value;

        case TYPE_STRING:
            return hash_bytes(o->string_pointer, o->string_length);

        case TYPE_SYMBOL:
            return hash_bytes(o->symbol_pointer, o->symbol_length);

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

object_t *hashcode_func(vm_state *vms, object_t *args_list) {
    return new_int(vms, hashcode_object(args_list->head));
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

uint64_t list_length(object_t *p) {
    uint64_t ret = 0;
    object_t *pair = p;

    while (pair->head) {
        ret++;
        pair = pair->tail;
    }

    return ret;
}

object_t *dict_func(vm_state *vms, object_t *args_list) {
    uint64_t n_args = list_length(args_list);
    if (n_args % 2) {
        die("Need even number of args.");
    }
    object_t *dobj = new_dict(vms, next_prime_size(n_args / 2));
    object_t *key;

    object_t *p = args_list;

    while (p->head) {
        key = p->head;
        p = p->tail;
        dict_add(dobj, key, p->head);
        p = p->tail;
    }

    return dobj;
}

void dict_add(object_t *d, object_t *key, object_t *value) {
    uint64_t hash = hashcode_object(key);

    if (d->dict_n_filled + 1 >= d->dict_n_size) {
        die("Fuck, need to grow it.");
    }

    dict_pair *pair = &d->dict_table[hash % d->dict_n_size];

    for (;;) {
        if (!pair->value) {
            pair->key = key;
            pair->value = value;
            return;
        }

        if (objects_equal(key, pair->key)) {
            return;
        }

        if (!pair->next) {
            pair->next = c_malloc(sizeof(dict_pair));
            pair = pair->next;
            pair->key = key;
            pair->value = value;
            pair->next = NULL;
            return;
        }

        pair = pair->next;
    }
}

object_t *dict_get(vm_state *vms, object_t *d, object_t *key) {
    uint64_t hash = hashcode_object(key);
    dict_pair *pair = &d->dict_table[hash % d->dict_n_size];
    for (;;) {
        if (!pair->value) {
            return new_pair(vms);
        }
        if (objects_equal(key, pair->key)) {
            return pair->value;
        }
        if (!pair->next) {
            return new_pair(vms);
        }
        pair = pair->next;
    }
}

object_t *set_func(vm_state *vms, object_t *args_list) {
    object_t *p = args_list;
    object_t *d = p->head;
    p = p->tail;
    object_t *key = p->head;
    p = p->tail;
    object_t *value = p->head;
    dict_add(d, key, value);
    return d;
}

object_t *get_func(vm_state *vms, object_t *args_list) {
    return dict_get(vms, args_list->head, args_list->tail->head);
}

uint64_t obj_len_func(object_t *o) {
    switch (o->type) {
        case TYPE_STRING:
            return o->string_length;
        case TYPE_SYMBOL:
            return o->symbol_length;
        case TYPE_LIST:
            return list_length(o);
        case TYPE_DICT:
            return o->dict_n_filled;
    }
    die("Don't know how get the length for that type.");
}

object_t *len_func(vm_state *vms, object_t *args_list) {
    return new_int(vms, obj_len_func(args_list->head));
}

object_t *list_func(vm_state *vms, object_t *args_list) {
    return args_list;
}

object_t *quote_func(vm_state *vms, object_t *args_list) {
    if (!args_list) {
        return new_pair(vms);
    }
    return args_list->head;
}

uint64_t objects_equal(object_t *a, object_t *b) {
    if (a->type != b->type) {
        return 0;
    }
    switch (a->type) {
        case TYPE_INT:
            return a->int_value == b->int_value;

        case TYPE_STRING:
            return !c_strcmp(a->string_pointer, b->string_pointer);

        case TYPE_SYMBOL:
            return !c_strcmp(a->symbol_pointer, b->symbol_pointer);

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

object_t *is_func(vm_state *vms, object_t *args_list) {
    return new_int(vms, objects_equal(args_list->head, args_list->tail->head));
}

object_t *eval_list(vm_state *vms, object_t *o) {
    object_t *head = o;

    // An empty list evaluates to itself.
    if (!head->head) {
        return o;
    }

    object_t *first_elem = head->head;
    if (first_elem->type != TYPE_SYMBOL) {
        die("Cannot eval non-symbol-starting list.");
    }

    object_t *func_pointer = dict_get(vms, vms->env, first_elem);

    if (func_pointer->type == TYPE_CONSTRUCT) {
        return (func_pointer->construct)(vms, head->tail);
    }

    object_t *args_list = eval_args_list(vms, head->tail);

    if (func_pointer->type == TYPE_BUILTIN_FUNC) {
        return ((func_pointer_t *) func_pointer->builtin)(vms, args_list);
    }

    die("That's not a function.");
}

object_t *eval(vm_state *vms, object_t *o) {
    switch (o->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_DICT:
            return o;

        case TYPE_SYMBOL:
            return dict_get(vms, vms->env, o);

        case TYPE_LIST:
            return eval_list(vms, o);

        default:
            die("Don't know how to eval that.");
    }
}

void discard_line(char *s, uint64_t *i) {
    while (s[(*i)++] != '\n');
}

object_t *parse_recursive(vm_state *vms, char *s, uint64_t *i, uint64_t len) {
    char c;
    for (;;) {
        if (*i > len) {
            die("Stepping over the end of the code.");
        }

        if (*i == len) {
            return new_pair(vms);
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
            return read_list(vms, s, i, len);
        }

        if (c == ')') {
            return (object_t *) ')';
        }

        if (c >= '0' && c <= '9') {
            (*i)--;
            return read_int(vms, s, i);
        }

        if (c == '\'') {
            return read_string(vms, s, i);
        }

        (*i)--;
        return read_symbol(vms, s, i);
    }
}

object_t *parse(vm_state *vms, char *s, uint64_t len) {
    uint64_t i = 0;
    return parse_recursive(vms, s, &i, len);
}

void free_dict(object_t *o) {
    uint64_t n = o->dict_n_size;
    uint64_t i;

    dict_pair *pair;
    dict_pair *next;

    for (i = 0; i < n; i++) {
        pair = (&o->dict_table[i])->next;

        while (pair) {
            next = pair->next;
            c_free(pair);
            pair = next;
        }
    }

    c_free(o->dict_table);
}

void free_object(object_t *o) {
    switch (o->type) {
        case TYPE_BUILTIN_FUNC:
        case TYPE_CONSTRUCT:
        case TYPE_LIST:
        case TYPE_INT:
            break;

        case TYPE_STRING:
            c_free(o->string_pointer);
            break;

        case TYPE_SYMBOL:
            c_free(o->symbol_pointer);
            // return not break since symbols have special functionality.
            return;

        case TYPE_DICT:
            free_dict(o);
            return;

        default:
            die("Free not implemented for this object type.");
    }
    c_free(o);
}

char *builtin_names[] = {
    "dict",
    "set",
    "get",
    "len",
    "list",
    "append",
    "head",
    "tail",
    "hashcode",
    "is",
    "+",
};
func_pointer_t *builtin_pointers[] = {
    dict_func,
    set_func,
    get_func,
    len_func,
    list_func,
    append_func,
    head_func,
    tail_func,
    hashcode_func,
    is_func,
    plus_func,
};

char *construct_names[] = {
    "quote",
};
func_pointer_t *construct_pointers[] = {
    quote_func,
};

vm_state *start_vm() {
    vm_state *vms = c_malloc(sizeof(vm_state));
    vms->last_object = NULL;
    vms->env = new_dict(vms, 4969); // TODO Change this to nested dicts.

    object_t *d = vms->env;
    uint64_t n_pointers = sizeof(builtin_pointers) / sizeof(uint64_t);
    uint64_t i;

    for (i = 0; i < n_pointers; i++) {
        dict_add(
            d,
            new_symbol(vms, builtin_names[i], c_strlen(builtin_names[i])),
            new_builtin_func(vms, builtin_pointers[i])
        );
    }

    n_pointers = sizeof(construct_pointers) / sizeof(uint64_t);

    for (i = 0; i < n_pointers; i++) {
        dict_add(
            d,
            new_symbol(vms, construct_names[i], c_strlen(construct_names[i])),
            new_construct(vms, construct_pointers[i])
        );
    }

    return vms;
}

void mark(object_t *o) {
    if (o->marked) {
        return;
    }

    o->marked = 1;

    switch (o->type) {
        case TYPE_LIST:
            {
                object_t *pair = o;
                while (pair->head) {
                    mark(pair->head);
                    pair = pair->tail;
                    pair->marked = 1;
                }
            }
            break;

        case TYPE_DICT:
            {
                uint64_t i;
                uint64_t n = o->dict_n_size;
                dict_pair *dict_table = o->dict_table;
                dict_pair *pair;
                for (i = 0; i < n; i++) {
                    pair = &dict_table[i];
                    if (!pair->key) {
                        continue;
                    }
                    for (;;) {
                        mark(pair->key);
                        mark(pair->value);
                        if (!pair->next) {
                            break;
                        }
                        pair = pair->next;
                    }
                }
            }
            break;
    }
}

void sweep(vm_state *vms) {
    object_t **o = &vms->last_object;
    object_t *unreached;

    while (*o) {
        if (!(*o)->marked) {
            unreached = *o;
            *o = unreached->next_object;
            free_object(unreached);
        } else {
            (*o)->marked = 0;
            o = &(*o)->next_object;
        }
    }
}

void gc(vm_state *vms) {
    mark(vms->env);
    sweep(vms);
}

void eval_lines() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    object_t *o;
    vm_state *vms = start_vm();

    for (;;) {
        c_fprintf(stderr, "> ");
        read = c_getline(&line, &len, stdin);
        if (read == -1) {
            break;
        }
        o = eval(vms, parse(vms, line, c_strlen(line)));
        print(o);
        c_fprintf(stdout, "\n");
        gc(vms); // TODO: Move this.
    }

    if (line) {
        c_free(line);
    }

    sweep(vms);
    c_free(vms);
}
