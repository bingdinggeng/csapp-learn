#include <stdio.h>

size_t my_strlen(const char *s) {
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

void my_strcpy(char *dst, const char *src) {
    while (*src != '\0') {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0';
}

int main(void) {
    char src[] = "CSAPP";
    char dst[16];

    my_strcpy(dst, src);
    printf("src=%s len=%zu\n", src, my_strlen(src));
    printf("dst=%s len=%zu\n", dst, my_strlen(dst));

    return 0;
}
