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

struct object_t {
    uint64_t type;
    uint64_t mark;
    struct object_t *next_object;

    union {
        // TYPE_INT
        uint64_t int_value;

        // TYPE_LIST
        struct {
            struct object_t* head;
            struct object_t* tail;
        };
    };
};
struct object_t;
typedef struct object_t object_t;

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

object_t *new_object_t(vm_state *vms, uint64_t type) {
    object_t *o = c_malloc(sizeof(object_t));
    o->type = type;
    o->mark = 0;
    // TODO: Remove these casts.
    o->next_object = (object_t *) vms->last_object;
    vms->last_object = (object *) o;
    return o;
}

object_t *new_int(vm_state *vms, uint64_t n) {
    object_t *ret = new_object_t(vms, TYPE_INT);
    ret->int_value = n;
    return ret;
}

object *new_builtin_func(vm_state *vms, func_pointer_t *func) {
    return new_object(vms, TYPE_BUILTIN_FUNC, func);
}

object *new_construct(vm_state *vms, func_pointer_t *func) {
    return new_object(vms, TYPE_CONSTRUCT, func);
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

object_t *read_list(vm_state *vms, char *s, uint64_t *i, uint64_t len) {
    object_t *o = new_pair(vms);

    // TODO: Remove cast.
    object_t *read_obj = (object_t *) parse_recursive(vms, s, i, len);
    if (read_obj == (object_t *) ')') {
        return o;
    }

    object_t *tail = o;
    tail->head = read_obj;
    tail->tail = new_pair(vms);

    for (;;) {
        // TODO: Remove cast.
        read_obj = (object_t *) parse_recursive(vms, s, i, len);
        if (read_obj == (object_t *) ')') {
            return o;
        }

        tail = tail->tail;
        tail->head = read_obj;
        tail->tail = new_pair(vms);
    }
}

void print_list(object *o) {
    c_fprintf(stdout, "(");
    uint64_t first_elem = 1;

    // TODO: Remove cast.
    object_t *pair = (object_t *) o;

    while (pair->head) {
        if (first_elem) {
            first_elem = 0;
        } else {
            c_fprintf(stdout, " ");
        }
        print((object *) pair->head); // TODO: Remove cast.
        pair = pair->tail;
    }

end_print_list:
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
    object_t *ot = (object_t *) o; // TODO: Remove this variable.
    switch (o->type) {
        case TYPE_INT:
            c_fprintf(stdout, "%llu", ot->int_value);
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

object_t *plus_func(vm_state *vms, object *args_list) {
    uint64_t ret = 0;

    object_t *pair = (object_t *) args_list; // TODO: Remove cast.
    object_t *o;

    while (pair->head) {
        o = (object_t *) pair->head; // TODO: Remove cast.
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

object_t *list_append_func(vm_state *vms, object_t *args_list) {
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
        evaled->head = (object_t *) eval(vms, (object *) unevaled->head); // TODO: RC
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

uint64_t hashcode_object(object *o) {
    object_t *ot = (object_t *) o; // TODO: Remove cast.
    switch (o->type) {
        case TYPE_INT:
            return ot->int_value;

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

object_t *hashcode_func(vm_state *vms, object_t *args_list) {
    return (object_t *) new_int(vms, hashcode_object((object *) args_list->head)); // TODO: RC
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

object *dict_func(vm_state *vms, object_t *args_list) {
    uint64_t n_args = list_length(args_list);
    if (n_args % 2) {
        die("Need even number of args.");
    }
    object *dobj = new_dict(vms, next_prime_size(n_args / 2));
    dict *d = dobj->value;
    object *key;

    object_t *p = args_list;

    while (p->head) {
        key = (object *) p->head;
        p = p->tail;
        dict_add(d, key, (object *) p->head); // TODO: RC
        p = p->tail;
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
            return (object *) new_pair(vms); // TODO: RC
        }
        if (objects_equal(key, pair->key)) {
            return pair->value;
        }
        if (!pair->next) {
            return (object *) new_pair(vms); // TODO: RC
        }
        pair = pair->next;
    }
}

object *dict_add_func(vm_state *vms, object_t *args_list) {
    object_t *p = args_list;
    object_t *d = p->head;
    p = p->tail;
    object_t *key = p->head;
    p = p->tail;
    object_t *value = p->head;
    dict_add(((object *) d)->value, (object *) key, (object *) value); // TODO: RC
    return (object *) d; // TODO: RC
}

object *dict_get_func(vm_state *vms, object_t *args_list) {
    return dict_get(vms, ((object *) args_list->head)->value, (object *) args_list->tail->head); // TODO: RC
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
            return list_length((object_t *) o); // TODO: RC

        case TYPE_DICT:
            {
                dict *d = o->value;
                return d->n_filled;
            }
    }
    die("Don't know how get the length for that type.");
}

object_t *len_func(vm_state *vms, object_t *args_list) {
    return new_int(vms, obj_len_func((object *) args_list->head)); // TODO: RC
}

object *list_func(vm_state *vms, object *args_list) {
    return args_list;
}

object *quote_func(vm_state *vms, object_t *args_list) {
    if (!args_list) {
        return (object *) new_pair(vms); // TODO: RC
    }
    return (object *) args_list->head; // TODO: RC
}

uint64_t objects_equal(object *a, object *b) {
    if (a->type != b->type) {
        return 0;
    }
    // TODO: Remove these variables;
    object_t *at = (object_t *) a;
    object_t *bt = (object_t *) b;
    switch (a->type) {
        case TYPE_INT:
            return at->int_value == bt->int_value;

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

object_t *is_func(vm_state *vms, object_t *args_list) {
    return new_int(vms, objects_equal((object *) args_list->head, (object *) args_list->tail->head));
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

    object *func_pointer = dict_get(vms, vms->env->value, (object *) first_elem); // TODO: RC

    if (func_pointer->type == TYPE_CONSTRUCT) {
        return (object_t *) ((func_pointer_t *) func_pointer->value)(vms, (object *) head->tail); // TODO: RC
    }

    object_t *args_list = eval_args_list(vms, head->tail);

    if (func_pointer->type == TYPE_BUILTIN_FUNC) {
        return (object_t *) ((func_pointer_t *) func_pointer->value)(vms, (object *) args_list); // TODO: RC
    }

    die("That's not a function.");
}

object *eval(vm_state *vms, object *o) {
    switch (o->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_DICT:
            return o;

        case TYPE_SYMBOL:
            return dict_get(vms, vms->env->value, o);

        case TYPE_LIST:
            return (object *) eval_list(vms, (object_t *) o); // TODO: Remove cast.

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
            return (object *) new_pair(vms); // TODO: RC
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
            return (object *) read_list(vms, s, i, len); // TODO: RC
        }

        if (c == ')') {
            return (object *) ')';
        }

        if (c >= '0' && c <= '9') {
            (*i)--;
            return (object *) read_int(vms, s, i); // TODO: Remove cast.
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
    "dict-add", // TODO: Rename to set.
    "dict-get", // TODO: Rename to get.
    "len",
    "list",
    "list-append", // TODO: Rename to append.
    "head",
    "tail",
    "hashcode",
    "is",
    "+",
};
func_pointer_t *builtin_pointers[] = {
    // TODO: Remove all these casts.
    (func_pointer_t *) dict_func,
    (func_pointer_t *) dict_add_func,
    (func_pointer_t *) dict_get_func,
    (func_pointer_t *) len_func,
    list_func,
    (func_pointer_t *) list_append_func,
    (func_pointer_t *) head_func,
    (func_pointer_t *) tail_func,
    (func_pointer_t *) hashcode_func,
    (func_pointer_t *) is_func,
    (func_pointer_t *) plus_func,
};

char *construct_names[] = {
    "quote",
};
func_pointer_t *construct_pointers[] = {
    (func_pointer_t *) quote_func,
};

vm_state *start_vm() {
    vm_state *vms = c_malloc(sizeof(vm_state));
    vms->last_object = NULL;
    vms->env = new_dict(vms, 4969); // TODO Change this to nested dicts

    dict *d = vms->env->value;
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
