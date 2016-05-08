#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define offsetof(st, m) ((size_t) ( (char *)&((st *)0)->m - (char *)0 ))

extern size_t (*c_strlen)(const char *str);
extern void *(*c_malloc)(size_t size);
extern int (*c_fprintf)(FILE *stream, const char *format, ...);
extern void (*c_free)(void *ptr);
extern void *(*c_memcpy)(void *destination, const void *source, size_t num);
extern ssize_t (*c_getline)(char **lineptr, size_t *n, FILE *stream);
extern void (*c_exit)(int status);

enum {
    TYPE_INT = 1,
    TYPE_STRING,
    TYPE_NULL,
    TYPE_LIST,
};

typedef struct {
    // The number of actual characters ('\0' is not included).
    int length;
    // Contains `length` characters plus an aditional '\0' at the end.
    char *value;
} string_struct;

typedef struct {
    int type;
    void *value;
} object;

struct list_elem {
    object *value;
    struct list_elem *next;
};
struct list_elem;
typedef struct list_elem list_elem;

static object *null_instance;

object *parse_recursive(char *s, int *i, int len);
void print(object *o);

static void die(char *msg) {
    c_fprintf(stderr, "%s\n", msg);
    c_exit(1);
}

object *new_int(int n) {
    object *o = c_malloc(sizeof(object));
    o->type = TYPE_INT;
    o->value = c_malloc(sizeof(int));
    *((int *) o->value) = n;
    return o;
}

object *new_string(char *s, int chars) {
    object *o = c_malloc(sizeof(object));
    o->type = TYPE_STRING;

    string_struct *ss = c_malloc(sizeof(string_struct));
    ss->length = chars;
    ss->value = c_malloc(sizeof(char) * (chars + 1));
    c_memcpy(ss->value, s, chars);

    o->value = ss;

    return o;
}

object *new_list() {
    object *o = c_malloc(sizeof(object));
    o->type = TYPE_LIST;

    list_elem *le = c_malloc(sizeof(list_elem));
    le->value = NULL;
    le->next = NULL;

    o->value = le;

    return o;
}

void free_list(object *o) {
    // TODO: Free lists.
}

object *read_int(char *s, int *i) {
    char digit;
    int num = 0;

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

object *read_string(char *s, int *i) {
    int start = *i;
    int end = start;
    while (s[end] != '"') {
        end++;
    }
    int chars = end - start;

    object *o = new_string(&s[start], chars);

    *i = end + 1;

    return o;
}

object *read_list(char *s, int *i, int len) {
    object *o = new_list(NULL, NULL);
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
    int first_elem = 1;

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
            c_fprintf(stdout, "%d", *((int *) o->value));
            break;

        case TYPE_STRING:
            c_fprintf(stdout, "\"%s\"", ((string_struct*) o->value)->value);
            break;

        case TYPE_NULL:
            c_fprintf(stdout, "null");
            break;

        case TYPE_LIST:
            print_list(o);
            break;

        default:
            c_fprintf(stdout, "[Unknown type.]");
    }
}

object *eval(object *o) {
    return o;
}

void discard_line(char *s, int *i) {
    while (s[(*i)++] != '\n');
}

object *parse_recursive(char *s, int *i, int len) {
    char c;
    for (;;) {
        if (*i > len) {
            die("Stepping over the end of the code.");
        }

        if (*i == len) {
            return null_instance;
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

        return read_string(s, i);
    }
}

object *parse(char *s, int len) {
    int i = 0;
    return parse_recursive(s, &i, len);
}

void free_object(object *o) {
    switch (o->type) {
        case TYPE_INT:
            break;

        case TYPE_STRING:
            c_free(((string_struct *) o->value)->value);
            break;

        case TYPE_NULL:
            return;

        case TYPE_LIST:
            free_list(o);
            break;

        default:
            die("Free not implemented for this object type.");
    }
    if (o->value) {
        c_free(o->value);
    }
    c_free(o);
}

void load_c_functions() {
    void *c_handle = dlopen("libc.so", RTLD_LAZY);

    *(void **) (&c_strlen) = dlsym(c_handle, "strlen");
    *(void **) (&c_malloc) = dlsym(c_handle, "malloc");
    *(void **) (&c_fprintf) = dlsym(c_handle, "fprintf");
    *(void **) (&c_free) = dlsym(c_handle, "free");
    *(void **) (&c_memcpy) = dlsym(c_handle, "memcpy");
    *(void **) (&c_getline) = dlsym(c_handle, "getline");
    *(void **) (&c_exit) = dlsym(c_handle, "exit");
}

void eval_lines() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    object *o;
    null_instance = c_malloc(sizeof(object));
    null_instance->type = TYPE_NULL;
    null_instance->value = NULL;

    for (;;) {
        c_fprintf(stderr, "> ");
        read = c_getline(&line, &len, stdin);
        if (read == -1) {
            break;
        }
        o = eval(parse(line, c_strlen(line)));
        print(o);
        c_fprintf(stdout, "\n");
        free_object(o);
    }

    if (line) {
        c_free(line);
    }

    c_free(null_instance);
}
