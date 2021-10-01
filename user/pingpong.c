#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    int pid;
    int p[2];
    pipe(p);

    pid = fork();
    if (pid == 0) {
        close(p[0]);
        fprintf(1, "%d: received ping\n", getpid());
        write(p[1], "a", 1);
        close(p[1]);
    } else {
        char ch[1];
        close(p[1]);
        read(p[0], ch, 1);
        fprintf(1, "%d: received pong\n", getpid());
        close(p[0]);
    }
    exit(0);
}