#include <stdio.h>

int main(void) {
    int arr[] = {10, 20, 30, 40};
    size_t n = sizeof(arr) / sizeof(arr[0]);

    for (size_t i = 0; i < n; i++) {
        printf("arr[%zu] value=%d address=%p\n", i, arr[i], (void *)&arr[i]);
    }

    return 0;
}
