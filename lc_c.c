#include <stdio.h>
#include <stdlib.h>

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
        printf("%s\n", line);
    }

    free(line);
}
