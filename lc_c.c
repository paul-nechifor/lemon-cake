#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    TYPE_INT = 1,
    TYPE_STRING,
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

static void die(char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

object *new_int(int n) {
    object *o = malloc(sizeof(object));
    o->type = TYPE_INT;
    o->value = malloc(sizeof(int));
    *((int *) o->value) = n;
    return o;
}

void free_int(object *o) {
    free(o->value);
    free(o);
}

object *new_string(char *s, int chars) {
    object *o = malloc(sizeof(object));
    o->type = TYPE_STRING;

    string_struct *ss = malloc(sizeof(string_struct));
    ss->length = chars;
    ss->value = malloc(sizeof(char) * (chars + 1));
    memcpy(ss->value, s, chars);

    o->value = ss;

    return o;
}

void free_string(object *o) {
    free(((string_struct *) o->value)->value);
    free(o->value);
    free(o);
}

object *read_int(char *s, int *i) {
    char digit;
    int num = 0;
    while (isdigit(digit = s[(*i)++])) {
        num = num * 10 + digit - '0';
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

void print(object *o) {
    switch (o->type) {
        case TYPE_INT:
            printf("%d", *((int *) o->value));
            break;

        case TYPE_STRING:
            printf("\"%s\"", ((string_struct*) o->value)->value);
            break;

        default:
            printf("[Unknown type.]");
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
            (*i)--;
            return read_int(s, i);
        }

        return read_string(s, i);
    }
}

void free_object(object *o) {
    switch (o->type) {
        case TYPE_INT:
            free_int(o);
            break;

        case TYPE_STRING:
            free_string(o);
            break;

        default:
            die("Free not implemented for this object type");
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
    object *o;

    for (;;) {
        fprintf(stderr, "> ");
        read = getline(&line, &len, stdin);
        if (read == -1) {
            break;
        }
        fprintf(stderr, "< ");
        o = eval(parse(line, len));
        print(o);
        free_object(o);
    }

    free(line);
}
