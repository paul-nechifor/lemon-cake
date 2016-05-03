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

object *create_int(char *s) {
    object *ret = malloc(size_of_object_struct(int_value));
    ret->type = TYPE_INT;
    ret->int_value = atoi(s);
    return ret;
}

object *create_string(char *s) {
    int i = 1;
    while (s[i] != '"') {
        i++;
    }
    int chars = i - 1;

    object *ret = malloc(size_of_object_struct(string_value));
    ret->type = TYPE_STRING;
    ret->string_value = malloc(sizeof(string_object));
    ret->string_value->length = chars;
    ret->string_value->value = malloc(sizeof(char *) * (chars + 1));
    memcpy(ret->string_value->value, &s[1], chars);
    ret->string_value->value[chars] = 0;

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

object *parse(char *s) {
    if (s[0] > '0' && s[0] < '9') {
        return create_int(s);
    } else {
        return create_string(s);
    }
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
        print(eval(parse(line)));
    }

    free(line);
}
