#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define member_size(type, member) sizeof(((type *)0)->member)
#define size_of_object_struct(member) (offsetof(object, member) + member_size(object, member))

enum {
    TYPE_INT = 1,
    TYPE_STRING,
};

typedef struct {
    // The number of actual characters ('\0' is not included).
    int length;
    // Contains `length` characters plus an aditional '\0' at the end.
    char *value;
} string_object;

typedef struct {
    int type;

    union {
        int int_value;

        string_object *string_value;
    };
} object;

static void die(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

object *create_int(char *s, int *i) {
    char digit;
    int num = 0;
    while (isdigit(digit = s[(*i)++])) {
        num = num * 10 + digit;
    }

    object *ret = malloc(size_of_object_struct(int_value));
    ret->type = TYPE_INT;
    ret->int_value = atoi(s);
    return ret;
}

object *create_string(char *s, int *i) {
    int start = *i;
    int end = start;
    while (s[end] != '"') {
        end++;
    }
    int chars = end - start;

    object *ret = malloc(size_of_object_struct(string_value));
    ret->type = TYPE_STRING;
    ret->string_value = malloc(sizeof(string_object));
    ret->string_value->length = chars;
    ret->string_value->value = malloc(sizeof(char *) * (chars + 1));
    memcpy(ret->string_value->value, &s[start], chars);
    ret->string_value->value[chars] = 0;

    *i = end + 1;

    return ret;
}

void print(object *o) {
    switch (o->type) {
        case TYPE_INT:
            printf("%d", o->int_value);
            break;

        case TYPE_STRING:
            printf("\"%s\"", o->string_value->value);
            break;

        default:
            printf("Unknown type.");
    }
    printf("\n");
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
        if (*i >= len) {
            die("Stepping over the end of the code.");
        }

        c = s[(*i)++];

        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            continue;
        }

        if (c == '#') {
            discard_line(s, i);
            continue;
        }

        if (isdigit(c)) {
            return create_int(s, i);
        }

        return create_string(s, i);
    }
}

object *parse(char *s, int len) {
    int i = 0;
    return parse_recursive(s, &i, len);
}

void c_main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    for (;;) {
        fprintf(stderr, "> ");
        read = getline(&line, &len, stdin);
        if (read == -1) {
            break;
        }
        fprintf(stderr, "< ");
        print(eval(parse(line, len)));
    }

    free(line);
}
