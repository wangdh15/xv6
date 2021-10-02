#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void test1() {
    int p[2];
    int pid;
    int ch;
    int num = 12345678;
    pipe(p);

    pid = fork();

    if (pid == 0) {
        close(p[1]);
        read(p[0], &ch, 4);
        fprintf(1, "%d", ch);
        close(p[0]);
    } else {
        close(p[0]);
        write(p[1], &num, 4);
        close(p[1]);
    }
    return;
}

void test2() {

    int p[2];
    int pid;
    int n;
    int num = 0x12345678;

    pipe(p);

    pid = fork();
    if (pid == 0) {
        close(p[1]);
        n = read(p[0], &num, 4);
        fprintf(1, "n: %d, num: %d\n", n, num);
        close(p[0]);
        exit(0);
    } else {
        close(p[0]);
        close(p[1]);
        exit(0);
    }
}


int main() {

    // test1();
    test2();
    exit(0);
}