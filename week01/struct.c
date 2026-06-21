#include <stdio.h>

struct Student {
    char name[32];
    int age;
    double score;
};

void print_student(const struct Student *student) {
    printf("name=%s age=%d score=%.1f\n", student->name, student->age, student->score);
}

void update_score(struct Student *student, double score) {
    student->score = score;
}

int main(void) {
    struct Student student = {"Alice", 20, 88.5};

    print_student(&student);
    update_score(&student, 95.0);
    print_student(&student);
    printf("sizeof(struct Student)=%zu\n", sizeof(struct Student));

    return 0;
}
