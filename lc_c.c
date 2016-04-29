#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    TYPE_INT = 1,
};

typedef struct {
    int type;

    union {
        int int_value;
    };
} object;

object *make_int(char *s) {
    size_t size = offsetof(object, int_value) + sizeof(int);
    object *ret = malloc(size);
    ret->type = TYPE_INT;
    ret->int_value = atoi(s);
    return ret;
}

void print(object *o) {
    switch (o->type) {
        case TYPE_INT:
            printf("%d", o->int_value);
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
    return make_int(s);
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
