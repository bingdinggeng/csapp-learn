#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *my_strdup(const char *s) {
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, s, len + 1);
    return copy;
}

int main(void) {
    int n = 5;
    int *arr = malloc(sizeof(int) * n);
    if (arr == NULL) {
        return 1;
    }

    for (int i = 0; i < n; i++) {
        arr[i] = i * i;
    }

    for (int i = 0; i < n; i++) {
        printf("arr[%d]=%d\n", i, arr[i]);
    }

    char *name = my_strdup("CSAPP");
    if (name == NULL) {
        free(arr);
        return 1;
    }
    printf("name=%s\n", name);

    free(name);
    free(arr);
    return 0;
}
