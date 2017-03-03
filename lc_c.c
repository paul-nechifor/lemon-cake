#include <stdint.h>
#include <stdio.h>
#include <dlfcn.h> // TODO: Remove this.

#include "lc.lc.h"

#define ADD_ON_CALL_STACK(vms, obj) \
    do { \
        obj->next_stack_object = vms->call_stack_objects; \
        vms->call_stack_objects = obj; \
    } while (0)

#define PATH_MAX 4096

extern void (*c_exit)(int status);
extern int (*c_fprintf)(FILE *stream, const char *format, ...);
extern void (*c_free)(void *ptr);
extern ssize_t (*c_getline)(char **lineptr, size_t *n, FILE *stream);
extern void *(*c_malloc)(size_t size);
extern void *(*c_memcpy)(void *destination, const void *source, size_t num);
extern void *(*c_memset)(void *ptr, int value, size_t num);
extern int (*c_strcmp)(const char *str1, const char *str2);
extern size_t (*c_strlen)(const char *str);
extern size_t (*c_fread)(void *ptr, size_t size, size_t nmemb, FILE *stream);
extern int (*c_fclose)(FILE *stream);
extern long int (*c_ftell)(FILE *stream);
extern int (*c_fseek)(FILE *stream, long int offset, int whence);
extern FILE *(*c_fopen)(const char *filename, const char *mode);
extern int (*c_fflush)(FILE *stream);
extern void *(*c_memalign)(size_t alignment, size_t size);
extern int (*c_mprotect)(void *addr, size_t len, int prot);
extern long (*c_sysconf)(int name);
extern char *(*c_getcwd)(char* buffer, size_t size);
extern char *(*c_strcat)(char *destination, char *source);
extern char *(*c_strstr)(const char *haystack, const char *needle);

extern uint64_t *prog_argc_ptr;
extern char *libc_handle;

enum {
    TYPE_INT = 1,
    TYPE_STRING,
    TYPE_LIST,
    TYPE_SYMBOL,
    TYPE_DICT,
    TYPE_BUILTIN_FUNC,
    TYPE_CONSTRUCT,
    TYPE_MACRO,
    TYPE_FUNC,
};

struct object_t;
struct vm_state;
struct dict_pair;
typedef struct object_t *func_pointer_t(
    struct vm_state *,
    struct object_t *,
    struct object_t *
);

struct object_t {
    uint64_t type;
    uint64_t marked;
    struct object_t *next_object;
    struct object_t *next_stack_object;

    union {
        // TYPE_INT
        int64_t int_value;

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

        // TYPE_MACRO
        struct {
            struct object_t* macro_args;
            struct object_t* macro_body;
            struct object_t* macro_parent_env;
        };

        // TYPE_FUNC
        struct {
            struct object_t* func_args;
            struct object_t* func_body;
            struct object_t* func_parent_env;
        };
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
    uint64_t n_objects;
    uint64_t max_objects;
    uint64_t gc_is_on;

    object_t *env;

    // This is a linked list of all the allocated objects and it's used for
    // garbage collection.
    object_t *last_object;

    // This is a linked list of all the objects that are still active in the
    // computation.
    object_t *call_stack_objects;

    // This is a dict containing all the interned objects. The value for each
    // key is the key.
    object_t *interned;

    // This structure contains all the elements above plus an embedded array of
    // interned objects. See `start_vm()` to see how that's done.
};
typedef struct vm_state vm_state;

object_t *parse_recursive(vm_state *vms, char *s, uint64_t *i, uint64_t len);
void print(vm_state *vms, object_t *o);
void free_object(object_t *o);
object_t *eval(vm_state *vms, object_t *env, object_t *o);
uint64_t objects_equal(object_t *a, object_t *b);
void dict_add(object_t *d, object_t *key, object_t *value);
object_t *dict_get(vm_state *vms, object_t *d, object_t *key);
void gc(vm_state *vms);
object_t *eval_file(vm_state *vms, char *dir, char *file_path);
object_t *call_func(vm_state *vms, object_t *func, object_t *args_list);

char *interned_symbols[] = {
    "$env",
    "$dynlibs",
    "$parent",
    "$args",
    "last",
    "funcs",
    "handle",
    "quote",
    "$dir",
    "$file",
};

#define FIRST_INTERNED_OBJ_PTR(vms) \
    ((object_t **)(((char *) vms) + sizeof(vm_state)))

#define DLR_ENV_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[0])
#define DLR_DYNLIBS_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[1])
#define DLR_PARENT_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[2])
#define DLR_ARGS_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[3])
#define LAST_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[4])
#define FUNCS_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[5])
#define HANDLE_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[6])
#define QUOTE_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[7])
#define DLR_DIR_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[8])
#define DLR_FILE_SYM(vms) (FIRST_INTERNED_OBJ_PTR(vms)[9])

static void die(char *msg) {
    c_fprintf(stderr, "%s\n", msg);
    c_exit(1);
}

object_t *new_object_t(vm_state *vms, uint64_t type) {
    if (vms->n_objects == vms->max_objects && vms->gc_is_on) {
        gc(vms);
    }
    object_t *o = c_malloc(sizeof(object_t));
    o->type = type;
    o->marked = 0;
    o->next_object = vms->last_object;
    o->next_stack_object = NULL;
    vms->last_object = o;
    vms->n_objects++;
    return o;
}

object_t *new_int(vm_state *vms, int64_t n) {
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

object_t *new_macro(
    vm_state *vms,
    object_t *macro_args,
    object_t *macro_body,
    object_t* macro_parent_env
) {
    object_t *ret = new_object_t(vms, TYPE_MACRO);
    ret->macro_args = macro_args;
    ret->macro_body = macro_body;
    ret->macro_parent_env = macro_parent_env;
    return ret;
}

object_t *new_func(
    vm_state *vms,
    object_t *func_args,
    object_t *func_body,
    object_t *func_parent_env
) {
    object_t *ret = new_object_t(vms, TYPE_FUNC);
    ret->func_args = func_args;
    ret->func_body = func_body;
    ret->func_parent_env = func_parent_env;
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
    int64_t num = 0;

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

object_t *read_hex(vm_state *vms, char *s, uint64_t *i) {
    char digit;
    int64_t num = 0;

    for (;;) {
        digit = s[*i];
        if (digit >= '0' && digit <= '9') {
            num = num * 16 + digit - '0';
        } else if (digit >= 'a' && digit <= 'f') {
            num = num * 16 + (digit - 'a' + 10);
        } else {
            break;
        }

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
        if (
            c == ' ' ||
            c == '\n' ||
            c == '\r' ||
            c == '\t' ||
            c == ')' ||
            c == '('
        ) {
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

void print_list(vm_state *vms, object_t *o) {
    c_fprintf(stdout, "(");
    uint64_t first_elem = 1;

    object_t *pair = o;

    while (pair->head) {
        if (first_elem) {
            first_elem = 0;
        } else {
            c_fprintf(stdout, " ");
        }
        print(vms, pair->head);
        pair = pair->tail;
    }

end_print_list:
    c_fprintf(stdout, ")");
}

void print_dict(vm_state *vms, object_t *o) {
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
                print(vms, pair->key);
                c_fprintf(stdout, " ");
                print(vms, pair->value);
            }
            if (!pair->next) {
                break;
            }
            pair = pair->next;
        }
    }

    c_fprintf(stdout, ")");
}

void print(vm_state *vms, object_t *o) {
    switch (o->type) {
        case TYPE_INT:
            c_fprintf(stdout, "%lld", o->int_value);
            break;

        case TYPE_STRING:
            c_fprintf(stdout, "\'%s\'", o->string_pointer);
            break;

        case TYPE_LIST:
            print_list(vms, o);
            break;

        case TYPE_SYMBOL:
            c_fprintf(stdout, "%s", o->symbol_pointer);
            break;

        case TYPE_DICT:
            print_dict(vms, o);
            break;

        case TYPE_BUILTIN_FUNC:
            c_fprintf(stdout, "(builtin %p)", o->builtin);
            break;

        case TYPE_CONSTRUCT:
            c_fprintf(stdout, "(construct %p)", o->construct);
            break;

        case TYPE_MACRO:
            c_fprintf(stdout, "(macro ");
            if (o->macro_args->head) {
                print(vms, o->macro_args);
                c_fprintf(stdout, " ");
            }
            print(vms, o->macro_body);
            c_fprintf(stdout, ")");
            break;

        case TYPE_FUNC:
            c_fprintf(stdout, "(~ ");
            if (o->func_args->head) {
                print(vms, o->func_args);
                c_fprintf(stdout, " ");
            }
            print(vms, o->func_body);
            c_fprintf(stdout, ")");
            break;

        default:
            c_fprintf(stdout, "[Unknown type.]");
    }
}

#define twoargfunc(name, operator) \
    object_t *name(vm_state *vms, object_t *env, object_t *args_list) { \
        return new_int( \
            vms, \
            args_list->head->int_value \
            operator \
            args_list->tail->head->int_value \
        ); \
    }

twoargfunc(add_func, +)
twoargfunc(sub_func, -)
twoargfunc(mul_func, *)
twoargfunc(div_func, /)

void list_append(vm_state *vms, object_t *list, object_t *o) {
    object_t *p = list;
    while (p->head) {
        p = p->tail;
    }
    p->head = o;
    p->tail = new_pair(vms);
}

object_t *append_func(vm_state *vms, object_t *env, object_t *args_list) {
    list_append(vms, args_list->head, args_list->tail->head);
    return args_list->head;
}

object_t *head_func(vm_state *vms, object_t *env, object_t *args_list) {
    return args_list->head->head;
}

object_t *tail_func(vm_state *vms, object_t *env, object_t *args_list) {
    return args_list->head->tail;
}

void eval_args_list(vm_state *vms, object_t *env, object_t* args_list, object_t *list) {
    object_t *unevaled = list;

    if (!unevaled->head) {
        return;
    }

    object_t *evaled = args_list;

    for (;;) {
        evaled->head = eval(vms, env, unevaled->head);
        evaled->tail = new_pair(vms);
        unevaled = unevaled->tail;

        if (!unevaled->head) {
            break;
        }

        evaled = evaled->tail;
    }
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

object_t *hashcode_func(vm_state *vms, object_t *env, object_t *args_list) {
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

object_t *dict_func(vm_state *vms, object_t *env, object_t *args_list) {
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
            pair->value = value;
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

object_t *dict_get_null(vm_state *vms, object_t *d, object_t *key) {
    uint64_t hash = hashcode_object(key);
    dict_pair *pair = &d->dict_table[hash % d->dict_n_size];
    for (;;) {
        if (!pair->value) {
            return NULL;
        }
        if (objects_equal(key, pair->key)) {
            return pair->value;
        }
        if (!pair->next) {
            return NULL;
        }
        pair = pair->next;
    }
}

object_t *dict_get(vm_state *vms, object_t *d, object_t *key) {
    object_t *ret = dict_get_null(vms, d, key);
    return ret ? ret : new_pair(vms);
}

object_t *set_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *p = args_list;
    object_t *d = p->head;
    p = p->tail;
    object_t *key = p->head;
    p = p->tail;
    object_t *value = p->head;
    dict_add(d, key, value);
    return d;
}

object_t *get_func(vm_state *vms, object_t *env, object_t *args_list) {
    return dict_get(vms, args_list->head, args_list->tail->head);
}

int64_t obj_len_func(object_t *o) {
    switch (o->type) {
        case TYPE_INT:
            return o->int_value; // TODO: Use abs here.
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

object_t *len_func(vm_state *vms, object_t *env, object_t *args_list) {
    return new_int(vms, obj_len_func(args_list->head));
}

object_t *list_func(vm_state *vms, object_t *env, object_t *args_list) {
    return args_list;
}

object_t *quote_func(vm_state *vms, object_t *env, object_t *args_list) {
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

object_t *is_func(vm_state *vms, object_t *env, object_t *args_list) {
    return new_int(vms, objects_equal(args_list->head, args_list->tail->head));
}

object_t *repr_func(vm_state *vms, object_t *env, object_t *args_list) {
    print(vms, args_list->head);
    return new_pair(vms);
}

object_t *last_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *o = args_list;

    if (!o->head) {
        return o;
    }

    while (o->tail->head) {
        o = o->tail;
    }

    return o->head;
}

object_t *dynsym_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *lib_name = args_list->head;
    object_t *sym_name = args_list->tail->head;

    object_t *dynlibs = dict_get_null(vms, vms->env, DLR_DYNLIBS_SYM(vms));

    if (!dynlibs) {
        die("Couldn't find $dynlibs.");
    }

    object_t *dl_lib = dict_get_null(vms, dynlibs, lib_name);
    object_t *funcs;
    object_t *handle;

    if (!dl_lib) {
        uint64_t handle_ptr = (uint64_t) dlopen(lib_name->string_pointer, 1);

        if (!handle_ptr) {
            die("Failed to open.");
        }

        dl_lib = new_dict(vms, 4969);
        dict_add(dynlibs, lib_name, dl_lib);

        funcs = new_dict(vms, 4969);

        handle = new_int(vms, handle_ptr);
        dict_add(dl_lib, HANDLE_SYM(vms), handle);
        dict_add(dl_lib, FUNCS_SYM(vms), funcs);

    } else {
        funcs = dict_get_null(vms, dl_lib, FUNCS_SYM(vms));

        if (!funcs) {
            die("Failed to find 'funcs'.");
        }
        handle = dict_get_null(vms, dl_lib, HANDLE_SYM(vms));

        if (!handle) {
            die("Failed to find 'handle'.");
        }
    }

    object_t *func = dict_get_null(vms, funcs, sym_name);

    if (func) {
        return func;
    }

    uint64_t sym = (uint64_t) dlsym(
        (void *) handle->int_value,
        sym_name->string_pointer
    );

    if (!sym) {
        die("Failed to find the sym.");
    }

    object_t *sym_obj = new_int(vms, sym);

    dict_add(funcs, sym_name, sym_obj);

    return sym_obj;
}

object_t *ccall_func(vm_state *vms, object_t *env, object_t *args_list) {
    uint64_t len = obj_len_func(args_list) - 1;

    void *func = (void *) args_list->head->int_value;

    uint64_t *ptr_args = c_malloc(sizeof(uint64_t) * len);

    uint64_t i;
    object_t *next = args_list->tail;

    for (i = 0; i < len; i++) {
        object_t *arg = next->head;
        switch (arg->type) {
            case TYPE_INT:
                ptr_args[i] = (uint64_t) arg->int_value;
                break;
            case TYPE_STRING:
                ptr_args[i] = (uint64_t) arg->string_pointer;
                break;
            default:
                die("Don't know how to convert that type.");
                break;
        }
        next = next->tail;
    }

    switch (len) {
        case 0:
            return new_int(vms, ((uint64_t (*)()) func)());
        case 1:
            return new_int(vms, ((uint64_t (*)(
                uint64_t
            )) func)(
                ptr_args[0]
            ));
        case 2:
            return new_int(vms, ((uint64_t (*)(
                uint64_t,
                uint64_t
            )) func)(
                ptr_args[0],
                ptr_args[1]
            ));
        case 3:
            return new_int(vms, ((uint64_t (*)(
                uint64_t,
                uint64_t,
                uint64_t
            )) func)(
                ptr_args[0],
                ptr_args[1],
                ptr_args[2]
            ));
        case 4:
            return new_int(vms, ((uint64_t (*)(
                uint64_t,
                uint64_t,
                uint64_t,
                uint64_t
            )) func)(
                ptr_args[0],
                ptr_args[1],
                ptr_args[2],
                ptr_args[3]
            ));
        case 5:
            return new_int(vms, ((uint64_t (*)(
                uint64_t,
                uint64_t,
                uint64_t,
                uint64_t,
                uint64_t
            )) func)(
                ptr_args[0],
                ptr_args[1],
                ptr_args[2],
                ptr_args[3],
                ptr_args[4]
            ));
        case 6:
            return new_int(vms, ((uint64_t (*)(
                uint64_t,
                uint64_t,
                uint64_t,
                uint64_t,
                uint64_t,
                uint64_t
            )) func)(
                ptr_args[0],
                ptr_args[1],
                ptr_args[2],
                ptr_args[3],
                ptr_args[4],
                ptr_args[5]
            ));
    }
}

object_t *assemble_func(vm_state *vms, object_t *env, object_t *args_list) {
    // Compute the total size required.
    uint64_t n_bytes = list_length(args_list->head);

    // Get the page size.
    int64_t page_size = c_sysconf(0x1e); // _SC_PAGE_SIZE
    if (page_size == -1) {
        die("Failed to get the page size.");
    }

    // Allocate the page aligned buffer.
    int64_t aligned_size = page_size * (n_bytes / page_size + 1);
    unsigned char *buffer = c_memalign(page_size, aligned_size);

    // Allow both write and execute on these pages.
    // 7 is PROT_EXEC | PROT_READ | PROT_WRITE
    if (c_mprotect(buffer, aligned_size, 7)) {
        die("mprotect failed");
    }

    uint64_t i = 0;
    object_t *next = args_list->head;
    while (next->head) {
        buffer[i++] = next->head->int_value & 0xff;
        next = next->tail;
    }

    return new_int(vms, (uint64_t) buffer);
}

object_t *import_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *dir_str = dict_get_null(vms, env, DLR_DIR_SYM(vms));
    if (dir_str == NULL) {
        die("Couldn't find $dir.");
    }
    return eval_file(
        vms,
        dir_str->string_pointer,
        args_list->head->string_pointer
    );
}

object_t *builtin_func(vm_state *vms, object_t *env, object_t *args_list) {
    return new_builtin_func(vms, (func_pointer_t *) args_list->head->int_value);
}

object_t *stitch_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *ret = new_pair(vms);
    object_t *ret_el = ret;
    object_t *next_arg = args_list;
    object_t *next_arg_el;

    while (next_arg->head) {
        next_arg_el = next_arg->head;

        while (next_arg_el->head) {
            ret_el->head = next_arg_el->head;
            ret_el->tail = new_pair(vms);
            ret_el = ret_el->tail;
            next_arg_el = next_arg_el->tail;
        }

        next_arg = next_arg->tail;
    }

    return ret;
}

object_t *byte_explode_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *ret = new_pair(vms);
    object_t *ret_el = ret;
    object_t *outer_list = args_list;
    object_t *inner_list;
    uint64_t num_size;
    uint64_t num;
    uint64_t j;

    while (outer_list->head) {
        inner_list = outer_list->head;
        num_size = inner_list->head->int_value;
        inner_list = inner_list->tail;

        while (inner_list->head) {
            num = inner_list->head->int_value;
            for (j = 0; j < num_size; j++, num = num >> 8) {
                ret_el->head = new_int(vms, num & 0xff);
                ret_el->tail = new_pair(vms);
                ret_el = ret_el->tail;
            }
            inner_list = inner_list->tail;
        }
        outer_list = outer_list->tail;
    }

    return ret;
}

object_t *reduce_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *fn = args_list->head;
    object_t *tmp = args_list->tail;
    object_t *list = tmp->head;
    object_t *memo;
    object_t *call_args;
    tmp = tmp->tail;

    if (tmp->head) {
        memo = tmp->head;
    } else {
        memo = list->head;
        list = list->tail;
    }

    while (list->head) {
        vms->gc_is_on = 0;
        call_args = new_pair(vms);
        call_args->head = memo;
        tmp = call_args->tail = new_pair(vms);
        tmp->head = list->head;
        tmp->tail = new_pair(vms);
        vms->gc_is_on = 1;

        // TODO: Handle all 4 callable types: BUILTIN_FUNC, CONSTRUCT, MACRO,
        // and FUNC.
        memo = ((func_pointer_t *) fn->builtin)(vms, env, call_args);

        list = list->tail;
    }

    return memo;
}

// TODO: Handle all 4 callable types: BUILTIN_FUNC, CONSTRUCT, MACRO,
// and FUNC.
object_t *eval_func_call(
    vm_state *vms,
    object_t *fn,
    object_t *env,
    object_t *args_list
) {
    switch (fn->type) {
        case TYPE_BUILTIN_FUNC:
            return ((func_pointer_t *) fn->builtin)(
                vms,
                env,
                args_list
            );
        case TYPE_FUNC:
            return call_func(vms, fn, args_list);
        default:
            die("Deal with this case.");
    };
}

object_t *apply_func(vm_state *vms, object_t *env, object_t *args_list) {
    return eval_func_call(vms, args_list->head, env, args_list->tail->head);
}

// Only joins two lists.
object_t *join_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *ret = new_pair(vms);
    object_t *ret_obj = ret;

    object_t *next = args_list->head;

    while (next->head) {
        ret_obj->head = next->head;
        ret_obj->tail = new_pair(vms);
        next = next->tail;
        ret_obj = ret_obj->tail;
    }

    next = args_list->tail->head;

    while (next->head) {
        ret_obj->head = next->head;
        ret_obj->tail = new_pair(vms);
        next = next->tail;
        ret_obj = ret_obj->tail;
    }

    return ret;
}

object_t *map_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *list = args_list->head;
    object_t *fn = args_list->tail->head;

    object_t *ret = new_pair(vms);
    object_t *next = ret;
    object_t *fn_args;
    uint64_t index = 0;

    while (list->head) {
        fn_args = new_pair(vms);
        fn_args->head = list->head;
        fn_args->tail = new_pair(vms);
        fn_args->tail->head = new_int(vms, index);
        fn_args->tail->tail = new_pair(vms);

        next->head = eval_func_call(vms, fn, env, fn_args);
        next->tail = new_pair(vms);

        list = list->tail;
        next = next->tail;
        index++;
    }

    return ret;
}

object_t *range_func(vm_state *vms, object_t *env, object_t *args_list) {
    uint64_t start = 0;
    uint64_t stop;
    uint64_t step = 1;

    object_t *tmp = args_list->tail;
    object_t *arg2 = tmp->head;

    if (arg2) {
        start = args_list->head->int_value;
        stop = arg2->int_value;
        tmp = tmp->tail;

        if (tmp->head) {
            step = tmp->head->int_value;
        }
    } else {
        stop = args_list->head->int_value;
    }

    object_t *ret = new_pair(vms);
    object_t *next = ret;

    if (start < stop) {
        for (; start < stop; start += step) {
            next->head = new_int(vms, start);
            next->tail = new_pair(vms);
            next = next->tail;
        }
    } else {
        for (; start > stop; start += step) {
            next->head = new_int(vms, start);
            next->tail = new_pair(vms);
            next = next->tail;
        }
    }

    return ret;
}

object_t *split_func(vm_state *vms, object_t *env, object_t *args_list) {
    char *needle = args_list->head->string_pointer;
    uint64_t needle_len = c_strlen(needle);
    char *haystack = args_list->tail->head->string_pointer;

    object_t *ret = new_pair(vms);
    object_t *next = ret;
    char *match;

    while (match = c_strstr(haystack, needle)) {
        next->head = new_string(vms, haystack, match - haystack);
        next->tail = new_pair(vms);
        next = next->tail;
        haystack = match + needle_len;
    }

    next->head = new_string(vms, haystack, c_strlen(haystack));
    next->tail = new_pair(vms);

    return ret;
}

object_t *fs_read_func(vm_state *vms, object_t *env, object_t *args_list) {
}

void read_file(char *file_path, char **content, uint64_t *file_length) {
    FILE *f = c_fopen(file_path, "rb");
    object_t *ret;

    if (!f) {
        die("File error.");
    }

    // Find out the size of the file by going to the end.
    c_fseek(f, 0, SEEK_END);
    *file_length = c_ftell(f);
    c_fseek(f, 0, SEEK_SET);

    *content = c_malloc(*file_length);

    if (!*content) {
        die("File error.");
    }

    if (c_fread(*content, 1, *file_length, f) != *file_length) {
        die("File error.");
    }

    c_fclose(f);
}

object_t *fs_write_func(vm_state *vms, object_t *env, object_t *args_list) {
}

object_t *get_env_of_name(vm_state *vms, object_t *env, object_t *name) {
    object_t *parent_sym = DLR_PARENT_SYM(vms);

    object_t *parent;
    object_t *value;
    object_t *curr_env = env;

    for (;;) {
        // If the value is in the current env, then return the env;
        value = dict_get_null(vms, curr_env, name);
        if (value) {
            return curr_env;
        }

        // If this env has no $parent, then this is the root env so return the
        // leaf env.
        parent = dict_get_null(vms, curr_env, parent_sym);
        if (!parent) {
            return env;
        }

        curr_env = parent;
    }
}

object_t *assign_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *name;
    object_t *value;
    object_t *list = args_list;
    object_t *ret = NULL;

    for (;;) {
        name = list->head;
        if (!name) {
            break;
        }

        list = list->tail;
        value = list->head;
        if (!name) {
            die("No value to assign to name.");
        }

        list = list->tail;

        dict_add(
            get_env_of_name(vms, env, name),
            name,
            ret = eval(vms, env, value)
        );
    }
    return ret ? ret : new_pair(vms);
}

/*
 * This function accepts either one or two arguments. E.g.:
 *   (~ <body>)
 *   (~ <args> <body>)
 */
object_t *func_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *arg1 = args_list->head;
    object_t *arg2 = args_list->tail->head;

    object_t *args;
    object_t *body;

    if (arg2) {
        args = arg1;
        body = arg2;
    } else {
        args = new_pair(vms);
        body = arg1;
    }

    return new_func(vms, args, body, env);
}

/*
 * This function accepts either one or two arguments. E.g.:
 *   (macro <body>)
 *   (macro <args> <body>)
 */
object_t *macro_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *arg1 = args_list->head;
    object_t *arg2 = args_list->tail->head;

    object_t *args;
    object_t *body;

    if (arg2) {
        args = arg1;
        body = arg2;
    } else {
        args = new_pair(vms);
        body = arg1;
    }
    return new_macro(vms, args, body, env);
}

object_t *if_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *arg = args_list;
    object_t *next_arg;

    for (;;) {
        if (!arg->head) {
            return new_pair(vms);
        }

        next_arg = arg->tail;

        if (!next_arg->head) {
            return eval(vms, env, arg->head);
        }

        if (obj_len_func(eval(vms, env, arg->head))) {
            return eval(vms, env, next_arg->head);
        }

        arg = next_arg->tail;
    }
}

object_t *or_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *prev = eval(vms, env, args_list->head);
    object_t *next = args_list->tail;
    object_t *evaled_now;

    if (obj_len_func(prev)) {
        return prev;
    }

    for (;;) {
        if (!next->head) {
            return prev;
        }

        evaled_now = eval(vms, env, next->head);

        if (obj_len_func(evaled_now)) {
            return evaled_now;
        }

        next = next->tail;
        prev = evaled_now;
    }
}

object_t *switch_func(vm_state *vms, object_t *env, object_t *args_list) {
    object_t *o = args_list->head;
    object_t *arg = args_list->tail;
    object_t *next_arg;

    for (;;) {
        if (!arg->head) {
            return new_pair(vms);
        }

        next_arg = arg->tail;

        if (!next_arg->head) {
            return eval(vms, env, arg->head);
        }

        if (objects_equal(o, eval(vms, env, arg->head))) {
            return eval(vms, env, next_arg->head);
        }

        arg = next_arg->tail;
    }
}

void populate_child_env(
    vm_state *vms,
    object_t *parent_env,
    object_t *child_env,
    object_t *arg_names,
    object_t *arg_values
) {
    dict_add(child_env, DLR_PARENT_SYM(vms), parent_env);
    dict_add(child_env, DLR_ARGS_SYM(vms), arg_values);

    object_t *arg_name = arg_names;
    object_t *arg_value = arg_values;

    while (arg_name->head) {
        dict_add(
            child_env,
            new_symbol(
                vms,
                arg_name->head->symbol_pointer,
                arg_name->head->symbol_length
            ),
            arg_value->head
        );
        arg_name = arg_name->tail;

        if (arg_value->head) {
            arg_value = arg_value->tail;
        }
    }
}

object_t *call_func(vm_state *vms, object_t *func, object_t *args_list) {
    // Turn gc off in order to create the child env.
    vms->gc_is_on = 0;
    object_t *child_env = new_dict(vms, 4969);
    ADD_ON_CALL_STACK(vms, child_env);
    vms->gc_is_on = 1;

    populate_child_env(
        vms, func->func_parent_env, child_env, func->func_args, args_list
    );

    return eval(vms, child_env, func->func_body);
}

object_t *eval_list(vm_state *vms, object_t *env, object_t *o) {
    object_t *ret;
    object_t *top_call_stack_elem = vms->call_stack_objects;

    ADD_ON_CALL_STACK(vms, o);

    // An empty list evaluates to itself.
    if (!o->head) {
        ret = o;
        goto eval_list_cleanup;
    }

    object_t *first_elem = o->head;
    object_t *func;

    if (first_elem->type == TYPE_SYMBOL || first_elem->type == TYPE_LIST) {
        func = eval(vms, env, first_elem);
    } else {
        die("For lists to be evaled they need to start with a symbol or a list");
    }


    if (func->type == TYPE_CONSTRUCT) {
        ret = (func->construct)(vms, env, o->tail);
        goto eval_list_cleanup;
    }

    if (func->type == TYPE_MACRO) {
        // Turn gc off in order to create the child env.
        vms->gc_is_on = 0;
        object_t *child_env = new_dict(vms, 4969);
        ADD_ON_CALL_STACK(vms, child_env);
        vms->gc_is_on = 1;

        populate_child_env(
            vms, func->macro_parent_env, child_env, func->macro_args, o->tail
        );

        ret = eval(vms, env, eval(vms, child_env, func->macro_body));
        goto eval_list_cleanup;
    }

    // Turn gc off in order to create the eval args list.
    vms->gc_is_on = 0;
    object_t *args_list = new_pair(vms);
    ADD_ON_CALL_STACK(vms, args_list);
    vms->gc_is_on = 1;

    eval_args_list(vms, env, args_list, o->tail);

    if (func->type == TYPE_FUNC) {
        ret = call_func(vms, func, args_list);
        goto eval_list_cleanup;
    }

    if (func->type == TYPE_BUILTIN_FUNC) {
        ret = ((func_pointer_t *) func->builtin)(
            vms, env, args_list
        );
        goto eval_list_cleanup;
    }

    die("That's not a function.");

eval_list_cleanup:
    vms->call_stack_objects = top_call_stack_elem;

    return ret;
}

object_t *eval(vm_state *vms, object_t *env, object_t *o) {
    switch (o->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_DICT:
        case TYPE_MACRO:
        case TYPE_FUNC:
            return o;

        case TYPE_SYMBOL:
            {
                vms->gc_is_on = 0;
                object_t *parent_sym = DLR_PARENT_SYM(vms);
                vms->gc_is_on = 1;

                object_t *parent;
                object_t *value;
                object_t *curr_env = env;

                for (;;) {
                    parent = dict_get_null(vms, curr_env, parent_sym);
                    if (!parent) {
                        return dict_get(vms, curr_env, o);
                    }
                    value = dict_get_null(vms, curr_env, o);
                    if (value) {
                        return value;
                    }
                    curr_env = parent;
                }
            }

        case TYPE_LIST:
            return eval_list(vms, env, o);

        default:
            die("Don't know how to eval that.");
    }
}

void discard_line(char *s, uint64_t *i) {
    while (s[(*i)++] != '\n');
}

object_t *parse_recursive(vm_state *vms, char *s, uint64_t *i, uint64_t len) {
    object_t *ret;
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
            ret = (object_t *) ')';
            goto parse_recursive_discard_non_object;
        }

        if (c == '0' && *i + 1 < len && s[*i] == 'x') {
            (*i)++;
            ret = read_hex(vms, s, i);
            goto parse_recursive_discard_non_object;
        }

        if (c >= '0' && c <= '9') {
            (*i)--;
            ret = read_int(vms, s, i);
            goto parse_recursive_discard_non_object;
        }

        if (c == '-' && *i < len) {
            char c2 = s[*i];
            if (c2 >= '0' && c2 <= '9') {
                ret = read_int(vms, s, i);
                ret->int_value = -ret->int_value;
                goto parse_recursive_discard_non_object;
            }
        }

        if (c == '\'') {
            ret = read_string(vms, s, i);
            goto parse_recursive_discard_non_object;
        }

        if (c == ':') {
            ret = new_pair(vms);
            ret->head = QUOTE_SYM(vms);
            ret->tail = new_pair(vms);
            ret->tail->head = parse_recursive(vms, s, i, len);
            ret->tail->tail = new_pair(vms);
            goto parse_recursive_discard_non_object;
        }

        (*i)--;
        return read_symbol(vms, s, i);
    }

    // This means we have reached the end of an expression. It useful at
    // this point to remove all the non instructions (whitespace and
    // comments).
parse_recursive_discard_non_object:

    for (;;) {
        if (*i >= len - 1) {
            break;
        }

        c = s[(*i)++];

        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            continue;
        }

        if (c == '#') {
            discard_line(s, i);
            continue;
        }

        (*i)--;
        break;
    }

    return ret;
}

object_t *parse(vm_state *vms, char *s, uint64_t len) {
    uint64_t i = 0;
    object_t *ret = new_pair(vms);
    object_t *pair = ret;

    pair->head = LAST_SYM(vms);
    pair->tail = new_pair(vms);

    while (i < len - 1) {
        pair = pair->tail;
        pair->head = parse_recursive(vms, s, &i, len);
        pair->tail = new_pair(vms);
    }

    return ret;

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
        case TYPE_MACRO:
        case TYPE_FUNC:
            break;

        case TYPE_STRING:
            c_free(o->string_pointer);
            o->string_pointer = NULL;
            break;

        case TYPE_SYMBOL:
            c_free(o->symbol_pointer);
            o->symbol_pointer = NULL;
            break;

        case TYPE_DICT:
            free_dict(o);
            break;

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
    "add",
    "sub",
    "mul",
    "div",
    "repr",
    "last",
    "dynsym",
    "ccall",
    "assemble",
    "import",
    "builtin",
    "stitch",
    "byte-explode",
    "reduce",
    "apply",
    "join",
    "map",
    "range",
    "split",
    "fs-read",
    "fs-write",
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
    add_func,
    sub_func,
    mul_func,
    div_func,
    repr_func,
    last_func,
    dynsym_func,
    ccall_func,
    assemble_func,
    import_func,
    builtin_func,
    stitch_func,
    byte_explode_func,
    reduce_func,
    apply_func,
    join_func,
    map_func,
    range_func,
    split_func,
    fs_read_func,
    fs_write_func,
};

char *construct_names[] = {
    "quote",
    "macro",
    "=",
    "~",
    "if",
    "switch",
    "or",
};
func_pointer_t *construct_pointers[] = {
    quote_func,
    macro_func,
    assign_func,
    func_func,
    if_func,
    switch_func,
    or_func,
};

vm_state *start_vm() {
    uint64_t n_interned_symbols = sizeof(interned_symbols) / sizeof(char *);
    uint64_t n_interned_objects = n_interned_symbols; // + n_interned_ints;

    vm_state *vms = c_malloc(
        sizeof(vm_state) +
        sizeof(object_t *) * n_interned_objects
    );

    vms->n_objects = 0;
    vms->max_objects = 8;
    vms->gc_is_on = 0;
    vms->last_object = NULL;
    vms->call_stack_objects = NULL;
    vms->env = new_dict(vms, 4969);
    vms->interned = new_dict(vms, 4969);

    uint64_t i;
    uint64_t n;
    object_t *d;

    d = vms->interned;
    n = sizeof(interned_symbols) / sizeof(char *);

    for (i = 0; i < n; i++) {
        object_t *s = new_symbol(
            vms,
            interned_symbols[i],
            c_strlen(interned_symbols[i])
        );
        dict_add(d, s, s);

        *(FIRST_INTERNED_OBJ_PTR(vms) + i) = s;
    }

    dict_add(vms->env, DLR_ENV_SYM(vms), vms->env);
    dict_add(vms->env, DLR_DYNLIBS_SYM(vms), new_dict(vms, 4969));

    d = vms->env;
    n = sizeof(builtin_pointers) / sizeof(uint64_t);

    for (i = 0; i < n; i++) {
        dict_add(
            d,
            new_symbol(vms, builtin_names[i], c_strlen(builtin_names[i])),
            new_builtin_func(vms, builtin_pointers[i])
        );
    }

    n = sizeof(construct_pointers) / sizeof(uint64_t);

    for (i = 0; i < n; i++) {
        dict_add(
            d,
            new_symbol(vms, construct_names[i], c_strlen(construct_names[i])),
            new_construct(vms, construct_pointers[i])
        );
    }

    object_t *dynlibs = dict_get_null(vms, vms->env, DLR_DYNLIBS_SYM(vms));

    object_t *dl_lib = new_dict(vms, 4969);
    dict_add(dynlibs, new_string(vms, "libc.so", 7), dl_lib);

    dict_add(dl_lib, HANDLE_SYM(vms), new_int(vms, (uint64_t) libc_handle));
    dict_add(dl_lib, FUNCS_SYM(vms), new_dict(vms, 4969));

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
                    if (!pair) {
                        break;
                    }
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

        case TYPE_MACRO:
            mark(o->macro_args);
            mark(o->macro_body);
            mark(o->macro_parent_env);
            break;

        case TYPE_FUNC:
            mark(o->func_args);
            mark(o->func_body);
            mark(o->func_parent_env);
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
            vms->n_objects--;
        } else {
            (*o)->marked = 0;
            o = &(*o)->next_object;
        }
    }
}

void get_dir_and_file(
    char *src_dir,
    char *src_file,
    char *dst_dir,
    char *dst_file
) {
    uint64_t len;

    if (src_file[0] == '/') {
        len = c_strlen(src_file);
        c_memcpy(dst_file, src_file, len);
    } else {
        len = c_strlen(src_dir);
        c_memcpy(dst_file, src_dir, len);
        c_strcat(dst_file, "/");
        c_strcat(dst_file, src_file);
        len = c_strlen(dst_file);
    }

    c_memcpy(dst_dir, dst_file, len);
    while (dst_dir[--len] != '/');
    dst_dir[len] = '\0';
}

/*
 * You need to check that vms->gc_is_on is true before calling this function.
 */
void gc(vm_state *vms) {
    vms->gc_is_on = 0;

    mark(vms->env);
    mark(vms->interned);

    object_t *next = vms->call_stack_objects;

    uint64_t n_st = 0;
    while (next) {
        mark(next);
        next = next->next_stack_object;
        n_st++;
    }

    sweep(vms);

    vms->max_objects = vms->n_objects * 2;

    vms->gc_is_on = 1;
}

object_t *eval_file(vm_state *vms, char *dir, char *file_path) {
    char actual_file_path[PATH_MAX + 1];
    char actual_file_parent[PATH_MAX + 1];

    get_dir_and_file(dir, file_path, actual_file_parent, actual_file_path);

    uint64_t file_length;
    char *content;

    read_file(actual_file_path, &content, &file_length);

    // Parse the file and create the child_env which will store the '$file' and
    // '$dir' variables.
    vms->gc_is_on = 0;
    object_t *parsed = parse(vms, content, file_length);
    object_t *child_env = new_dict(vms, 4969);
    ADD_ON_CALL_STACK(vms, child_env);

    dict_add(child_env, DLR_PARENT_SYM(vms), vms->env);
    dict_add(
        child_env,
        DLR_DIR_SYM(vms),
        new_string(vms, actual_file_parent, c_strlen(actual_file_parent))
    );
    dict_add(
        child_env,
        DLR_FILE_SYM(vms),
        new_string(vms, actual_file_path, c_strlen(actual_file_path))
    );
    vms->gc_is_on = 1;

    object_t *ret;
    ret = eval(vms, child_env, parsed);
    goto eval_file_cleanup2;

eval_file_cleanup:
    ret = new_pair(vms);

eval_file_cleanup2:
    if (content) {
        c_free(content);
    }

    return ret;
}

void eval_lines() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    object_t *parsed;
    object_t *o;
    vm_state *vms = start_vm();

    vms->gc_is_on = 0;
    parsed = parse(vms, INITIAL_CODE, c_strlen(INITIAL_CODE));
    vms->gc_is_on = 1;
    eval(vms, vms->env, parsed);

    // If argc is 1, that means there are no arguments so just run a REPL.
    if (*prog_argc_ptr == 1) {
        for (;;) {
            c_fprintf(stderr, "> ");
            read = c_getline(&line, &len, stdin);
            if (read == -1) {
                break;
            }
            vms->gc_is_on = 0;
            parsed = parse(vms, line, c_strlen(line));
            vms->gc_is_on = 1;
            o = eval(vms, vms->env, parsed);
            print(vms, o);
            c_fprintf(stdout, "\n");
            if (vms->gc_is_on) {
                gc(vms);
            }

        }
        goto eval_lines_cleanup;
    }

    char *file_path = *((char **)prog_argc_ptr + 2);
    char cwd[PATH_MAX + 1];

    if (c_getcwd(cwd, PATH_MAX + 1) == NULL) {
        die("getcwd failed");
    }

    eval_file(vms, cwd, file_path);

eval_lines_cleanup:
    if (vms->gc_is_on) {
        gc(vms);
    }
    if (line) {
        c_free(line);
    }
    sweep(vms);
    c_free(vms);

    c_fflush(stdout);
    c_fflush(stderr);
}
