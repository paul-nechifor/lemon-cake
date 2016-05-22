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

typedef struct {
    object *env;
    object *last_object;
} vm_state;

typedef object *func_pointer_t(vm_state *, object *);

typedef struct {
    func_pointer_t *func;
} func_struct;

object *parse_recursive(vm_state *vms, char *s, uint64_t *i, uint64_t len);
void print(object *o);
void free_object(object *o);
object *eval(vm_state *vms, object *o);
uint64_t objects_equal(object *a, object *b);
void dict_add(dict *d, object *key, object *value);

static void die(char *msg) {
    c_fprintf(stderr, "%s\n", msg);
    c_exit(1);
}

object *new_object(vm_state *vms, uint64_t type, void *value) {
    object *o = c_malloc(sizeof(object));
    o->type = type;
    o->value = value;
    o->mark = 0;
    o->next_object = vms->last_object;
    vms->last_object = o;
    return o;
}

object *new_int(vm_state *vms, uint64_t n) {
    uint64_t *i = c_malloc(sizeof(uint64_t));
    *i = n;
    return new_object(vms, TYPE_INT, i);
}

object *new_builtin_func(vm_state *vms, func_pointer_t *func) {
    return new_object(vms, TYPE_BUILTIN_FUNC, func);
}

object *new_string(vm_state *vms, char *s, uint64_t chars) {
    string_struct *ss = c_malloc(sizeof(string_struct));
    ss->length = chars;
    ss->value = c_malloc(sizeof(char) * (chars + 1));
    c_memcpy(ss->value, s, chars);
    ss->value[chars] = '\0';

    return new_object(vms, TYPE_STRING, ss);
}

object *new_symbol(vm_state *vms, char *s, uint64_t chars) {
    // TODO: Use unique symbols.
    symbol_struct *ss = c_malloc(sizeof(symbol_struct));
    ss->id = 0;
    ss->length = chars;
    ss->value = c_malloc(sizeof(char) * (chars + 1));
    c_memcpy(ss->value, s, chars);
    ss->value[chars] = '\0';

    return new_object(vms, TYPE_SYMBOL, ss);
}

object *new_dict(vm_state *vms, uint64_t size) {
    dict *d = c_malloc(sizeof(dict));

    d->n_size = size;
    d->n_filled = 0;
    uint64_t n_table_bytes = sizeof(dict_pair) * d->n_size;
    d->table = c_malloc(n_table_bytes);
    c_memset(d->table, 0, n_table_bytes);

    return new_object(vms, TYPE_DICT, d);
}

void free_symbol(object *o) {
    c_free(((symbol_struct *) o->value)->value);
    c_free(o->value);
    c_free(o);
}

object *new_list(vm_state *vms) {
    list_elem *le = c_malloc(sizeof(list_elem));
    le->value = NULL;
    le->next = NULL;

    return new_object(vms, TYPE_LIST, le);
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

object *read_int(vm_state *vms, char *s, uint64_t *i) {
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

object *read_string(vm_state *vms, char *s, uint64_t *i) {
    uint64_t start = *i;
    uint64_t end = start;
    while (s[end] != '"') {
        end++;
    }

    object *o = new_string(vms, &s[start], end - start);

    *i = end + 1;

    return o;
}

object *read_symbol(vm_state *vms, char *s, uint64_t *i) {
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

object *read_list(vm_state *vms, char *s, uint64_t *i, uint64_t len) {
    object *o = new_list(vms);
    object *read_obj;
    list_elem** curr_elem_ptr = (list_elem **)(
        ((char *) o) + offsetof(object, value)
    );
    list_elem* curr_elem;

    for (;;) {
        read_obj = parse_recursive(vms, s, i, len);
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

void print_dict(object *o) {
    c_fprintf(stdout, "(dict");

    dict *d = o->value;
    dict_pair *table = d->table;
    dict_pair *pair;

    uint64_t i;
    for (i = 0; i < d->n_size; i++) {
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
            print_dict(o);
            break;

        case TYPE_BUILTIN_FUNC:
            c_fprintf(stdout, "(builtin %llu)", o->value);
            break;

        default:
            c_fprintf(stdout, "[Unknown type.]");
    }
}

object *add_numbers(vm_state *vms, object *args_list) {
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

    return new_int(vms, ret);
}

void list_append(vm_state *vms, list_elem *list, object *o) {
    if (!list->value) {
        list->value = o;
        return;
    }

    list_elem *le = list;

    while (le->next) {
        le = le->next;
    }

    le->next = c_malloc(sizeof(list_elem));
    le = le->next;
    le->value = o;
    le->next = NULL;
}

object *list_append_func(vm_state *vms, object *args_list) {
    list_elem *list = args_list->value;
    object *list_obj = list->value;
    object *item = list->next->value;
    list_append(vms, list_obj->value, item);
    return list_obj;
}

object *eval_args_list(vm_state *vms, list_elem *le) {
    object *ret = new_list(vms);
    list_elem *unevaled = le;
    list_elem *evaled = ret->value;

    if (!unevaled) {
        return ret;
    }

    for (;;) {
        evaled->value = eval(vms, unevaled->value);
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

object *hashcode_func(vm_state *vms, object *args_list) {
    return new_int(vms, hashcode_object(((list_elem *) args_list->value)->value));
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

object *dict_func(vm_state *vms, object *args_list) {
    uint64_t n_args = list_length(args_list->value);
    if (n_args % 2) {
        die("Need even number of args.");
    }
    object *dobj = new_dict(vms, next_prime_size(n_args / 2));
    dict *d = dobj->value;
    object *key, value;

    list_elem *le = args_list->value;

    while (le && le->value) {
        key = le->value;
        le = le->next;
        dict_add(d, key, le->value);
        le = le->next;
    }

    return dobj;
}

void dict_add(dict *d, object *key, object *value) {
    uint64_t hash = hashcode_object(key);

    if (d->n_filled + 1 >= d->n_size) {
        die("Fuck, need to grow it.");
    }

    dict_pair *pair = &d->table[hash % d->n_size];

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

object *dict_get(vm_state *vms, dict *d, object *key) {
    uint64_t hash = hashcode_object(key);
    dict_pair *pair = &d->table[hash % d->n_size];
    for (;;) {
        if (!pair->value) {
            return new_list(vms);
        }
        if (objects_equal(key, pair->key)) {
            return pair->value;
        }
        if (!pair->next) {
            return new_list(vms);
        }
        pair = pair->next;
    }
}

object *dict_add_func(vm_state *vms, object *args_list) {
    list_elem *le = args_list->value;
    object *d = le->value;
    le = le->next;
    object *key = le->value;
    le = le->next;
    object *value = le->value;
    dict_add(d->value, key, value);
    return d;
}

object *dict_get_func(vm_state *vms, object *args_list) {
    list_elem *le = args_list->value;
    object *d = le->value;
    le = le->next;
    object *key = le->value;
    return dict_get(vms, d->value, key);
}

uint64_t obj_len_func(object *o) {
    switch (o->type) {
        case TYPE_STRING:
            {
                string_struct *ss = o->value;
                return ss->length;
            }
        case TYPE_SYMBOL:
            {
                symbol_struct *ss = o->value;
                return ss->length;
            }
        case TYPE_LIST:
            return list_length(o->value);

        case TYPE_DICT:
            {
                dict *d = o->value;
                return d->n_filled;
            }
    }
    die("Don't know how get the length for that type.");
}

object *len_func(vm_state *vms, object *args_list) {
    return new_int(vms, obj_len_func(((list_elem *) args_list->value)->value));
}

object *list_func(vm_state *vms, object *args_list) {
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
            {
                symbol_struct *ssa = a->value;
                symbol_struct *ssb = b->value;
                return !c_strcmp(ssa->value, ssb->value);
            }
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

object *is_func(vm_state *vms, object *args_list) {
    list_elem *le1 = args_list->value;
    list_elem *le2 = le1->next;

    return new_int(vms, objects_equal(le1->value, le2->value));
}

object *eval_list(vm_state *vms, object *o) {
    list_elem *le = o->value;

    // An empty list evaluates to itself.
    if (!le->value) {
        return o;
    }

    object *first_elem = le->value;
    if (first_elem->type != TYPE_SYMBOL) {
        die("Cannot eval non-symbol-starting list.");
    }

    object *args_list = eval_args_list(vms, le->next);
    object *func_pointer = dict_get(vms, vms->env->value, first_elem);

    if (func_pointer->type == TYPE_BUILTIN_FUNC) {
        return ((func_pointer_t *) func_pointer->value)(vms, args_list);
    }

    die("That's not a function.");
}

object *eval(vm_state *vms, object *o) {
    switch (o->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_SYMBOL:
        case TYPE_DICT:
            return o;

        case TYPE_LIST:
            return eval_list(vms, o);

        default:
            die("Don't know how to eval that.");
    }
}

void discard_line(char *s, uint64_t *i) {
    while (s[(*i)++] != '\n');
}

object *parse_recursive(vm_state *vms, char *s, uint64_t *i, uint64_t len) {
    char c;
    for (;;) {
        if (*i > len) {
            die("Stepping over the end of the code.");
        }

        if (*i == len) {
            return new_list(vms);
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
            return (object *) ')';
        }

        if (c >= '0' && c <= '9') {
            (*i)--;
            return read_int(vms, s, i);
        }

        if (c == '"') {
            return read_string(vms, s, i);
        }


        (*i)--;
        return read_symbol(vms, s, i);
    }
}

object *parse(vm_state *vms, char *s, uint64_t len) {
    uint64_t i = 0;
    return parse_recursive(vms, s, &i, len);
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

char *builtin_names[] = {
    "dict",
    "dict-add",
    "dict-get",
    "len",
    "list",
    "list-append",
    "hashcode",
    "is",
    "+",
};
func_pointer_t *builtin_pointers[] = {
    dict_func,
    dict_add_func,
    dict_get_func,
    len_func,
    list_func,
    list_append_func,
    hashcode_func,
    is_func,
    add_numbers,
};

vm_state *start_vm() {
    vm_state *vms = c_malloc(sizeof(vm_state));
    vms->last_object = NULL;
    vms->env = new_dict(vms, 4969); // TODO Change this to nested dicts

    dict *d = vms->env->value;
    uint64_t n_builtins = sizeof(builtin_pointers) / sizeof(uint64_t);
    uint64_t i;

    for (i = 0; i < n_builtins; i++) {
        dict_add(
            d,
            new_symbol(vms, builtin_names[i], c_strlen(builtin_names[i])),
            new_builtin_func(vms, builtin_pointers[i])
        );
    }

    return vms;
}

void eval_lines() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    object *o;
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
    }

    if (line) {
        c_free(line);
    }

    // TODO Free vms;
}
